/*
 * ADXL345/346 Three-Axis Digital Accelerometers (I2C/SPI Interface)
 *
 * Enter bugs at http://blackfin.uclinux.org/
 *
 * Copyright (C) 2009 Michael Hennerich, Analog Devices Inc.
 * Licensed under the GPL-2 or later.
 */

#include <linux/device.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/spi/spi.h>
#include <linux/i2c.h>
/*Add sensor self adapt*/
#include <hsad/config_interface.h>
/* add early suspend */
#include <linux/earlysuspend.h>
#include "adxl34x.h"
#include "../sensors.h"

struct data_rate_map data_rate_tbl[] =
{
    {625,   0x0e},
    {1250,  0x0d},
    {2500,  0x0c},
    {5000,  0x0b},
    {10000, 0x0a},
    {20000, 0x09},
    {40000, 0x08},
    {80000, 0x07},
    {160000,0x06},
    {320000,0x05},
    {640000,0x04},
    {1280000,0x03},
    {2560000,0x02},
    {5000000, 0x01},
    {10000000, 0x00},
};

static int16_t adxl34_data[3] = {0, 0, 0};

/*move mutex to work function*/
static void adxl34x_get_triple(struct adxl34x *ac, struct axis_triple *axis)
{
    short buf[3];

    ac->read_block(ac->bus, DATAX0, DATAZ1 - DATAX0 + 1,
               (unsigned char *)buf);
    //mutex_lock(&ac->mutex);
    ac->saved.x = (s16) le16_to_cpu(buf[0]);

    ac->saved.y = (s16) le16_to_cpu(buf[1]);

    ac->saved.z = (s16) le16_to_cpu(buf[2]);

    axis->x = (ac->saved.position) ? (ac->saved.y) : (-ac->saved.y);
    axis->y = (ac->saved.position) ? (ac->saved.x) : (ac->saved.x);
    axis->z = (ac->saved.position) ? (-ac->saved.z) : (ac->saved.z);
    //mutex_unlock(&ac->mutex);
}
static void adxl34x_service_ev_fifo(struct adxl34x *ac)
{
    struct adxl34x_platform_data *pdata = &ac->pdata;
    struct axis_triple axis;

    adxl34x_get_triple(ac, &axis);
/*Add sensor self adapt*/
    adxl34_data[0] = axis.x;
    adxl34_data[1] = axis.y;
    adxl34_data[2] = axis.z;
    input_event(ac->input, ac->pdata.ev_type, pdata->ev_code_x,
            scale*(axis.x - ac->swcal.x));
    input_event(ac->input, ac->pdata.ev_type, pdata->ev_code_y,
             scale*(axis.y - ac->swcal.y));
    input_event(ac->input, ac->pdata.ev_type, pdata->ev_code_z,
             scale*(axis.z - ac->swcal.z));
}

/* disable tap fucntion,for it doesn't need */
#if 0
static void adxl34x_report_key_single(struct input_dev *input, int key)
{
    input_report_key(input, key, 1);
    input_sync(input);
    input_report_key(input, key, 0);
}

static void adxl34x_report_key_double(struct input_dev *input, int key)
{
    input_report_key(input, key, 1);
    input_sync(input);
    input_report_key(input, key, 0);
    input_sync(input);
    input_report_key(input, key, 1);
    input_sync(input);
    input_report_key(input, key, 0);
}
#endif
/*add mutex*/
static void adxl34x_work(struct work_struct *work)
{
    struct adxl34x *ac = container_of(work, struct adxl34x, work);
    struct adxl34x_platform_data *pdata = &ac->pdata;
    int int_stat, tap_stat, samples;
    /*
     * ACT_TAP_STATUS should be read before clearing the interrupt
     * Avoid reading ACT_TAP_STATUS in case TAP detection is disabled
     */

    mutex_lock(&ac->mutex);
    if (pdata->tap_axis_control & (TAP_X_EN | TAP_Y_EN | TAP_Z_EN))
        tap_stat = AC_READ(ac, ACT_TAP_STATUS);
    else
        tap_stat = 0;

    int_stat = AC_READ(ac, INT_SOURCE);
#if 0
    if (int_stat & FREE_FALL)
        adxl34x_report_key_single(ac->input, pdata->ev_code_ff);

    if (int_stat & OVERRUN)
        dev_dbg(&ac->bus->dev, "OVERRUN\n");

    if (int_stat & SINGLE_TAP) {
        if (tap_stat & TAP_X_SRC)
            adxl34x_report_key_single(ac->input,
                          pdata->ev_code_tap_x);
        if (tap_stat & TAP_Y_SRC)
            adxl34x_report_key_single(ac->input,
                          pdata->ev_code_tap_y);
        if (tap_stat & TAP_Z_SRC)
            adxl34x_report_key_single(ac->input,
                          pdata->ev_code_tap_z);
    }

    if (int_stat & DOUBLE_TAP) {
        if (tap_stat & TAP_X_SRC)
            adxl34x_report_key_double(ac->input,
                          pdata->ev_code_tap_x);
        if (tap_stat & TAP_Y_SRC)
            adxl34x_report_key_double(ac->input,
                          pdata->ev_code_tap_y);
        if (tap_stat & TAP_Z_SRC)
            adxl34x_report_key_double(ac->input,
                          pdata->ev_code_tap_z);
    }

    if (pdata->ev_code_act_inactivity) {
        if (int_stat & ACTIVITY)
            input_report_key(ac->input,
                     pdata->ev_code_act_inactivity, 1);
        if (int_stat & INACTIVITY)
            input_report_key(ac->input,
                     pdata->ev_code_act_inactivity, 0);
    }
#endif
    if (int_stat & (DATA_READY | WATERMARK)) {

        if (pdata->fifo_mode)
            samples = ENTRIES(AC_READ(ac, FIFO_STATUS)) + 1;
        else
            samples = 1;

        for (; samples > 0; samples--) {
            adxl34x_service_ev_fifo(ac);
        /*
         * To ensure that the FIFO has
         * completely popped, there must be at least 5 us between
         * the end of reading the data registers, signified by the
         * transition to register 0x38 from 0x37 or the CS pin
         * going high, and the start of new reads of the FIFO or
         * reading the FIFO_STATUS register. For SPI operation at
         * 1.5 MHz or lower, the register addressing portion of the
         * transmission is sufficient delay to ensure the FIFO has
         * completely popped. It is necessary for SPI operation
         * greater than 1.5 MHz to de-assert the CS pin to ensure a
         * total of 5 us, which is at most 3.4 us at 5 MHz
         * operation.
         */
            if (ac->fifo_delay && (samples > 1))
                udelay(3);
        }
    }

    input_sync(ac->input);
    enable_irq(ac->bus->irq);
    mutex_unlock(&ac->mutex);
}

static irqreturn_t adxl34x_irq(int irq, void *handle)
{
    struct adxl34x *ac = handle;
    //printk("*************adxl34x_irq--------enter\n");
    disable_irq_nosync(irq);
    schedule_work(&ac->work);

    return IRQ_HANDLED;
}

static void adxl34x_disable(struct adxl34x *ac)
{
    short buf[3];
    mutex_lock(&ac->mutex);
    if (!ac->disabled && ac->opened) {
        ac->disabled = 1;
/*  Reason: Modify for dead lock  */
/* must unmask interrupt before cancel work or it will cause unstable */
        AC_WRITE(ac, INT_ENABLE, 0);
        mutex_unlock(&ac->mutex);
        /* must wait for work done */
        flush_work(&ac->work);
        mutex_lock(&ac->mutex);
        /*
         * A '0' places the ADXL34x into standby mode
         * with minimum power consumption.
         */
/* make sure interrupt is cleared */
        AC_READ(ac, INT_SOURCE);
        ac->read_block(ac->bus, DATAX0, DATAZ1 - DATAX0 + 1,
               (unsigned char *)buf);
        AC_WRITE(ac, POWER_CTL, 0);
    }

    mutex_unlock(&ac->mutex);
}

static void adxl34x_enable(struct adxl34x *ac)
{
    mutex_lock(&ac->mutex);
    if (ac->disabled && ac->opened) {
        AC_WRITE(ac, POWER_CTL, PCTL_MEASURE);
        AC_WRITE(ac, INT_ENABLE, ac->int_mask /*| OVERRUN*/);
        //AC_WRITE(ac, POWER_CTL, PCTL_MEASURE);
        ac->disabled = 0;
    }
    mutex_unlock(&ac->mutex);
}
//modify name
//static ssize_t adxl34x_disable_show(struct device *dev,
static ssize_t adxl34x_enable_show(struct device *dev,
                    struct device_attribute *attr, char *buf)
{
    struct adxl34x *ac = dev_get_drvdata(dev);

    return sprintf(buf, "%u\n", ac->disabled);
}
//static ssize_t adxl34x_disable_store(struct device *dev,
static ssize_t adxl34x_enable_store(struct device *dev,
                     struct device_attribute *attr,
                     const char *buf, size_t count)
{
    struct adxl34x *ac = dev_get_drvdata(dev);
    unsigned long val;
    int error;

    error = strict_strtoul(buf, 10, &val);
    if (error)
        return error;

    if (val)
        //adxl34x_disable(ac);
        adxl34x_enable(ac);
    else
        //adxl34x_enable(ac);
        adxl34x_disable(ac);


    return count;
}

//static DEVICE_ATTR(disable, 0664, adxl34x_disable_show, adxl34x_disable_store);
static DEVICE_ATTR(enable, 0664, adxl34x_enable_show, adxl34x_enable_store);

static ssize_t adxl34x_calibrate_show(struct device *dev,
                      struct device_attribute *attr, char *buf)
{
    struct adxl34x *ac = dev_get_drvdata(dev);
    ssize_t count;
	short x,y,z;
	struct axis_triple axis;
	adxl34x_get_triple(ac, &axis);
	mutex_lock(&ac->mutex);
	x = axis.y*scale*(-1);
	y = axis.x*scale;
	z = axis.z*scale;
	mutex_unlock(&ac->mutex);
	if(z>=0)
		z -= 1024;
	else
		z +=1024;
	count = sprintf(buf, "x = %d mg, y = %d mg, z = %d mg\n", x,y,z);
    return count;
}

static ssize_t adxl34x_calibrate_store(struct device *dev,
                       struct device_attribute *attr,
                       const char *buf, size_t count)
{
    struct adxl34x *ac = dev_get_drvdata(dev);
	unsigned long flag;
	int err = strict_strtoul(buf, 10, &flag);
	if(err)
	{
		return -EINVAL;
	}
	if((flag != 0)&&(flag != 1))
	{
		return -EINVAL;
	}
	else
	{
		if(flag == 1)
		{
    /*
     * Hardware offset calibration has a resolution of 15.6 mg/LSB.
     * We use HW calibration and handle the remaining bits in SW. (4mg/LSB)
     */

			mutex_lock(&ac->mutex);
			ac->hwcal.x = (-1)*(ac->saved.x / 4);
			ac->hwcal.y = (-1)*(ac->saved.y / 4);
			if(ac->saved.z >= 0)
				ac->hwcal.z = (-1)*((ac->saved.z - 256) / 4);
			else
				ac->hwcal.z = (-1)*((ac->saved.z + 256) / 4);
			AC_WRITE(ac, OFSX, (s8) ac->hwcal.x);
			AC_WRITE(ac, OFSY, (s8) ac->hwcal.y);
			AC_WRITE(ac, OFSZ, (s8) ac->hwcal.z);
			mutex_unlock(&ac->mutex);
		}
		else
		{
			mutex_lock(&ac->mutex);
			AC_WRITE(ac, OFSX, 0x00);
			AC_WRITE(ac, OFSY, 0x00);
			AC_WRITE(ac, OFSZ, 0x00);
			mutex_unlock(&ac->mutex);
		}
	}
    return count;
}

static DEVICE_ATTR(calibrate, 0664, adxl34x_calibrate_show,
           adxl34x_calibrate_store);

static ssize_t adxl34x_rate_show(struct device *dev,
                 struct device_attribute *attr, char *buf)
{
    struct adxl34x *ac = dev_get_drvdata(dev);
    ssize_t count;

    mutex_lock(&ac->mutex);
    count = sprintf(buf, "%u\n", RATE(ac->pdata.data_rate));
    mutex_unlock(&ac->mutex);

    return count;
}

/* add delay */
static ssize_t adxl34x_rate_store(struct device *dev,
                  struct device_attribute *attr,
                  const char *buf, size_t count)
{
    struct adxl34x *ac = dev_get_drvdata(dev);
    unsigned long val;
    int error,i;

    mutex_lock(&ac->mutex);
    error = strict_strtoul(buf, 10, &val);
    if (error)
        return error;

    /*can't too fast*/
	if(val<80 || val>10000000)
    {
        dev_dbg(dev, "%s:val[%d] is invalid use default 10 ms\n",__func__,(unsigned int)val);
        val = 80;
    }

    val=val*1000;
    i = 0;
    for(i=0;i<ARRAY_SIZE(data_rate_tbl);i++)
    {
        if(data_rate_tbl[i].u_delay>=val)
        {
            break;
        }
    }

    ac->pdata.data_rate = data_rate_tbl[i-1].bw_rate;
    AC_WRITE(ac, BW_RATE, ac->pdata.data_rate |
         (ac->pdata.low_power_mode ? LOW_POWER : 0));
    mutex_unlock(&ac->mutex);

    return count;
}

//add  pollrate_ms
static DEVICE_ATTR(pollrate_ms, 0664, adxl34x_rate_show, adxl34x_rate_store);

//static DEVICE_ATTR(rate, 0664, adxl34x_rate_show, adxl34x_rate_store);

static ssize_t adxl34x_autosleep_show(struct device *dev,
                 struct device_attribute *attr, char *buf)
{
    struct adxl34x *ac = dev_get_drvdata(dev);
    ssize_t count;

    mutex_lock(&ac->mutex);
    count = sprintf(buf, "%u\n", ac->pdata.power_mode &
        (PCTL_AUTO_SLEEP | PCTL_LINK) ? 1 : 0);
    mutex_unlock(&ac->mutex);

    return count;
}

static ssize_t adxl34x_autosleep_store(struct device *dev,
                  struct device_attribute *attr,
                  const char *buf, size_t count)
{
    struct adxl34x *ac = dev_get_drvdata(dev);
    unsigned long val;
    int error;

    mutex_lock(&ac->mutex);
    error = strict_strtoul(buf, 10, &val);
    if (error)
        return error;

    if (val)
        ac->pdata.power_mode |= (PCTL_AUTO_SLEEP | PCTL_LINK);
    else
        ac->pdata.power_mode &= ~(PCTL_AUTO_SLEEP | PCTL_LINK);

    if (!ac->disabled && ac->opened)
        AC_WRITE(ac, POWER_CTL, ac->pdata.power_mode | PCTL_MEASURE);

    mutex_unlock(&ac->mutex);

    return count;
}

static DEVICE_ATTR(autosleep, 0664, adxl34x_autosleep_show,
           adxl34x_autosleep_store);

static ssize_t adxl34_get_accl_data(struct device *dev,
                    struct device_attribute *attr,    char *buf)
{
    *((int16_t *)&buf[0]) = adxl34_data[0];
    *((int16_t *)&buf[2]) = adxl34_data[1];
    *((int16_t *)&buf[4]) = adxl34_data[2];
    //printk("buf1=%d,buf2=%d,buf3=%d", buf[0],buf[1],buf[2]);
    return ADXL34_DATA_SIZE;
}

static DEVICE_ATTR(accl_data, 0666, adxl34_get_accl_data, NULL);

#ifdef ADXL_DEBUG
static ssize_t adxl34x_write_store(struct device *dev,
                   struct device_attribute *attr,
                   const char *buf, size_t count)
{
    struct adxl34x *ac = dev_get_drvdata(dev);
    unsigned long val;
    int error;

    /*
     * This allows basic ADXL register write access for debug purposes.
     */
    mutex_lock(&ac->mutex);
    error = strict_strtoul(buf, 16, &val);
    if (error)
        return error;

    AC_WRITE(ac, val >> 8, val & 0xFF);
    mutex_unlock(&ac->mutex);

    return count;
}

static DEVICE_ATTR(write, 0664, NULL, adxl34x_write_store);
#endif

/* add reg dump for debug */
static ssize_t adxl34x_reg_show(struct device *dev,
                   struct device_attribute *attr,
                   char *buf)
{
    struct adxl34x *ac = dev_get_drvdata(dev);
    u8 i,ret;
    u8 val;
    ssize_t count=0;
    mutex_lock(&ac->mutex);
    for(i=0;i<0x3d;i++)
    {
        val = AC_READ(ac, i);
        if(i%4 == 0)
        {
            ret = sprintf(buf,"\n");
            buf += ret;
            count += ret;
        }
        ret = sprintf(buf, "reg[0x%02x] val[0x%02x] \t", i,val);
        buf += ret;
        count += ret;
    }
    mutex_unlock(&ac->mutex);
    return count;
}
static DEVICE_ATTR(reg, 0664, adxl34x_reg_show, NULL);
static ssize_t adxl34x_get_accl_info(struct device *dev,
                 struct device_attribute *attr, char *buf)
{
    ssize_t count;
    count = sprintf(buf, "ADI ADXL346");
    return count;
}
static DEVICE_ATTR(accl_info, 0666, adxl34x_get_accl_info, NULL);
static struct attribute *adxl34x_attributes[] = {
    &dev_attr_enable.attr,
    &dev_attr_calibrate.attr,
    &dev_attr_pollrate_ms.attr,
    &dev_attr_autosleep.attr,
    &dev_attr_accl_data.attr,
    &dev_attr_accl_info.attr,
#ifdef ADXL_DEBUG
    &dev_attr_write.attr,
#endif
    &dev_attr_reg.attr,
    NULL
};

static const struct attribute_group adxl34x_attr_group = {
    .attrs = adxl34x_attributes,
};

static int adxl34x_input_open(struct input_dev *input)
{
    struct adxl34x *ac = input_get_drvdata(input);

    mutex_lock(&ac->mutex);
    ac->opened = 1;
    mutex_unlock(&ac->mutex);

    //adxl34x_enable(ac);

    return 0;
}

static void adxl34x_input_close(struct input_dev *input)
{
    struct adxl34x *ac = input_get_drvdata(input);

    //adxl34x_disable(ac);

    mutex_lock(&ac->mutex);
    ac->opened = 0;
    mutex_unlock(&ac->mutex);
}

static int __devinit adxl34x_initialize(bus_device *bus, struct adxl34x *ac)
{
    struct input_dev *input_dev;
    struct adxl34x_platform_data *devpd = bus->dev.platform_data;
    struct adxl34x_platform_data *pdata;
    int err, range;
    unsigned char revid;

	/*if (!bus->irq) {
		dev_err(&bus->dev, "no IRQ?\n");
		return -ENODEV;
	}*/

    if (!devpd) {
        dev_dbg(&bus->dev,
            "No platfrom data: Using default initialization\n");
        return -ENOMEM;
    }

    memcpy(&ac->pdata, devpd, sizeof(ac->pdata));
    pdata = &ac->pdata;

    input_dev = input_allocate_device();
    if (!input_dev)
        return -ENOMEM;

    ac->input = input_dev;
    ac->disabled = 1;

    INIT_WORK(&ac->work, adxl34x_work);
    mutex_init(&ac->mutex);

    revid = ac->read(bus, DEVID);

    switch (revid) {
    case ID_ADXL345:
        ac->model = 345;
        break;
    case ID_ADXL346:
        ac->model = 346;
        break;
    default:
        dev_err(&bus->dev, "Failed to probe %s\n", input_dev->name);
        err = -ENODEV;
        goto err_free_mem;
    }

    snprintf(ac->phys, sizeof(ac->phys), "%s/input0", dev_name(&bus->dev));

    input_dev->name = "input_accl";
    input_dev->phys = ac->phys;
    input_dev->dev.parent = &bus->dev;
    input_dev->id.product = ac->model;
    input_dev->id.bustype = BUS_I2C;
    input_dev->open = adxl34x_input_open;
    input_dev->close = adxl34x_input_close;

    input_set_drvdata(input_dev, ac);

    __set_bit(ac->pdata.ev_type, input_dev->evbit);

    if (ac->pdata.ev_type == EV_REL) {
        __set_bit(REL_X, input_dev->relbit);
        __set_bit(REL_Y, input_dev->relbit);
        __set_bit(REL_Z, input_dev->relbit);
    } else {
        /* EV_ABS */
        __set_bit(ABS_X, input_dev->absbit);
        __set_bit(ABS_Y, input_dev->absbit);
        __set_bit(ABS_Z, input_dev->absbit);

        if (pdata->data_range & FULL_RES)
            range = ADXL_FULLRES_MAX_VAL;    /* Signed 13-bit */
        else
            range = ADXL_FIXEDRES_MAX_VAL;    /* Signed 10-bit */

        input_set_abs_params(input_dev, ABS_X, -range, range, 3, 3);
        input_set_abs_params(input_dev, ABS_Y, -range, range, 3, 3);
        input_set_abs_params(input_dev, ABS_Z, -range, range, 3, 3);
    }

    __set_bit(EV_KEY, input_dev->evbit);
    __set_bit(pdata->ev_code_tap_x, input_dev->keybit);
    __set_bit(pdata->ev_code_tap_y, input_dev->keybit);
    __set_bit(pdata->ev_code_tap_z, input_dev->keybit);


    if (pdata->ev_code_ff) {
        ac->int_mask = FREE_FALL;
        __set_bit(pdata->ev_code_ff, input_dev->keybit);
    }

    if (pdata->ev_code_act_inactivity)
        __set_bit(pdata->ev_code_act_inactivity, input_dev->keybit);

    ac->int_mask |= ACTIVITY | INACTIVITY;

    if (pdata->watermark) {
        ac->int_mask |= WATERMARK;
        if (!FIFO_MODE(pdata->fifo_mode))
            pdata->fifo_mode |= FIFO_STREAM;
    } else {
        ac->int_mask |= DATA_READY;
    }

    if (pdata->tap_axis_control & (TAP_X_EN | TAP_Y_EN | TAP_Z_EN))
        ac->int_mask |= SINGLE_TAP | DOUBLE_TAP;

    if (FIFO_MODE(pdata->fifo_mode) == FIFO_BYPASS)
        ac->fifo_delay = 0;

    ac->write(bus, POWER_CTL, 0);

    if(pdata->gpio_config_interrupt)
    {
		bus->irq = pdata->gpio_config_interrupt(1);
        if(bus->irq<0)
        {
            dev_err(&bus->dev, "irq gpio reguest failed\n");
            goto err_irq_config;
        }
	}
    else
    {
        dev_err(&bus->dev, "no irq config func\n");
        goto err_irq_config;
    }
    err = request_irq(bus->irq, adxl34x_irq,
              IRQF_TRIGGER_HIGH, bus->dev.driver->name, ac);

    if (err) {
        dev_err(&bus->dev, "irq %d busy?\n", bus->irq);
        goto err_irq_config;
    }
    err = sysfs_create_group(&bus->dev.kobj, &adxl34x_attr_group);
    if (err)
        goto err_free_irq;

    err = input_register_device(input_dev);
    if (err)
        goto err_remove_attr;

    AC_WRITE(ac, THRESH_TAP, pdata->tap_threshold);
    AC_WRITE(ac, OFSX, pdata->x_axis_offset);
    ac->hwcal.x = pdata->x_axis_offset;
    AC_WRITE(ac, OFSY, pdata->y_axis_offset);
    ac->hwcal.y = pdata->y_axis_offset;
    AC_WRITE(ac, OFSZ, pdata->z_axis_offset);
    ac->hwcal.z = pdata->z_axis_offset;
    AC_WRITE(ac, THRESH_TAP, pdata->tap_threshold);
    AC_WRITE(ac, DUR, pdata->tap_duration);
    AC_WRITE(ac, LATENT, pdata->tap_latency);
    AC_WRITE(ac, WINDOW, pdata->tap_window);
    AC_WRITE(ac, THRESH_ACT, pdata->activity_threshold);
    AC_WRITE(ac, THRESH_INACT, pdata->inactivity_threshold);
    AC_WRITE(ac, TIME_INACT, pdata->inactivity_time);
    AC_WRITE(ac, THRESH_FF, pdata->free_fall_threshold);
    AC_WRITE(ac, TIME_FF, pdata->free_fall_time);
    AC_WRITE(ac, TAP_AXES, pdata->tap_axis_control);
    AC_WRITE(ac, ACT_INACT_CTL, pdata->act_axis_control);
    AC_WRITE(ac, BW_RATE, RATE(ac->pdata.data_rate) |
         (pdata->low_power_mode ? LOW_POWER : 0));
    AC_WRITE(ac, DATA_FORMAT, pdata->data_range);
    AC_WRITE(ac, FIFO_CTL, FIFO_MODE(pdata->fifo_mode) |
         SAMPLES(pdata->watermark));
    AC_WRITE(ac, INT_MAP, 0);    /* Map all INTs to INT1 */
    AC_WRITE(ac, INT_ENABLE, 0);
//del
    //pdata->power_mode &= (PCTL_AUTO_SLEEP | PCTL_LINK);
    pdata->power_mode &=  PCTL_LINK;

	err = set_sensor_input(ACC, input_dev->dev.kobj.name);
	if (err) {
		dev_err(&bus->dev, "%s set_sensor_input failed\n", __func__);
		goto err_set_input;
	}

    dev_info(&bus->dev, "ADXL%d accelerometer, irq %d\n",
         ac->model, bus->irq);

    return 0;

 err_set_input:
    input_unregister_device(input_dev);
    input_free_device(input_dev);
 err_remove_attr:
    sysfs_remove_group(&bus->dev.kobj, &adxl34x_attr_group);
 err_free_irq:
    free_irq(bus->irq, ac);
 err_irq_config:
    if(pdata->gpio_config_interrupt)
	{
        pdata->gpio_config_interrupt(0);
    }
 err_free_mem:
    input_free_device(input_dev);
        if(devpd->power_off)
                devpd->power_off();
    return err;
}

static int __devexit adxl34x_cleanup(bus_device *bus, struct adxl34x *ac)
{
    adxl34x_disable(ac);
    sysfs_remove_group(&ac->bus->dev.kobj, &adxl34x_attr_group);
        free_irq(ac->bus->irq, ac);
    input_unregister_device(ac->input);
    dev_dbg(&bus->dev, "unregistered accelerometer\n");

    return 0;
}

#ifdef CONFIG_PM
static int adxl34x_suspend(bus_device *bus, pm_message_t message)
{
    struct adxl34x *ac = dev_get_drvdata(&bus->dev);

    adxl34x_disable(ac);

    return 0;
}

static int adxl34x_resume(bus_device *bus)
{
    struct adxl34x *ac = dev_get_drvdata(&bus->dev);

    adxl34x_enable(ac);

    return 0;
}
#else
#define adxl34x_suspend NULL
#define adxl34x_resume  NULL
#endif
/* add early suspend */
#ifdef CONFIG_HAS_EARLYSUSPEND
static void adxl34x_early_suspend(struct early_suspend *h)
{
    struct adxl34x *ac;
    ac = container_of(h, struct adxl34x, early_suspend);
    adxl34x_suspend(ac->bus, PMSG_SUSPEND);
}

static void adxl34x_late_resume(struct early_suspend *h)
{
    struct adxl34x *ac;
    ac = container_of(h, struct adxl34x, early_suspend);
    adxl34x_resume(ac->bus);
}
#endif

#if defined(CONFIG_INPUT_ADXL34X_SPI)

#define MAX_SPI_FREQ_HZ        5000000
#define MAX_FREQ_NO_FIFODELAY    1500000
#define ADXL34X_CMD_MULTB    (1 << 6)
#define ADXL34X_CMD_READ    (1 << 7)
#define ADXL34X_WRITECMD(reg)    (reg & 0x3F)
#define ADXL34X_READCMD(reg)    (ADXL34X_CMD_READ | (reg & 0x3F))
#define ADXL34X_READMB_CMD(reg) (ADXL34X_CMD_READ | ADXL34X_CMD_MULTB \
                    | (reg & 0x3F))

static int adxl34x_spi_read(struct spi_device *spi, unsigned char reg)
{
    unsigned char cmd;

    cmd = ADXL34X_READCMD(reg);

    return spi_w8r8(spi, cmd);
}

static int adxl34x_spi_write(struct spi_device *spi,
                 unsigned char reg, unsigned char val)
{
    unsigned char buf[2];

    buf[0] = ADXL34X_WRITECMD(reg);
    buf[1] = val;

    return spi_write(spi, buf, sizeof(buf));
}

static int adxl34x_spi_read_block(struct spi_device *spi,
                  unsigned char reg, int count,
                  unsigned char *buf)
{
    ssize_t status;

    reg = ADXL34X_READMB_CMD(reg);
    status = spi_write_then_read(spi, &reg, 1, buf, count);

    return (status < 0) ? status : 0;
}

static int __devinit adxl34x_spi_probe(struct spi_device *spi)
{
    struct adxl34x *ac;
    int error;

    /* don't exceed max specified SPI CLK frequency */
    if (spi->max_speed_hz > MAX_SPI_FREQ_HZ) {
        dev_err(&spi->dev, "SPI CLK %d Hz?\n", spi->max_speed_hz);
        return -EINVAL;
    }

    ac = kzalloc(sizeof(struct adxl34x), GFP_KERNEL);
    if (!ac)
        return -ENOMEM;

    dev_set_drvdata(&spi->dev, ac);
    ac->bus = spi;

    ac->read = adxl34x_spi_read;
    ac->read_block = adxl34x_spi_read_block;
    ac->write = adxl34x_spi_write;

    if (spi->max_speed_hz > MAX_FREQ_NO_FIFODELAY)
        ac->fifo_delay = 1;

    error = adxl34x_initialize(spi, ac);
    if (error) {
        dev_set_drvdata(&spi->dev, NULL);
        kfree(ac);
    }

    return 0;
}

static int __devexit adxl34x_spi_remove(struct spi_device *spi)
{
    struct adxl34x *ac = dev_get_drvdata(&spi->dev);

    adxl34x_cleanup(spi, ac);
    dev_set_drvdata(&spi->dev, NULL);
    kfree(ac);

    return 0;
}

static struct spi_driver adxl34x_driver = {
    .driver = {
        .name = "adxl34x",
        .bus = &spi_bus_type,
        .owner = THIS_MODULE,
    },
    .probe   = adxl34x_spi_probe,
    .remove  = __devexit_p(adxl34x_spi_remove),
    .suspend = adxl34x_suspend,
    .resume  = adxl34x_resume,
};

static int __init adxl34x_spi_init(void)
{
    return spi_register_driver(&adxl34x_driver);
}

module_init(adxl34x_spi_init);

static void __exit adxl34x_spi_exit(void)
{
    spi_unregister_driver(&adxl34x_driver);
}

module_exit(adxl34x_spi_exit);

#elif defined(CONFIG_INPUT_ADXL34X_I2C)

static int adxl34x_i2c_smbus_read(struct i2c_client *client, unsigned char reg)
{
    return i2c_smbus_read_byte_data(client, reg);
}

static int adxl34x_i2c_smbus_write(struct i2c_client *client,
                   unsigned char reg, unsigned char val)
{
    return i2c_smbus_write_byte_data(client, reg, val);
}

static int adxl34x_i2c_smbus_read_block_data(struct i2c_client *client,
                         unsigned char reg, int count,
                         unsigned char *buf)
{
    return i2c_smbus_read_i2c_block_data(client, reg, count, buf);
}

static int adxl34x_i2c_master_read_block_data(struct i2c_client *client,
                          unsigned char reg, int count,
                          unsigned char *buf)
{
    int ret;

    ret = i2c_master_send(client, &reg, 1);
    if (ret < 0)
        return ret;
    ret = i2c_master_recv(client, buf, count);
    if (ret < 0)
        return ret;
    if (ret != count)
        return -EIO;

    return 0;
}

/* add early suspend and optmize error handle*/
static int __devinit adxl34x_i2c_probe(struct i2c_client *client,
                       const struct i2c_device_id *id)
{
    struct adxl34x *ac;
    int error;
        struct adxl34x_platform_data *devpd = client->dev.platform_data;

      if (!devpd) {
        dev_dbg(&client->dev,
            "No platfrom data: Using default initialization\n");
        error = -EINVAL;
        goto get_platdata_error;
    }
      if(devpd->power_on)
      {
              error = devpd->power_on(&client->dev);
          if(error < 0){
            dev_err(&client->dev,"adi_acc_probe: get power fail!\n");
        return error;
          }
      }
    error = i2c_check_functionality(client->adapter,
            I2C_FUNC_SMBUS_BYTE_DATA);
    if (!error) {
        dev_err(&client->dev, "SMBUS Byte Data not Supported\n");
        error = -ENODEV;
        goto i2c_detect_error;
    }

    ac = kzalloc(sizeof(struct adxl34x), GFP_KERNEL);
    if (!ac)
    {
        dev_err(&client->dev, "alloc adxl34x failed\n");
        error = -ENOMEM;
        goto alloc_mem_error;
    }

    i2c_set_clientdata(client, ac);
    ac->bus = client;

    if (i2c_check_functionality(client->adapter,
                    I2C_FUNC_SMBUS_READ_I2C_BLOCK))
        ac->read_block = adxl34x_i2c_smbus_read_block_data;
    else
        ac->read_block = adxl34x_i2c_master_read_block_data;

    ac->read = adxl34x_i2c_smbus_read;
    ac->write = adxl34x_i2c_smbus_write;

    error = adxl34x_initialize(client, ac);
    if (error) {
        dev_err(&client->dev, "initialize failed\n");
        error = -ENODEV;
        goto init_error;
    }
	if(is_gsensor_layout_top())
		ac->saved.position = GS_POSITION_TOP;
	else
		ac->saved.position= GS_POSITION_BOTTOM;
#ifdef CONFIG_HAS_EARLYSUSPEND
    ac->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    ac->early_suspend.suspend = adxl34x_early_suspend;
    ac->early_suspend.resume = adxl34x_late_resume;
    register_early_suspend(&ac->early_suspend);
#endif
    return 0;
init_error:
        i2c_set_clientdata(client, NULL);
        kfree(ac);
alloc_mem_error:
i2c_detect_error:
    if(devpd->power_off)
    {
        devpd->power_off();
    }
get_platdata_error:

    return error;
}

static int __devexit adxl34x_i2c_remove(struct i2c_client *client)
{
    struct adxl34x *ac = dev_get_drvdata(&client->dev);
        struct adxl34x_platform_data *devpd = client->dev.platform_data;
    adxl34x_cleanup(client, ac);
    if(devpd->power_off)
                devpd->power_off();
    i2c_set_clientdata(client, NULL);
    kfree(ac);

    return 0;
}

static const struct i2c_device_id adxl34x_id[] = {
    { "adxl34x", 0 },
    { }
};

MODULE_DEVICE_TABLE(i2c, adxl34x_id);

static struct i2c_driver adxl34x_driver = {
    .driver = {
        .name = "adxl34x",
        .owner = THIS_MODULE,
    },
    .probe    = adxl34x_i2c_probe,
    .remove   = __devexit_p(adxl34x_i2c_remove),
/* add early suspend */
#ifndef CONFIG_HAS_EARLYSUSPEND
    .suspend  = adxl34x_suspend,
    .resume   = adxl34x_resume,
#endif
    .id_table = adxl34x_id,
};

static int __init adxl34x_i2c_init(void)
{
    return i2c_add_driver(&adxl34x_driver);
    //return 0;
}

module_init(adxl34x_i2c_init);

static void __exit adxl34x_i2c_exit(void)
{
    i2c_del_driver(&adxl34x_driver);
}

module_exit(adxl34x_i2c_exit);

#endif

MODULE_AUTHOR("Michael Hennerich <hennerich@blackfin.uclinux.org>");
MODULE_DESCRIPTION("ADXL345/346 Three-Axis Digital Accelerometer Driver");
MODULE_LICENSE("GPL");
