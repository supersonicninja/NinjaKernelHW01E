/* drivers/input/misc/tmd2771.c
 *
 * Copyright (C) 2011 HUAWEI, Inc.
 * Author: chenlian <chenlian@huawei.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/wakelock.h>
#include "tmd2771.h"

/* creat at 2011.03.22,need to add wake lock */
static int taos_debug_mask = 0;

#define PROX_THRESHOLD_HI_MAX 600
#define PROX_THRESHOLD_LO_MAX 550

module_param_named(debug_mask, taos_debug_mask, int,
        S_IRUGO | S_IWUSR | S_IWGRP);

#define TAOS_DBG(x...) do {\
    if (taos_debug_mask) \
        printk(KERN_DEBUG x);\
    } while (0)
// taos sysfs driver debug switch
#define TAOS_DRIVER_DEBUG
static int light_device_minor = 0;
static int proximity_device_minor = 0;
static struct workqueue_struct *taos_wq;
static struct wake_lock proximity_wake_lock;

#define LSENSOR_MAX_LEVEL 7

static unsigned char als_first_read = 1;
static const u16 luxValues[LSENSOR_MAX_LEVEL] = {
            80, 150, 350,
            800, 1400, 2600, 10240
};
static struct lux_data TritonFN_lux_data[] = {
    { 9830,  8320,  15360 },
    { 12452, 10554, 22797 },
    { 14746, 6234,  11430 },
    { 17695, 3968,  6400  },
    { 0,     0,     0     }
};
static struct lux_data *lux_tablep = TritonFN_lux_data;
//static int lux_history[TAOS_FILTER_DEPTH] = {-ENODATA, -ENODATA, -ENODATA};
u8 taos_triton_gain_table[] = {1, 8, 16, 120};

struct taos_data
{
    struct i2c_client *client;
    struct input_dev *input_dev;
    struct mutex  io_lock;// lock for i2c read & write
    struct mutex  mlock;// lock for access taos_data
    struct hrtimer timer;
    atomic_t timer_delay;
    struct work_struct  work;

    atomic_t open_times;// open times,if > 0,timer is enabled, = 0,timer is diabled
    atomic_t l_flag;    // light enable flag
    atomic_t p_flag;    // proximity enable flag

    struct taos_cfg dev_config;// save current device config

    //int (*power)(int on);
};
static struct taos_data  *this_taos_data;
/* default config */
static struct taos_cfg taos_dev_def_config =
{
    .calibrate_target = 300000,
    .als_time = 100,//uint ms
    .scale_factor = 1,
    .gain_trim = 512,
    .filter_history = 3,
    .filter_count = 1,
    .gain = 1,
    .prox_threshold_hi = PROX_THRESHOLD_HI_MAX,//need calibrate
    .prox_threshold_lo = PROX_THRESHOLD_LO_MAX,
 //   .prox_int_time = 0xFC,//integration time
    .prox_adc_time = 0xFF, //2.72ms
    .prox_wait_time = 0xF2,
    .prox_intr_filter = 0x00,
    .prox_config = 0x00,
/* from 0x08 to 0x03 */
    .prox_pulse_cnt = 0x03,
    .prox_gain = 0x20,
};

// verify device,make sure bufp size is 32
static s32 taos_device_name(const unsigned char *bufp)
{
    /* device id reg */
    if(bufp[0x12]!=0x29)
        return -1;
    return 0;
}

static s32 taos_normal_reg_in(struct taos_data *taos,u8 reg,u8 *val)
{
    s32 ret = 0;
    struct i2c_client *client = taos->client;

    mutex_lock(&taos->io_lock);
    ret = i2c_smbus_write_byte(client, (TAOS_TRITON_CMD_REG | reg));
    if(ret)
    {
        mutex_unlock(&taos->io_lock);
        printk(KERN_ERR "%s: failed to write reg[%d], err=%d\n", __FUNCTION__, reg, ret );
        return ret;
    }

    ret = i2c_smbus_read_byte(client);
    if(0 > ret)
    {
        mutex_unlock(&taos->io_lock);
        printk(KERN_ERR "%s: failed to read reg[%d], err=%d\n", __FUNCTION__, reg, ret );
        return ret;
    }
    mutex_unlock(&taos->io_lock);

    *val = (u8)ret;
    return 0;
}

static s32 taos_normal_reg_out(struct taos_data *taos,u8 reg,u8 val)
{
    s32 ret = 0;
    struct i2c_client *client = taos->client;

    mutex_lock(&taos->io_lock);
    ret = i2c_smbus_write_byte_data(client, (TAOS_TRITON_CMD_REG | reg), val);
    if(ret)
    {
        printk(KERN_ERR "%s: failed to write reg[%d], err=%d\n", __FUNCTION__, reg, ret );
    }
    mutex_unlock(&taos->io_lock);

    return ret;
}

/* used to restore current cfg to taos device */
static s32 taos_device_update_cfg(struct taos_data  *taos)
{
    s32 ret;
    u8 reg = 0;
    struct taos_cfg * cfg;
    u8 itime;

    TAOS_DBG("taos_device_update_cfg enter\n");
    cfg = &taos->dev_config;
    /* stop timer and work first anyway */
    //if(atomic_read(&taos->l_flag) || atomic_read(&taos->p_flag))
    {
        hrtimer_cancel(&taos->timer);
        //cancel_work_sync(&taos->work);
    }

    /* ALS interrupt clear */
    ret = i2c_smbus_write_byte(taos->client, (TAOS_TRITON_CMD_REG|TAOS_TRITON_CMD_SPL_FN|TAOS_TRITON_CMD_ALS_INTCLR));
    if(ret < 0)
    {
        printk(KERN_ERR "%s: ALS interrupt clear failed\n",__FUNCTION__);
        goto resume_timer;
    }

    /* device power down first */
    ret = taos_normal_reg_out(taos, TAOS_TRITON_CNTRL, 0x00);
    if(ret < 0)
    {
        printk(KERN_ERR "%s: device power down failed\n",__FUNCTION__);
        goto resume_timer;
    }

    /* update als time */
    itime = (((taos->dev_config.als_time/50) * 18) - 1);
    itime = (~itime);
    ret = taos_normal_reg_out(taos, TAOS_TRITON_ALS_TIME, itime);
    if(ret < 0)
    {
        printk(KERN_ERR "%s: set als time failed\n",__FUNCTION__);
        goto resume_timer;
    }

    /* update prox time */
    ret = taos_normal_reg_out(taos, TAOS_TRITON_PRX_TIME, cfg->prox_adc_time);
    if(ret < 0)
    {
        printk(KERN_ERR "%s: set prox time failed\n",__FUNCTION__);
        goto resume_timer;
    }

    /* update wait time */
    ret = taos_normal_reg_out(taos, TAOS_TRITON_WAIT_TIME, cfg->prox_wait_time);
    if(ret < 0)
    {
        printk(KERN_ERR "%s: set wait time failed\n",__FUNCTION__);
        goto resume_timer;
    }

    /* update interrupt persistence filter */
    ret = taos_normal_reg_out(taos, TAOS_TRITON_INTERRUPT, cfg->prox_intr_filter);
    if(ret < 0)
    {
        printk(KERN_ERR "%s: set interrupt persistence failed\n",__FUNCTION__);
        goto resume_timer;
    }

    /* update config reg */
    ret = taos_normal_reg_out(taos, TAOS_TRITON_PRX_CFG, cfg->prox_config);
    if(ret < 0)
    {
        printk(KERN_ERR "%s: set config reg failed\n",__FUNCTION__);
        goto resume_timer;
    }

    /* update prox pulse count */
    ret = taos_normal_reg_out(taos, TAOS_TRITON_PRX_COUNT, cfg->prox_pulse_cnt);
    if(ret < 0)
    {
        printk(KERN_ERR "%s: set prox pulse count failed\n",__FUNCTION__);
        goto resume_timer;
    }

    /* update als gain. to be improved */
    ret = taos_normal_reg_out(taos, TAOS_TRITON_GAIN, cfg->prox_gain | cfg->gain);
    if(ret < 0)
    {
        printk(KERN_ERR "%s: set als gain failed\n",__FUNCTION__);
        goto resume_timer;
    }

    /* enable als,prox,wait if p_flag or l_flag are enabled.then power on */
    if(atomic_read(&taos->p_flag) || atomic_read(&taos->l_flag))
        reg = (TAOS_TRITON_CNTL_WAIT_TMR_ENBL | TAOS_TRITON_CNTL_PWRON
                | TAOS_TRITON_CNTL_ADC_ENBL | TAOS_TRITON_CNTL_PROX_DET_ENBL);

    ret = taos_normal_reg_out(taos, TAOS_TRITON_CNTRL, reg);
    if(ret < 0)
    {
        printk(KERN_ERR "%s: device power on failed\n",__FUNCTION__);
        goto resume_timer;
    }

resume_timer:
    /* resume timer */
    if(atomic_read(&taos->l_flag) || atomic_read(&taos->p_flag))
    {
        hrtimer_start(&taos->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
    }

    TAOS_DBG("taos_device_update_cfg out\n");

    return ret;
}

static int taos_prox_get(struct taos_data *taos,struct taos_prox_info *prxp)
{
    int ret;
    static u8 prox_history_hi = 0;
    static u8 prox_history_lo = 0;
    u16 sat_als = 0;
    u16 clear;
    int i = 0;
    u8 chdata[6];
    u8 reg;
    u8 atime;

    /* not need in poll mode */
#if 0
    do{
        ret = taos_normal_reg_in(taos, TAOS_TRITON_STATUS, &status);
        if(ret)
        {
            printk(KERN_ERR "%s: get status reg failed\n",__FUNCTION__);
            return (ret);
        }
    }while(((status & 0x30) != 0x30)&&(wait_count++ < 10))
    if(wait_count >= 10)
    {
        printk(KERN_ERR "%s: poll status reg failed\n",__FUNCTION__);
        return (ret);
    }
#endif

    /* check device is enable or not */
    ret = taos_normal_reg_in(taos, TAOS_TRITON_CNTRL, &reg);
    if(ret < 0)
    {
        printk(KERN_ERR "%s: get ctl reg failed\n",__FUNCTION__);
        return (ret);
    }
    if((reg & (TAOS_TRITON_CNTL_PWRON | TAOS_TRITON_CNTL_ADC_ENBL | TAOS_TRITON_CNTL_PROX_DET_ENBL))!=
        (TAOS_TRITON_CNTL_PWRON | TAOS_TRITON_CNTL_ADC_ENBL | TAOS_TRITON_CNTL_PROX_DET_ENBL))
    {
        printk(KERN_ERR "%s: device is diabled,can't get prox data\n",__FUNCTION__);
        return -ENODATA;
    }

    /* read ch0 & ch1 data */
    for (i = 0; i < 6; i++)
    {
        ret = taos_normal_reg_in(taos, TAOS_TRITON_ALS_CHAN0LO+i, &chdata[i]);
        if(ret)
        {
            printk(KERN_ERR "%s: get status reg failed\n",__FUNCTION__);
            return (ret);
        }
    }

    /* get als time for sat */
    ret = taos_normal_reg_in(taos, TAOS_TRITON_ALS_TIME, &atime);
    if(ret < 0)
    {
        printk(KERN_ERR "%s: get als time failed\n",__FUNCTION__);
        return (ret);
    }
    sat_als = (256 - atime) << 10;
    clear = chdata[1];
    clear <<= 8;
    clear |= chdata[0];
    if (clear > ((sat_als*80)/100))
    {
    	 prxp->prox_event = 1;
         return 0;
    }
    prxp->prox_clear = clear;
    prxp->prox_data = chdata[5];
    prxp->prox_data <<= 8;
    prxp->prox_data |= chdata[4];

    /* just for get a stable status */
    prox_history_hi <<= 1;
    prox_history_hi |= ((prxp->prox_data > taos->dev_config.prox_threshold_hi)? 1:0);
    prox_history_hi &= 0x07;
    prox_history_lo <<= 1;
    prox_history_lo |= ((prxp->prox_data > taos->dev_config.prox_threshold_lo)? 1:0);
    prox_history_lo &= 0x07;
/* event 0 is close  event 1 is far */
    if (prox_history_hi == 0x07)
    {
        /* close */
        prxp->prox_event = 0;
    }
    else if(prox_history_lo == 0)
    {
        /* far */
        prxp->prox_event = 1;
    }
    else
    {
        /* no action */
        prxp->prox_event = -1;
    }

    return 0;
}

//1 to do: modify from taos.c,need improved
static int taos_lux_get(struct taos_data *taos)
{
    u16 raw_clear = 0, raw_ir = 0, raw_lux = 0;
    u32 lux = 0;
    u32 ratio = 0;
    u8 dev_gain = 0;
    struct time_scale_factor TritonTime;
    struct lux_data *p;
    int ret = 0;
    u8 chdata[4];
    int tmp = 0, i = 0;

    for (i = 0; i < 4; i++) {
        ret = taos_normal_reg_in(taos, TAOS_TRITON_ALS_CHAN0LO+i, &chdata[i]);
        if(ret)
        {
            printk(KERN_ERR "%s: get status reg failed\n",__FUNCTION__);
            return (ret);
        }
        //printk("ch(%d),data=%d\n",i,chdata[i]);
    }
    //printk("ch0=%d\n",chdata[0]+chdata[1]*256);
    //printk("ch1=%d\n",chdata[2]+chdata[3]*256);
    tmp = (taos->dev_config.als_time + 25)/50; //if atime =100  tmp = (atime+25)/50=2.5   tine = 2.7*(256-atime)=  412.5
    TritonTime.numerator = 1;
    TritonTime.denominator = tmp;

    tmp = 300 * taos->dev_config.als_time; //tmp = 300*atime  400
    if(tmp > 65535)
        tmp = 65535;
    TritonTime.saturation = tmp;
    raw_clear = chdata[1];
    raw_clear <<= 8;
    raw_clear |= chdata[0];
    raw_ir    = chdata[3];
    raw_ir    <<= 8;
    raw_ir    |= chdata[2];

    if(raw_ir > raw_clear) {
        raw_lux = raw_ir;
        raw_ir = raw_clear;
        raw_clear = raw_lux;
    }
    raw_clear *= taos->dev_config.scale_factor;
    raw_ir *= taos->dev_config.scale_factor;
    dev_gain = taos_triton_gain_table[taos->dev_config.gain & 0x3];
    if(raw_clear >= TritonTime.saturation)
        return(TAOS_MAX_LUX);
    if(raw_ir >= TritonTime.saturation)
        return(TAOS_MAX_LUX);
    if(raw_clear == 0)
        return(0);
    if(dev_gain == 0 || dev_gain > 127) {
        printk(KERN_ERR "TAOS: dev_gain = 0 or > 127 in taos_lux_get()\n");
        return -1;
    }
    if(TritonTime.denominator == 0) {
        printk(KERN_ERR "TAOS: TritonTime.denominator = 0 in taos_lux_get()\n");
        return -1;
    }
    ratio = (raw_ir<<15)/raw_clear;
    for (p = lux_tablep; p->ratio && p->ratio < ratio; p++);
    if(!p->ratio)
        return 0;
    lux = ((raw_clear*(p->clear)) - (raw_ir*(p->ir)));
    lux = ((lux + (TritonTime.denominator >>1))/TritonTime.denominator) * TritonTime.numerator;
	lux = (lux + (dev_gain >> 1))<<1;
    lux >>= TAOS_SCALE_MILLILUX;
    if(lux > TAOS_MAX_LUX)
        lux = TAOS_MAX_LUX;
    return(lux)*taos->dev_config.filter_count;
}

/* get prox threshold
 * ---------min----mean----max--------threshold_lo--------threshold_hi----max
 */
static int taos_prox_calibrate(struct taos_data *taos)
{
    int ret = 0;
    u8 reg;
    int i;
    u16 sat_prox = 0;
    int prox_sum = 0, prox_mean = 0, prox_max = 0;
    struct taos_prox_info prox_cal_info[20];

    TAOS_DBG("taos_prox_calibrate enter\n");

    /* force als prox wait enabled */
    reg = (TAOS_TRITON_CNTL_WAIT_TMR_ENBL | TAOS_TRITON_CNTL_PWRON
                | TAOS_TRITON_CNTL_ADC_ENBL | TAOS_TRITON_CNTL_PROX_DET_ENBL);

    ret = taos_normal_reg_out(taos, TAOS_TRITON_CNTRL, reg);
    if(ret < 0)
    {
        printk(KERN_ERR "%s: device power on failed\n",__FUNCTION__);
        goto restore_cfg;
    }

    /* loop 20 times to get mean value */
    for (i = 0; i < 20; i++)
    {
        if ((ret = taos_prox_get(taos, &prox_cal_info[i])) < 0)
        {
            printk(KERN_ERR "%s: call to taos_prox_get failed\n",__FUNCTION__);
            goto restore_cfg;
        }
        prox_sum += prox_cal_info[i].prox_data;
        if (prox_cal_info[i].prox_data > prox_max)
            prox_max = prox_cal_info[i].prox_data;
        mdelay(100);
    }

    sat_prox = (256 - taos->dev_config.prox_adc_time) << 10;
    prox_mean = prox_sum/20;

    /* algorithms to be improved */
    /* threshold value may be different according to current environment */
    taos->dev_config.prox_threshold_hi = ((((prox_max - prox_mean) * 200) + 50)/100) + prox_mean;
    taos->dev_config.prox_threshold_lo = ((((prox_max - prox_mean) * 170) + 50)/100) + prox_mean;
    if (taos->dev_config.prox_threshold_lo < ((sat_prox*12)/100)) {
        taos->dev_config.prox_threshold_lo = ((sat_prox*12)/100);
        taos->dev_config.prox_threshold_hi = ((sat_prox*15)/100);
    }

    if(taos->dev_config.prox_threshold_hi > PROX_THRESHOLD_HI_MAX)
        taos->dev_config.prox_threshold_hi = PROX_THRESHOLD_HI_MAX;
    if(taos->dev_config.prox_threshold_lo > PROX_THRESHOLD_LO_MAX)
        taos->dev_config.prox_threshold_lo = PROX_THRESHOLD_LO_MAX;

restore_cfg:
    /* update config */
    taos_device_update_cfg(taos);

    TAOS_DBG("taos_prox_calibrate out\n");

    return ret;
}
static int taos_open(struct inode *inode, struct file *file)
{
  //  int ret;
    TAOS_DBG("taos_open enter\n");

    if( light_device_minor == iminor(inode) ){
        als_first_read = 1;
        TAOS_DBG("%s:light sensor open", __FUNCTION__);
    }

    if( proximity_device_minor == iminor(inode) ){
        TAOS_DBG("%s:proximity snesor open", __FUNCTION__);
		wake_lock( &proximity_wake_lock);

        /* why ?? */
        /* 0 is close, 1 is far */
        input_report_abs(this_taos_data->input_dev, ABS_DISTANCE, DISTENCE_FAR);
        input_sync(this_taos_data->input_dev);
        TAOS_DBG("%s:proximity = %d", __FUNCTION__, 1);
    }

    mutex_lock(&this_taos_data->mlock);
    if( atomic_read(&this_taos_data->open_times) == 0)
    {
        /* calibrate when first open,if failed put err and use default */
        /* needn't calibrate,use default threshold val */
        /*ret = taos_prox_calibrate(this_taos_data);
        if(ret)
        {
            printk(KERN_ERR "%s: taos_prox_calibrate\n", __FUNCTION__);
        }*/
        // set timer delay in timer_func
        // hrtimer_start(&this_taos_data->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
    }

    atomic_inc(&this_taos_data->open_times);
    mutex_unlock(&this_taos_data->mlock);

    TAOS_DBG("taos_open out\n");

    return nonseekable_open(inode, file);
}

static int taos_release(struct inode *inode, struct file *file)
{
    TAOS_DBG("taos_release enter\n");

    mutex_lock(&this_taos_data->mlock);
    atomic_dec(&this_taos_data->open_times);
    if(atomic_read(&this_taos_data->open_times) == 0)
    {
        /* force two flag to off */
        atomic_set(&this_taos_data->p_flag, 0);
        atomic_set(&this_taos_data->l_flag, 0);
        hrtimer_cancel(&this_taos_data->timer);
    }
    mutex_unlock(&this_taos_data->mlock);
    if( proximity_device_minor == iminor(inode) )
    {
        printk("%s: proximity_device_minor == iminor(inode)", __func__);
        wake_unlock( &proximity_wake_lock);
    }

    TAOS_DBG("taos_release out\n");
    return 0;
}

static long taos_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)
{
    void __user *argp = (void __user *)arg;
    short flag;

    switch (cmd)
    {
        case TAOS_ECS_IOCTL_APP_SET_LFLAG:
            if (copy_from_user(&flag, argp, sizeof(flag)))
                return -EFAULT;
            break;
        case TAOS_ECS_IOCTL_APP_SET_PFLAG:
            if (copy_from_user(&flag, argp, sizeof(flag)))
                return -EFAULT;
            break;
        case TAOS_ECS_IOCTL_APP_SET_DELAY:
            if (copy_from_user(&flag, argp, sizeof(flag)))
                return -EFAULT;
            break;

        default:
            break;
    }

    mutex_lock(&this_taos_data->mlock);
    switch (cmd)
    {
        case TAOS_ECS_IOCTL_APP_SET_LFLAG:
            atomic_set(&this_taos_data->l_flag, flag);
            taos_device_update_cfg(this_taos_data);
            break;

        case TAOS_ECS_IOCTL_APP_GET_LFLAG:
            flag = atomic_read(&this_taos_data->l_flag);
            break;

        case TAOS_ECS_IOCTL_APP_SET_PFLAG:
            atomic_set(&this_taos_data->p_flag, flag);
            taos_device_update_cfg(this_taos_data);
            break;

        case TAOS_ECS_IOCTL_APP_GET_PFLAG:
            flag = atomic_read(&this_taos_data->p_flag);
            break;

        case TAOS_ECS_IOCTL_APP_SET_DELAY:
            if(flag)
                atomic_set(&this_taos_data->timer_delay, flag);
            else
                atomic_set(&this_taos_data->timer_delay, TIMER_DEFAULT_DELAY);
            break;

        case TAOS_ECS_IOCTL_APP_GET_DELAY:
            flag = atomic_read(&this_taos_data->timer_delay);
            break;

        default:
            break;
    }
    mutex_unlock(&this_taos_data->mlock);

    switch (cmd)
    {
        case TAOS_ECS_IOCTL_APP_GET_LFLAG:
            if (copy_to_user(argp, &flag, sizeof(flag)))
                return -EFAULT;
            break;

        case TAOS_ECS_IOCTL_APP_GET_PFLAG:
            if (copy_to_user(argp, &flag, sizeof(flag)))
                return -EFAULT;
            break;

        case TAOS_ECS_IOCTL_APP_GET_DELAY:
            if (copy_to_user(argp, &flag, sizeof(flag)))
                return -EFAULT;

            break;

        default:
            break;
    }
    return 0;

}

static struct file_operations taos_fops =
{
    .owner = THIS_MODULE,
    .open = taos_open,
    .release = taos_release,
    .unlocked_ioctl = taos_ioctl,
};

static struct miscdevice light_device =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = "light",
    .fops = &taos_fops,
};

static struct miscdevice proximity_device =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = "proximity",
    .fops = &taos_fops,
};

static void taos_work_func(struct work_struct *work)
{
    struct taos_data *taos = container_of(work, struct taos_data, work);
    int ret;
    int sesc;
    int nsesc;
         u8 als_level = 0;
         u8 i = 0;
    TAOS_DBG("taos_work_func enter\n ");

    mutex_lock(&taos->mlock);

    /* prox process */
    if(atomic_read(&taos->p_flag))
    {
        struct taos_prox_info prox;
        ret = taos_prox_get(taos,&prox);
        if(ret < 0)
        {
            printk(KERN_ERR "%s: get prox failed\n",__FUNCTION__);
            goto als_process;
        }
        TAOS_DBG("get prox event = %d\n",prox.prox_event);
        /* report far event immediately */
        /* 0 is close, 1 is far */
        if(prox.prox_event != -1)
        {
            input_report_abs(taos->input_dev, ABS_DISTANCE, prox.prox_event);
            input_sync(taos->input_dev);
        }
    }

    /* als process */
als_process:
    if(atomic_read(&taos->l_flag))
    {
        u32 lux;
        ret = taos_lux_get(taos);
        if(ret < 0)
        {
            printk(KERN_ERR "%s: get lux failed\n",__FUNCTION__);
            goto restart_timer;
        }

        /*actual lux need to div 1000 */
        lux = ret>>10;
        TAOS_DBG("get lux = %d\n",lux);
        als_level = LSENSOR_MAX_LEVEL - 1;
        for(i = 0; i < LSENSOR_MAX_LEVEL; i++)
        {
              if(lux < (u32)luxValues[i])
              {                
                 als_level = i;
                 break;
              }
        }
        if(als_first_read)
        {
	        	als_first_read = 0;
                input_report_abs(taos->input_dev, ABS_LIGHT, -1);
				input_sync(taos->input_dev);
        }
        else
        {
                input_report_abs(taos->input_dev, ABS_LIGHT, als_level);
				input_sync(taos->input_dev);
        }
    }
    
restart_timer:
    sesc = atomic_read(&taos->timer_delay)/1000;
    nsesc = (atomic_read(&taos->timer_delay)%1000)*1000000;
    hrtimer_start(&taos->timer, ktime_set(sesc, nsesc), HRTIMER_MODE_REL);

    mutex_unlock(&taos->mlock);

    TAOS_DBG("taos_work_func out\n ");
}

static enum hrtimer_restart taos_timer_func(struct hrtimer *timer)
{
    struct taos_data *taos = container_of(timer, struct taos_data, timer);
    queue_work(taos_wq, &taos->work);
    return HRTIMER_NORESTART;
}

#ifdef TAOS_DRIVER_DEBUG
static ssize_t taos_flag_show(struct kobject *kobj,struct kobj_attribute *attr, char *buf)
{
    struct taos_data *taos = this_taos_data;
    return sprintf(buf,"l_flag:0x%x p_flag:0x%x delay:%d\n",
        atomic_read(&taos->l_flag),atomic_read(&taos->p_flag),atomic_read(&taos->timer_delay));
}
static ssize_t taos_lflag_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    unsigned long flag;
    int err;
    struct taos_data *taos = this_taos_data;

    TAOS_DBG("get l_flag:%s\n",buf);
    err = strict_strtoul(buf, 10, &flag);
    if(err)
    {
        printk("input err!\n");
        return -EINVAL;
    }
    if((flag != 0)&&(flag != 1))
    {
        printk("input val invalid,please input 0/1!\n");
        return -EINVAL;
    }
    else
    {
        mutex_lock(&taos->mlock);
        atomic_set(&taos->l_flag, flag);
        taos_device_update_cfg(taos);
        mutex_unlock(&taos->mlock);
    }

    return count;
}
static ssize_t taos_pflag_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    unsigned long flag;
    int ret;
    int err;
    struct taos_data *taos = this_taos_data;

    TAOS_DBG("get p_flag:%s\n",buf);
    err = strict_strtoul(buf, 10, &flag);
    if(err)
    {
        printk("input err!\n");
        return -EINVAL;
    }
    if((flag != 0)&&(flag != 1))
    {
        printk("input val invalid,please input 0/1!\n");
        return -EINVAL;
    }
    else
    {
        mutex_lock(&taos->mlock);
        if(flag)
        {
            /* calibrate when first open,if failed put err and use default */
            ret = taos_prox_calibrate(taos);
            if(ret)
            {
                printk(KERN_ERR "%s: taos_prox_calibrate\n", __FUNCTION__);
            }
        }
        atomic_set(&taos->p_flag, flag);
        taos_device_update_cfg(taos);
        mutex_unlock(&taos->mlock);
    }
    return count;
}
static ssize_t taos_delay_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int err;
    unsigned long flag;
    struct taos_data *taos = this_taos_data;

    TAOS_DBG("get delay:%s\n",buf);
    err = strict_strtoul(buf, 10, &flag);
    if (err)
    {
        printk("input err!\n");
        return -EINVAL;
    }
    if((flag<100) || (flag>2000))
    {
        printk("input val invalid,please input 100-2000!\n");
        return -EINVAL;
    }
    else
    {
        mutex_lock(&taos->mlock);
        atomic_set(&taos->timer_delay, flag);
        taos_device_update_cfg(taos);
        mutex_unlock(&taos->mlock);
    }
    return count;
}
static struct kobject *taos_debug_kobj;
static struct kobj_attribute lflag_attr = {
    .attr = {
        .name = "l_flag",
        .mode = 0666,
    },
    .show = &taos_flag_show,
    .store = &taos_lflag_store,
};
static struct kobj_attribute pflag_attr = {
    .attr = {
        .name = "p_flag",
        .mode = 0666,
    },
    .show = &taos_flag_show,
    .store = &taos_pflag_store,
};
static struct kobj_attribute delay_attr = {
    .attr = {
        .name = "delay",
        .mode = 0666,
    },
    .show = &taos_flag_show,
    .store = &taos_delay_store,
};

static struct attribute *taos_debug_attrs[] = {
    &lflag_attr.attr,
    &pflag_attr.attr,
    &delay_attr.attr,
    NULL
};

static struct attribute_group taos_debug_attr_group = {
    .attrs = taos_debug_attrs,
};

static int taos_driver_debug_creat(void)
{
    int ret = -1;
    taos_debug_kobj = kobject_create_and_add("taos_debug", NULL);
    if(!taos_debug_kobj)
        return -ENOMEM;
    ret = sysfs_create_group(taos_debug_kobj,
                     &taos_debug_attr_group);
    if(ret)
    {
        pr_err("failed to create board_properties\n");
        kobject_put(taos_debug_kobj);
    }
    return ret;
}
#endif

static int taos_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;
    int i;
    struct taos_data *taos;
    unsigned char reg_buf[TAOS_MAX_DEVICE_REGS];
    struct tmd2771_platform_data *pdata;
    printk(KERN_INFO "tmd2771_probe enter\n");
    pdata = client->dev.platform_data;
    if(pdata->power_on)
        pdata->power_on(&client->dev);
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        printk(KERN_ERR "%s: need I2C_FUNC_I2C\n",__FUNCTION__);
        ret = -ENODEV;
        goto err_check_functionality_failed;
    }

    taos = kzalloc(sizeof(*taos), GFP_KERNEL);
    if (NULL == taos) {
        printk(KERN_ERR "%s: alloc mem for taos_data failed\n",__FUNCTION__);
        ret = -ENOMEM;
        goto err_alloc_data_failed;
    }

    mutex_init(&taos->io_lock);
    mutex_init(&taos->mlock);

    INIT_WORK(&taos->work, taos_work_func);

    taos->client = client;
    i2c_set_clientdata(client, taos);

    taos->input_dev = input_allocate_device();
    if (taos->input_dev == NULL) {
        ret = -ENOMEM;
        printk(KERN_ERR "%s: Failed to allocate input device\n",__FUNCTION__);
        goto err_input_dev_alloc_failed;
    }
    taos->input_dev->name = "sensors_aps";
    taos->input_dev->id.bustype = BUS_I2C;
    input_set_drvdata(taos->input_dev, taos);
    ret = input_register_device(taos->input_dev);
    if (ret) {
        printk(KERN_ERR "%s: Unable to register %s input device\n", taos->input_dev->name,__FUNCTION__);
        goto err_input_register_device_failed;
    }
    set_bit(EV_ABS, taos->input_dev->evbit);
	input_set_abs_params(taos->input_dev, ABS_LIGHT, 0, 10240, 0, 0);
    input_set_abs_params(taos->input_dev, ABS_DISTANCE, 0, 1, 0, 0);

    ret = misc_register(&light_device);
    if (ret) {
        printk(KERN_ERR "%s: light_device register failed\n",__FUNCTION__);
        goto err_light_misc_device_register_failed;
    }

    ret = misc_register(&proximity_device);
    if (ret) {
        printk(KERN_ERR "%s: proximity_device register failed\n",__FUNCTION__);
        goto err_proximity_misc_device_register_failed;
    }

    // why??
    if( light_device.minor != MISC_DYNAMIC_MINOR ){
        light_device_minor = light_device.minor;
    }

    if( proximity_device.minor != MISC_DYNAMIC_MINOR ){
        proximity_device_minor = proximity_device.minor ;
    }

    hrtimer_init(&taos->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    taos->timer.function = taos_timer_func;
	wake_lock_init(&proximity_wake_lock, WAKE_LOCK_SUSPEND, "proximity");
    atomic_set(&taos->timer_delay, TIMER_DEFAULT_DELAY);

    taos_wq = create_singlethread_workqueue("taos_wq");

    if (!taos_wq) {
        ret = -ENOMEM;
        goto err_create_workqueue_failed;
    }

    // check devie id
    for (i = 0; i < TAOS_MAX_DEVICE_REGS; i++)
    {
        taos_normal_reg_in(taos, i, &reg_buf[i]);
    }
    if(taos_device_name(reg_buf))
    {
        printk(KERN_ERR "%s: detect taos failed\n",__FUNCTION__);
        ret = -ENODEV;
        goto err_detect_device_failed;
    }

    atomic_set(&taos->l_flag, 0);
    atomic_set(&taos->p_flag, 0);

    memcpy(&taos->dev_config,&taos_dev_def_config,sizeof(struct taos_cfg));

    ret = taos_device_update_cfg(taos);
    if(ret)
    {
        printk(KERN_ERR "%s: taos device init failed\n",__FUNCTION__);
        ret = -ENODEV;
        goto err_detect_device_failed;
    }

    atomic_set(&taos->open_times, 0);

    this_taos_data = taos;
#ifdef TAOS_DRIVER_DEBUG
    if(taos_driver_debug_creat())
        printk(KERN_INFO "warn:taos driver debug creat failed\n");
    else
        printk(KERN_INFO "taos driver debug success,see dir:/sys/taos_debug\n");
#endif
    printk(KERN_INFO "taos_probe success\n");

    return 0;
err_detect_device_failed:
    destroy_workqueue(taos_wq);
err_create_workqueue_failed:
    misc_deregister(&proximity_device);
err_proximity_misc_device_register_failed:
    misc_deregister(&light_device);
err_light_misc_device_register_failed:
err_input_register_device_failed:
    input_unregister_device(taos->input_dev);
    input_free_device(taos->input_dev);
err_input_dev_alloc_failed:
    mutex_destroy(&taos->cfg_lock);
    mutex_destroy(&taos->io_lock);
    if(pdata->power_off)
        pdata->power_off();
    kfree(taos);
err_alloc_data_failed:
err_check_functionality_failed:
    return ret;

}

static int taos_remove(struct i2c_client *client)
{
    int ret;
    struct taos_data *taos = i2c_get_clientdata(client);
    struct tmd2771_platform_data *pdata = client->dev.platform_data;
    TAOS_DBG("taos_remove enter\n ");

    hrtimer_cancel(&taos->timer);
    ret = taos_normal_reg_out(taos, TAOS_TRITON_CNTRL, 0x00);
    if(ret < 0)
    {
        printk(KERN_ERR "%s: device power down failed\n",__FUNCTION__);
    }

    misc_deregister(&light_device);
    misc_deregister(&proximity_device);

    input_unregister_device(taos->input_dev);
    if(pdata->power_off)
        pdata->power_off();
    kfree(taos);
#ifdef TAOS_DRIVER_DEBUG
    if(taos_debug_kobj)
        kobject_put(taos_debug_kobj);
#endif

    TAOS_DBG("taos_remove out\n ");

    return 0;

}

static int taos_suspend(struct i2c_client *client, pm_message_t mesg)
{
    int ret = 0;
    struct taos_data *taos = i2c_get_clientdata(client);

    TAOS_DBG("taos_suspend enter\n ");

    mutex_lock(&taos->mlock);
    if(atomic_read(&taos->l_flag) || atomic_read(&taos->p_flag))
    {
        hrtimer_cancel(&taos->timer);
    }
    mutex_unlock(&taos->mlock);

    cancel_work_sync(&taos->work);

    mutex_lock(&taos->mlock);
    ret = taos_normal_reg_out(taos, TAOS_TRITON_CNTRL, 0x00);
    if(ret < 0)
    {
        printk(KERN_ERR "%s: device power down failed\n",__FUNCTION__);
    }

    mutex_unlock(&taos->mlock);
    TAOS_DBG("taos_suspend out\n ");

    return ret;
}

static int taos_resume(struct i2c_client *client)
{
    int ret = 0;
    struct taos_data *taos = i2c_get_clientdata(client);

    TAOS_DBG("taos_resume enter\n ");
    mutex_lock(&taos->mlock);
    /* restore config and power on */
    ret = taos_device_update_cfg(taos);
    if(ret < 0)
    {
        printk(KERN_ERR "%s: device power up failed\n",__FUNCTION__);
    }

    mutex_unlock(&taos->mlock);
    TAOS_DBG("taos_resume out\n ");

    return ret;
}

static const struct i2c_device_id taos_id[] = {
    { "tmd2771", 0 },
    { }
};

 static struct i2c_driver taos_driver = {
     .probe      = taos_probe,
     .remove     = taos_remove,
     .suspend    = taos_suspend,
     .resume     = taos_resume,
     .id_table   = taos_id,
     .driver = {
         .name   ="tmd2771",
     },
 };

static int __devinit tmd2771_init(void)
{
    return i2c_add_driver(&taos_driver);
}

static void __exit tmd2771_exit(void)
{
    i2c_del_driver(&taos_driver);
    if (taos_wq)
        destroy_workqueue(taos_wq);
}

/* for boad_msm7x27.c
#ifdef CONFIG_PROXIMITY_EVERLIGHT_TMD2771
  {
    I2C_BOARD_INFO("tmd2771", 0x72 >> 1),
  },
#endif
*/
module_init(tmd2771_init);
module_exit(tmd2771_exit);

MODULE_DESCRIPTION("taos-tmd2771 Driver");
MODULE_LICENSE("GPL");

