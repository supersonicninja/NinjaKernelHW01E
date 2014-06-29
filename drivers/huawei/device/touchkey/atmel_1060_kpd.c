/*
 *  atmel_1060_kpd.c - Atmel AT42QT1060 Touch Sense Controller
 *
 *  Copyright (C) 2009 Raphael Derosso Pereira <raphaelpereira@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* modify driver from omap44x0 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/earlysuspend.h>
#include <asm/gpio.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/leds.h>
#include "touchkey_platform_config.h"
/* Add timer to check temprature, if too cold to work, calibrate TK device*/
#include <linux/hrtimer.h>
#include <linux/i2c/bq275x0_battery.h>

/* Add PM89212 SMPS4 control begin */
extern int pm8921_set_smps4_pwm(void);
extern int pm8921_set_smps4_tcxo_en(void);
/* Add PM89212 SMPS4 control end */

#define QT1060_VALID1_CHIPID  49
#define QT1060_VALID2_CHIPID  63
#define QT1060_CMD_CHIPID     0
#define QT1060_CMD_VERSION    1
#define QT1060_CMD_SUBVER	2
#define QT1060_CMD_DETCTION_STATUS     4
#define QT1060_CMD_INPUT_PORT_STATUS   5
#define QT1060_CMD_CALIBRATE     12
#define QT1060_CMD_RESET 13
#define QT1060_CMD_DRIFT    14
#define QT1060_CMD_NTHR_KEY0     16
#define QT1060_CMD_NTHR_KEY1     17
#define QT1060_CMD_NTHR_KEY2     18
#define QT1060_CMD_LP_MODE      22
#define QT1060_CMD_IO_MASK      23
#define QT1060_CMD_KEY_MASK      24
#define QT1060_CMD_AKS_MASK      25
#define QT1060_CMD_PWM_MASK      26
#define QT1060_CMD_DETECTION_MASK      27
#define QT1060_CMD_ACTIVE_LEVEL      28
#define QT1060_CMD_USER_OUTPUT_BUFFER      29
#define QT1060_CMD_DETECTION_INTERGRATOR  30
#define QT1060_CMD_PWM_LEVEL      31
#define QT1060_CYCLE_INTERVAL	(2*HZ)
#define QT1060_DEFAULT_SENSITIVITY 0x07
#define QT1060_LP_VALUE  0x01
#define QT1060_CAL_VALUE  0x01
#define KEYNUMBER 3
static int atmel_debug_mask = 0;
static bool flag_tk_status = false;
//willow add
extern atomic_t touch_is_pressed;
extern u32 time_check;
module_param_named(debug_mask, atmel_debug_mask, int,
		S_IRUGO | S_IWUSR | S_IWGRP);

#define ATMEL_1060_DBG(x...) do {\
	if (atmel_debug_mask) \
		printk(KERN_DEBUG x);\
	} while (0)
/* delete one line for moving irq config to devices-msm.c */
/* delete one line for moving irq config to devices-msm.c */
struct qt1060_data {
	struct i2c_client *client;
	struct input_dev *input;
	struct workqueue_struct *atmel_wq;
	struct work_struct work;
/* Add timer to check temprature, if too cold to work, calibrate TK device*/
	struct work_struct temp_work;
	int temp;
	int enable;
	unsigned short *keycodes;
	int keynum;
	int key_matrix;
	u8 sensitive;
	struct early_suspend early_suspend;
	struct hrtimer timer;
};

static int  qt1060_read(struct i2c_client *client, u8 reg)
{
	int ret;
	msleep(1);
	ret = i2c_smbus_write_byte(client, reg);
	if (ret) {
		dev_err(&client->dev,
			"couldn't send request. Returned %d\n", ret);
		return ret;
	}
	msleep(1);
	ret = i2c_smbus_read_byte(client);
	if (ret < 0) {
		dev_err(&client->dev,
			"couldn't read register. Returned %d\n", ret);
		return ret;
	}
	return ret;
}

static int  qt1060_write(struct i2c_client *client, u8 reg, u8 data)
{
	int error;
	msleep(1);
	error = i2c_smbus_write_byte_data(client, reg, data);
	if (error) {
		dev_err(&client->dev,
			"couldn't send request. Returned %d\n", error);
		return error;
	}
	return error;
}

//for controlling LED linght
struct qt1060_data*  qt1060copy;
static void all_keys_up(struct qt1060_data *devdata)
{
	int i;

	for (i = 0; i < devdata->keynum; i++)
		input_report_key(devdata->input,
						devdata->keycodes[i], 0);

	input_sync(devdata->input);
}
static int atmel_touchkey_resume(struct i2c_client *client)
{
	struct qt1060_data *devdata = i2c_get_clientdata(client);
	int mode,i,temp;

	/* Add for set S4 as PWM mode begin */
	pm8921_set_smps4_pwm();
	/* Add for set S4 as PWM mode end */
    
/* Add timer to check temprature, if too cold to work, calibrate TK device*/
	mode = qt1060_write(devdata->client, QT1060_CMD_CALIBRATE, 1);
	if (mode)
	{
		dev_err(&devdata->client->dev, "failed to calibrate device\n");
	}
	/*reset the chip*/
	for(i = 0; i < 10; i++)
	{
		mode = qt1060_write(client, QT1060_CMD_LP_MODE, QT1060_LP_VALUE);
		if (mode)
			dev_err(&client->dev, "atmel_qt1060 failed to resume device\n");
		temp = qt1060_read(client, QT1060_CMD_LP_MODE);
		if (temp == QT1060_LP_VALUE)
			break;
	}
	if (temp != QT1060_LP_VALUE)
	{
		dev_err(&client->dev, "atmel_qt1060 failed to active the chip!\n");
	}
/* Add timer to check temprature, if too cold to work, calibrate TK device*/
	devdata->enable = 1;
	enable_irq(devdata->client->irq);
	hrtimer_start(&devdata->timer, ktime_set(60, 0), HRTIMER_MODE_REL);
	ATMEL_1060_DBG("qt1060_model has resume\n");
	return 0;
}
static int atmel_touchkey_suspend(struct i2c_client *client)
{
	struct qt1060_data *devdata = i2c_get_clientdata(client);
	int mode;
	disable_irq(devdata->client->irq);
	all_keys_up(devdata);
/* Add timer to check temprature, if too cold to work, calibrate TK device*/
	hrtimer_cancel(&devdata->timer);
	cancel_work_sync(&devdata->temp_work);
	mode = cancel_work_sync(&devdata->work);
	if (mode)
		enable_irq(client->irq);
	mode = qt1060_read(devdata->client, QT1060_CMD_DETCTION_STATUS);
	mode = qt1060_write(devdata->client, QT1060_CMD_USER_OUTPUT_BUFFER, 0x00);
	if (mode)
		dev_err(&devdata->client->dev, "atmel_qt1060 failed to mode device\n");
	mode = qt1060_write(devdata->client,QT1060_CMD_LP_MODE, 0);
	if (mode)
	{
		dev_err(&devdata->client->dev, "atmel_qt1060 failed to active user buffer\n");
	}
	devdata->enable = 0;
	ATMEL_1060_DBG("qt1060_model has suspend\n");

	/* Add for set S4 as TCXO_EN mode begin */
	pm8921_set_smps4_tcxo_en();
	/* Add for set S4 as TCXO_EN mode end */
    
	return 0;
}
#ifdef CONFIG_HAS_EARLYSUSPEND
static void atmel_touchkey_early_suspend(struct early_suspend *h)
{
	struct qt1060_data *devdata =
		container_of(h, struct qt1060_data, early_suspend);
	atmel_touchkey_suspend(devdata->client);
}
static void atmel_touchkey_later_resume(struct early_suspend *h)
{
	struct qt1060_data *devdata =
		container_of(h, struct qt1060_data, early_suspend);
	atmel_touchkey_resume(devdata->client);
}
#endif

static int qt1060_get_key_matrix(struct qt1060_data *qt1060)
{
	int i,status = 0, old_matrix = 0, new_matrix = 0,mask,keyval;
	u32 t,check;
	dev_dbg(&qt1060->client->dev, "requesting keys...\n");
	/*
	 * Read all registers from General Status Register
	 * to GPIOs register
	 */
	status = qt1060_read(qt1060->client, QT1060_CMD_DETCTION_STATUS);
/*Add error disposal for"home" Key report persistently when read status error*/
	if(status < 0)
	{
		enable_irq(qt1060->client->irq);
		return 0;
	}
	t = (u32) ktime_to_ms(ktime_get());
	check = t - time_check;
	if(((atomic_read(&touch_is_pressed) == 0 )&& (check > 120))||flag_tk_status)
	{
		old_matrix = qt1060->key_matrix;
		new_matrix = status;
		qt1060->key_matrix = new_matrix;
		mask = 1;
		for (i = 0; i < qt1060->keynum; ++i, mask <<= 1)
		{
			keyval = new_matrix & mask;
			if ((old_matrix & mask) != keyval)
			{
				input_report_key(qt1060->input, qt1060->keycodes[i], keyval);
				ATMEL_1060_DBG("key %d keycode%d %s\n",
					i, qt1060->keycodes[i], keyval ? "pressed" : "released");
				flag_tk_status = (bool)keyval;
				break;
			}
		}
		input_sync(qt1060->input);
	}
	enable_irq(qt1060->client->irq);
	return 0;
}

static irqreturn_t qt1060_irq(int irq, void *_qt1060)
{
	struct qt1060_data *qt1060 = _qt1060;
	disable_irq_nosync(qt1060->client->irq);
	queue_work(qt1060->atmel_wq, &qt1060->work);
	return IRQ_HANDLED;
}

static void atmel_tpk_work_func(struct work_struct *work)
{
	struct qt1060_data *qt1060 =
		container_of(work, struct qt1060_data, work);
	qt1060_get_key_matrix(qt1060);
}

/* Add timer to check temprature, if too cold to work, calibrate TK device*/
static void temp_work_func(struct work_struct *work)
{
	struct qt1060_data *qt1060 = container_of(work,
					struct qt1060_data,temp_work);
	int cal = 0, new_temp = 0, delta = 0;
	if(qt1060->enable)
	{
		new_temp = bq275x0_battery_temperature_wrap();
		delta = (new_temp >= qt1060->temp)?(new_temp - qt1060->temp):(qt1060->temp - new_temp);
		if( delta >= 50)
		{
			cal = qt1060_write(qt1060->client, QT1060_CMD_CALIBRATE, 1);
			if (cal)
			{
				dev_err(&qt1060->client->dev, "failed to calibrate device\n");
			}
		}
		qt1060->temp = new_temp;
	}
}
static enum hrtimer_restart temp_timer_func(struct hrtimer *timer)
{
	struct qt1060_data *qt1060 = container_of(timer, \
					struct qt1060_data, timer);
	queue_work(qt1060->atmel_wq, &qt1060->temp_work);
	hrtimer_start(&qt1060->timer, ktime_set(60, 0), HRTIMER_MODE_REL);
	return HRTIMER_NORESTART;
}

static bool qt1060_identify(struct i2c_client *client)
{
	int id, ver, rev;

	/* Read Chid ID to check if chip is valid */
	id = qt1060_read(client, QT1060_CMD_CHIPID);
	if (!((id == QT1060_VALID1_CHIPID)||(id == QT1060_VALID2_CHIPID))) {
		dev_err(&client->dev, "ID %d not supported\n", id);
		return false;
	}
	/* Read chip version */
	ver = qt1060_read(client, QT1060_CMD_VERSION);
	if (ver < 0) {
		dev_err(&client->dev, "could not get firmware version\n");
		return false;
	}

	/* Read chip firmware revision */
	rev = qt1060_read(client, QT1060_CMD_SUBVER);
	if (rev < 0) {
		dev_err(&client->dev, "could not get firmware revision\n");
		return false;
	}

	dev_info(&client->dev, "AT42QT1060 firmware version %d.%d.%d\n",
			ver >> 4, ver & 0xf, rev);

	return true;
}

static void qt1060_keypad_bl_led_set(struct led_classdev *led_cdev,
	enum led_brightness value)
{
	int ret;
	if (value)
	{
		ret = qt1060_write(qt1060copy->client,QT1060_CMD_USER_OUTPUT_BUFFER, 0x08);
		if (ret)
		{
			dev_err(&qt1060copy->client->dev, "failed to active user buffer\n");
		}
	}
	else
	{
		ret = qt1060_write(qt1060copy->client,QT1060_CMD_USER_OUTPUT_BUFFER, 0x00);
		if (ret)
		{
			dev_err(&qt1060copy->client->dev, "failed to active user buffer\n");
		}
	}
}

static struct led_classdev qt1060_kp_bl_led = {
	.name                   = "button-backlight-tk",
	.brightness_set		= qt1060_keypad_bl_led_set,
	.brightness		= LED_OFF,
};
static int qt1060_set_sensitive(struct qt1060_data *qt,u8 sensitivity)
{
	struct qt1060_data *qt1060 = qt;
	int ret = 0;
	/*set HOME key0 sensitivity*/
/*move out disable_irp and enable_irq*/
	ret = qt1060_write(qt1060->client, QT1060_CMD_NTHR_KEY0, sensitivity);
	if (ret)
	{
		dev_err(&qt1060->client->dev, "failed to write QT1060_CMD_NTHR_KEY0.\n");
		return ret;
	}
	/*set BACK key1 sensitivity*/
	ret = qt1060_write(qt1060->client, QT1060_CMD_NTHR_KEY1, sensitivity);
	if (ret)
	{
		dev_err(&qt1060->client->dev, "failed to write QT1060_CMD_NTHR_KEY1.\n");
		return ret;
	}
	ret = qt1060_write(qt1060->client, QT1060_CMD_NTHR_KEY2, sensitivity);
	if (ret)
	{
		dev_err(&qt1060->client->dev, "failed to write QT1060_CMD_NTHR_KEY0.\n");
		return ret;
	}
	return 0;

}
static int qt1060_set_initconfig(struct i2c_client *client)
{
	int ret, value;
	struct qt1060_data *devdata = i2c_get_clientdata(client);
	/*colse the unsed key*/
	ret = qt1060_write(devdata->client,QT1060_CMD_KEY_MASK, 0x07);
	if (ret)
	{
		dev_err(&devdata->client->dev, "failed to close the unsed key device\n");
		goto err_set_register;
	}
	ret = qt1060_write(devdata->client, QT1060_CMD_DRIFT, 0);
	if (ret)
	{
		dev_err(&devdata->client->dev, "failed to set drift mode!\n");
		goto err_set_register;
	}
	ret = qt1060_write(devdata->client, QT1060_CMD_DETECTION_INTERGRATOR, 0x02);
	if (ret)
	{
		dev_err(&devdata->client->dev, "failed to set detection intergrator!\n");
		goto err_set_register;
	}
	/*set calibrate device*/
	ret = qt1060_write(devdata->client, QT1060_CMD_CALIBRATE, 2);
	if (ret)
	{
		dev_err(&devdata->client->dev, "failed to calibrate device\n");
		goto err_set_register;
	}
	/*set aks mask device*/
	ret = qt1060_write(devdata->client, QT1060_CMD_AKS_MASK , 0x3f);
	if (ret)
	{
		dev_err(&devdata->client->dev, "failed to aks mask device\n");
		goto err_set_register;
	}
	/*clear the detection mask*/
	ret = qt1060_write(devdata->client, QT1060_CMD_DETECTION_MASK , 0x00);
	if (ret)
	{
		dev_err(&devdata->client->dev, "failed to active level device\n");
		goto err_set_register;
	}
	ret = qt1060_write(devdata->client, QT1060_CMD_PWM_LEVEL, 0x80);
	if (ret)
	{
		dev_err(&devdata->client->dev, "failed to io mask device\n");
		goto err_set_register;
	}
	ret = qt1060_write(devdata->client, QT1060_CMD_PWM_MASK , 0x7f);
	if (ret)
	{
		dev_err(&devdata->client->dev, "failed to io mask device\n");
		goto err_set_register;
	}
    /*set the active level*/
	ret = qt1060_write(devdata->client, QT1060_CMD_ACTIVE_LEVEL , 0x7f);
	if (ret)
	{
	     dev_err(&devdata->client->dev, "failed to active level device\n");
	     goto err_set_register;
	}
	/*set the IO as output leave open*/
	ret = qt1060_write(devdata->client, QT1060_CMD_IO_MASK , 0x7f);
	if (ret)
	{
		dev_err(&devdata->client->dev, "failed to io mask device\n");
		goto err_set_register;
	}
	qt1060_set_sensitive(devdata,devdata->sensitive);
	ret = qt1060_write(devdata->client, QT1060_CMD_LP_MODE, QT1060_LP_VALUE);
	if (ret)
	{
		dev_err(&devdata->client->dev, "atmel_qt1060 failed to resume device\n");
		goto err_set_register;
	}
	value = qt1060_read(client, QT1060_CMD_LP_MODE);
	if (value != QT1060_LP_VALUE)
	{
		dev_err(&devdata->client->dev, "atmel_qt1060 failed to active the chip!\n");
		ret = EIO;
		goto err_set_register;
	}
err_set_register:
       return ret;
}

static ssize_t qt1060_reg_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct qt1060_data *qt1060 = dev_get_drvdata(dev);
	u8 i,ret;
	u8 val;
	ssize_t count=0;
	disable_irq(qt1060->client->irq);
	for(i=0;i<64;i++)
	{
		val = qt1060_read(qt1060->client,i);
		 ret = sprintf(buf, "reg[0x%02x] val[0x%02x] \t", i,val);
		 buf += ret;
		 count += ret;
		 if(i%4 == 0)
			ret = sprintf(buf,"\n");
		 buf += ret;
		 count += ret;
	}
	enable_irq(qt1060->client->irq);
	return count;
}

static ssize_t qt1060_reg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct qt1060_data *qt1060 = dev_get_drvdata(dev);
	int regs,value;
	size_t ret;
	if (2 != sscanf(buf, "%x %x", &regs, &value)) {
		dev_err(&qt1060->client->dev, "failed to reg store\n");
		return -EINVAL;
	}
	disable_irq(qt1060->client->irq);
	ret = qt1060_write(qt1060->client,regs, value);
	enable_irq(qt1060->client->irq);
	return ret;
}

static DEVICE_ATTR(reg, 0664, qt1060_reg_show,
		   qt1060_reg_store);

static struct attribute *qt1060_attributes[] = {
	&dev_attr_reg.attr,
	NULL
};

static const struct attribute_group qt1060_attr_group = {
	.attrs = qt1060_attributes,
};

static int __devinit qt1060_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct qt1060_data *qt1060;
	struct touchkey_platform_data *pdata;
	struct input_dev *input;
	int i;
	u8 sensitivity = QT1060_DEFAULT_SENSITIVITY;
	int error;
	/* Check functionality */
	error = i2c_check_functionality(client->adapter,
	I2C_FUNC_SMBUS_BYTE);
	if (!error) 
	{
		dev_err(&client->dev, "%s adapter not supported\n",
		dev_driver_string(&client->adapter->dev));
		error = -ENODEV;
		goto err_check_functionality_failed;

	}
	pdata = client->dev.platform_data;
	/*Add TK reset*/
	if(pdata->touchkey_reset)
	{
		pdata->touchkey_reset();
	}

	if (!qt1060_identify(client))
	{
		printk(KERN_ERR"qt1060_identify error\n");
		return -ENODEV;
	}
	/* Chip is valid and active. Allocate structure */
	qt1060 = kzalloc(sizeof(struct qt1060_data), GFP_KERNEL);
	if (qt1060 == NULL)
	{
		printk(KERN_ERR"%s: allocate atmel_ts_data failed\n", __func__);
		error = -ENOMEM;
		goto err_alloc_data_failed;
	}
	qt1060copy = qt1060;
	qt1060->atmel_wq = create_singlethread_workqueue("atmel_tpk_wq");
	if (!qt1060->atmel_wq) 
	{
		printk(KERN_ERR"%s: create workqueue failed\n", __func__);
		error = -ENOMEM;
		goto err_cread_wq_failed;
	}
	INIT_WORK(&qt1060->work, atmel_tpk_work_func);
/* Add timer to check temprature, if too cold to work, calibrate TK device*/
	INIT_WORK(&qt1060->temp_work,temp_work_func);
	qt1060->client = client;
	i2c_set_clientdata(client, qt1060);
	if(pdata->get_sensitive)
	{
		sensitivity = pdata->get_sensitive();
		if(sensitivity)
			qt1060->sensitive = sensitivity;
		else
			qt1060->sensitive = QT1060_DEFAULT_SENSITIVITY;
	}
	else
	{
		qt1060->sensitive = sensitivity;
	}
	qt1060->keycodes = pdata->get_key_map();
	qt1060->keynum = KEYNUMBER;
	if(pdata->touchkey_gpio_config_interrupt)
	{
		client->irq = pdata->touchkey_gpio_config_interrupt(1);
		if (client->irq < 0) 
		{
			dev_err(&client->dev, "irq gpio reguest failed\n");
			error = -ENODEV;
			goto err_request_gpio_failed;
		}
	}
	else
	{
		dev_err(&client->dev, "no irq config func\n");
		error = -ENODEV;
		goto err_request_gpio_failed;
	}
/* Add timer to check temprature, if too cold to work, calibrate TK device*/
	qt1060->enable = 1;
	error = qt1060_set_initconfig(client);
	if (error)
	{
		dev_err(&client->dev,"Failed to set register value!\n");
		error = -EIO;
		goto err_init_config_failed;
	}
	input = input_allocate_device();
	qt1060->input = input;
	if (!qt1060 || !input)
	{
		dev_err(&client->dev, "insufficient memory\n");
		error = -ENOMEM;
		goto err_input_dev_alloc_failed;
	}
	dev_set_drvdata(&input->dev, qt1060);
	input->name = "qt1060";
	input->id.bustype = BUS_I2C;
	input->keycode = qt1060->keycodes;
	input->keycodesize = sizeof(qt1060->keycodes[0]);
	input->keycodemax = qt1060->keynum;
	__set_bit(EV_KEY, input->evbit);
	__clear_bit(EV_REP, input->evbit);
	for (i = 0; i < qt1060->keynum; i++)
	{
		input_set_capability(qt1060->input, EV_KEY,
		qt1060->keycodes[i]);
	}
	__clear_bit(KEY_RESERVED, input->keybit);
	error = input_register_device(qt1060->input);
	if (error)
	{
		dev_err(&client->dev,"Failed to register input device\n");
		error = -ENOMEM;
		goto err_input_register_device_failed;
	}
	error = request_irq(client->irq, qt1060_irq,
	IRQF_TRIGGER_LOW, "qt1060", qt1060);// IRQF_TRIGGER_LOW // | IRQF_TRIGGER_RISING
	if (error) 
	{
		dev_err(&client->dev,"failed to allocate irq %d\n", client->irq);
		error = EIO;
		goto err_request_irq_failed;
	}
/* Add timer to check temprature, if too cold to work, calibrate TK device*/
	hrtimer_init(&qt1060->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	qt1060->timer.function = temp_timer_func;
	hrtimer_start(&qt1060->timer, ktime_set(90, 0), HRTIMER_MODE_REL);
	error = led_classdev_register(&client->dev, &qt1060_kp_bl_led);
	if (error)
	{
		dev_err(&client->dev, "unable to register led class driver\n");
		error = -ENOMEM;
		goto err_intput_reg;
	}
	qt1060_keypad_bl_led_set(&qt1060_kp_bl_led, LED_OFF);
	error = sysfs_create_group(&client->dev.kobj, &qt1060_attr_group);
	if(error)
	{
		dev_err(&client->dev, "unable to creat attribute\n");
	}
/*move to the end of prob*/
#ifdef CONFIG_HAS_EARLYSUSPEND
	qt1060->early_suspend.suspend = atmel_touchkey_early_suspend;
	qt1060->early_suspend.resume = atmel_touchkey_later_resume;
	register_early_suspend(&qt1060->early_suspend);
#endif
	printk(KERN_DEBUG "atmel-tk is successfully probed");
	return 0;

err_intput_reg:
err_request_irq_failed:
	input_unregister_device(qt1060->input);
err_input_register_device_failed:
	input_free_device(input);
err_input_dev_alloc_failed:
    if(pdata->touchkey_gpio_config_interrupt)
    {
        pdata->touchkey_gpio_config_interrupt(0);
    }
err_init_config_failed:
err_request_gpio_failed:
	destroy_workqueue(qt1060->atmel_wq);
err_cread_wq_failed:
	kfree(qt1060);
err_alloc_data_failed:
err_check_functionality_failed:
	return error;
}
static void qt1060_shutdown(struct i2c_client *client)
{
	struct qt1060_data *qt1060 = i2c_get_clientdata(client);
	qt1060_write(qt1060->client,QT1060_CMD_USER_OUTPUT_BUFFER, 0x00);
}
static int __devexit qt1060_remove(struct i2c_client *client)
{
	struct qt1060_data *qt1060;
	qt1060 = i2c_get_clientdata(client);
	unregister_early_suspend(&qt1060->early_suspend);
	/* Release IRQ so no queue will be scheduled */
	if (client->irq)
		free_irq(client->irq, qt1060);
	all_keys_up(qt1060);
	input_unregister_device(qt1060->input);
	kfree(qt1060);
	qt1060copy = NULL;
	led_classdev_unregister(&qt1060_kp_bl_led);
	sysfs_remove_group(&client->dev.kobj, &qt1060_attr_group);
	return 0;
}

static const struct i2c_device_id qt1060_idtable[] = {
	{ "qt1060", 0, },
	{ }
};

MODULE_DEVICE_TABLE(i2c, qt1060_idtable);

static struct i2c_driver qt1060_driver = {
	.driver = {
		.name	= "qt1060",
		.owner  = THIS_MODULE,
	},

	.id_table	= qt1060_idtable,
	.probe		= qt1060_probe,
	.remove		= __devexit_p(qt1060_remove),
	.shutdown      = qt1060_shutdown,
};

static int __init qt1060_init(void)
{
	return i2c_add_driver(&qt1060_driver);
}
module_init(qt1060_init);

static void __exit qt1060_cleanup(void)
{
	i2c_del_driver(&qt1060_driver);
}
module_exit(qt1060_cleanup);


MODULE_AUTHOR("Raphael Derosso Pereira <raphaelpereira@gmail.com>");
MODULE_DESCRIPTION("Driver for AT42QT1060 Touch Sensor");
MODULE_LICENSE("GPL");
