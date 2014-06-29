/*
 * leds-tps61310.c - driver for National Semiconductor tps61310 Funlight Chip
 *
 * Copyright (C) 2009 Antonio Ospite <ospite@studenti.unina.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

/*
 * I2C driver for National Semiconductor tps61310 Funlight Chip
 *
 * This helper chip can drive up to 8 leds, with two programmable DIM modes;
 * it could even be used as a gpio expander but this driver assumes it is used
 * as a led controller.
 *
 * The DIM modes are used to set _blink_ patterns for leds, the pattern is
 * specified supplying two parameters:
 *   - period: from 0s to 1.6s
 *   - duty cycle: percentage of the period the led is on, from 0 to 100
 *
 * tps61310 can be found on Motorola A910 smartphone, where it drives the rgb
 * leds, the camera flash light and the displays backlights.
 */
#include <mach/board.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/leds.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <mach/gpio.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/debugfs.h>
#include <hsad/config_interface.h>
#include <linux/semaphore.h>

#define  TPS61310_DEBUG
#ifdef TPS61310_DEBUG
#define TPS61310G(fmt, args...) printk(KERN_INFO "leds-tps61310: " fmt, ## args)
#else
#define TPS61310G(fmt, args...) 
#endif

#define TPS61310_RESET "FLASH_DRV_RST"
#define TPS61310_STRB0 "FLASH_DRV_STROBE0"
#define TPS61310_STRB1 "FLASH_DRV_STROBE1"
#define TPS61310_I2C_RETRY_TIMES 10

#define TPS61310_LED_OFF            0
#define TPS61310_LED_LOW            1
#define TPS61310_LED_HIGH           2
#define TPS61310_LED_INIT           3
#define TPS61310_LED_RELEASE        4
#define GET_GPIO_20_21_CONFIG       5
#define SET_GPIO_20_21_CONFIG_4     6
#define TPS61310_FLASHLIGHT_OFF     7
#define TPS61310_FLASHLIGHT_LOW     8
#define TPS61310_FLASHLIGHT_MEDIUM  9
#define TPS61310_FLASHLIGHT_HIGH    10

/* register setting  */
#define TPS61310_REG_0     0x00 /* LEDs 0-7 InputRegister (Read Only) */
#define TPS61310_REG_1     0x01 /* None (Read Only) */
#define TPS61310_REG_2     0x02 /* Frequency Prescaler 0 (R/W) */
#define TPS61310_REG_3     0x03 /* PWM Register 0 (R/W) */
#define TPS61310_REG_4     0x04 /* Frequency Prescaler 1 (R/W) */
#define TPS61310_REG_5     0x05 /* PWM Register 1 (R/W) */
#define TPS61310_REG_6     0x06 /* LEDs 0-3 Selector (R/W) */
#define TPS61310_REG_7     0x07 /* LEDs 4-7 Selector (R/W) */

/*low mode*/
#define TPS61310_LOW_MODE_REG_0_VAL	0x0A    /* video (LED2)50mA+(LED1/3)2*25mA */
#define TPS61310_LOW_MODE_REG_1_VAL 0x40    /* video mode */
#define TPS61310_LOW_MODE_REG_2_VAL 0x40    /* video mode */
#define TPS61310_LOW_MODE_REG_5_VAL 0x6F    /* enable LED1/2/3 */

/*high mode*/
#define TPS61310_HIGH_MODE_REG_0_VAL 0x00	/* flash mode */
#define TPS61310_HIGH_MODE_REG_1_VAL 0x90	/* flash LED2 400mA */
#define TPS61310_HIGH_MODE_REG_2_VAL 0x88	/* flash LED1/3 400mA */
#define TPS61310_HIGH_MODE_REG_1_VAL_C8869L 0x98	/* flash LED2 600mA */
#define TPS61310_HIGH_MODE_REG_2_VAL_C8869L 0x8C	/* flash LED1/3 300mA+300mA=600mA */
#define TPS61310_HIGH_MODE_REG_3_SFT_DIS_VAL 0x64	/* flash time 170.4ms SFT disable */
#define TPS61310_HIGH_MODE_REG_3_SFT_EN_VAL 0x66	/* flash time 170.4ms SFT enable */
#define TPS61310_HIGH_MODE_REG_5_VAL 0x6F	/* enable LED1/2/3 */

/*shutdown mode*/
#define TPS61310_SHUTDOWN_MODE_REG_0_VAL 0x00   /* 0mA */
#define TPS61310_SHUTDOWN_MODE_REG_1_VAL 0x00   /* shutdown 0mA */
#define TPS61310_SHUTDOWN_MODE_REG_2_VAL 0x00   /* shutdown 0mA */

/*flashlight*/
#define STATE_BRIGHT_OFF       0x0
#define STATE_BRIGHT_LOW       0x09
#define STATE_BRIGHT_MEDIUM    0x12
#define STATE_BRIGHT_HIGH      0x1e
#define STATE_LEFT_BRIGHT_HIGH  0x18
#define STATE_RIGHT_BRIGHT_HIGH  0x06

static u8 flashlight_select = STATE_BRIGHT_HIGH;
static int g_gpio_rst, g_gpio_strb0, g_gpio_strb1;
static int flashlight_status = false;
static struct semaphore felica_sem;

static struct regulator *vreg_lvs5 = NULL;
static bool tps61310_init = false;
static struct i2c_client *tps61310_client;
static const struct i2c_device_id tps61310_i2c_id[] = {
    {"tps61310", 0},
    { }
};

#define TPS61310_LOW_MODE_RESTART 10000	/* low mode restart after 10 S */
static bool flash_low_mode_on;
static struct work_struct work_flash_on;
static struct hrtimer flash_timer;

#if CONFIG_DEBUG_FS
int msm_camera_flash_ext_ps61310(unsigned led_state);
static struct dentry *debugfs_led_flash;
extern unsigned gpio_func_config_read(unsigned gpio);
extern void gpio_func_config_write(unsigned gpio,uint32_t bits);
#endif

extern int get_camera_power_seq_type(void);
static int tps61310_flashlight_gpio_set(void)
{    
    /* reset */
    gpio_set_value(g_gpio_rst, 1);
    msleep(10);
    gpio_set_value(g_gpio_rst, 0);
    msleep(10);
    gpio_set_value(g_gpio_rst, 1);

    /* STRB0 */
    gpio_set_value(g_gpio_strb0, 1);

    /* STRB1 */
    gpio_set_value(g_gpio_strb1, 1);
    return 0;
}
static int __devinit tps61310_write(struct i2c_client *client, u8 reg, u8 data)
{
    int error;

    error = i2c_smbus_write_byte_data(client, reg, data);
    if (error) 
    {
        dev_err(&client->dev,
            "couldn't send request. Returned %d\n", error);
        return error;
    }
    return error;
}

static int __devinit tps61310_read(struct i2c_client *client, u8 reg)
{
    int ret;

    ret = i2c_smbus_write_byte(client, reg);
    if (ret) 
    {
        dev_err(&client->dev,
            "couldn't send request. Returned %d\n", ret);
        return ret;
    }

    ret = i2c_smbus_read_byte(client);
    if (ret < 0) 
    {
        dev_err(&client->dev,
            "couldn't read register. Returned %d\n", ret);
        return ret;
    }

    return ret;
}

static int tps61310_register_config(void)
{
    int error = 0;
    error = tps61310_write(tps61310_client, TPS61310_REG_2 , 0x40);
    if (error) 
    {
        dev_err(&tps61310_client->dev, "failed to set leds mode!\n");
        return error;
    }
    error = tps61310_read(tps61310_client, TPS61310_REG_2);
    if (error != 0x40) 
    {
        dev_err(&tps61310_client->dev, "read leds mode fail\n");
        return error;
    }    
    error = tps61310_write(tps61310_client, TPS61310_REG_5 , 0x6e);
    if (error) 
    {
        dev_err(&tps61310_client->dev, "failed to set leds mode!\n");
        return error;
    }    
    return 0;
}

#if 0
static int flash_tps61310_i2c_read_b(struct i2c_client *client, uint8_t address, uint8_t *data, uint8_t length)
{
    int retry;
    uint8_t addr[1];

    struct i2c_msg msg[] = {
        {
            .addr = client->addr,
            .flags = 0,
            .len = 1,
            .buf = addr, /*i2c address*/
        },
        {
            .addr = client->addr,
            .flags = I2C_M_RD,
            .len = length,
            .buf = data,
        }
    };
    addr[0] = address & 0xFF;

    for (retry = 0; retry < TPS61310_I2C_RETRY_TIMES; retry++) {
        if (i2c_transfer(client->adapter, msg, 2) == 2)/* read two times,because msg[2] */
            break;
        mdelay(10);
    }
    if (retry == TPS61310_I2C_RETRY_TIMES) {
        printk(KERN_ERR "i2c_read_block retry over %d\n",
            TPS61310_I2C_RETRY_TIMES);
        return -EIO;
    }
    return 0;

}
#endif

static int flash_tps61310_i2c_txdata(struct i2c_client *client, uint8_t address, uint8_t *data, uint8_t length)
{
    int retry, loop_i;
    uint8_t buf[length + 1];

    struct i2c_msg msg[] = 
    {
        {
        .addr = client->addr,
        .flags = 0,
        .len = length + 1,
        .buf = buf,
        }
    };

    buf[0] = address & 0xFF;

    for (loop_i = 0; loop_i < length; loop_i++)
        buf[loop_i + 1] = data[loop_i];

    for (retry = 0; retry < TPS61310_I2C_RETRY_TIMES; retry++) 
    {
        if (i2c_transfer(client->adapter, msg, 1) == 1)
            break;
        mdelay(10);
    }

    if (retry == TPS61310_I2C_RETRY_TIMES) 
    {
        printk(KERN_ERR "i2c_write_block retry over %d\n",
            TPS61310_I2C_RETRY_TIMES);
        return -EIO;
    }
    return 0;

}

static int flash_tps61310_i2c_write_b(struct i2c_client *client, uint16_t address, uint8_t value)
{
    flash_tps61310_i2c_txdata(client, address, &value, 1);
    return 0;
}

/*solve kernel warning prompt because of regulator enable & disable do not match start*/

static int tps61310_i2c_init(void)
{
    int err = 0;
    /* power up */
/* camera and flash light using same i2c interface, here just not judge regulator is enabled or not */
    //if (vreg_lvs5 != NULL && !regulator_is_enabled(vreg_lvs5)) 
    if (vreg_lvs5 != NULL)
    {
        regulator_enable(vreg_lvs5);
    }

    /* reset */
    gpio_set_value(g_gpio_rst, 1);
    msleep(10);
    gpio_set_value(g_gpio_rst, 0);
    msleep(10);
    gpio_set_value(g_gpio_rst, 1);

    /* STRB0 */
    gpio_set_value(g_gpio_strb0, 1);

    /* STRB1 */
    gpio_set_value(g_gpio_strb1, 0);
    return err;
}

static void tps61310_flashlight_off(void)
{
    int ret;
    if (flashlight_status) 
    {
        flashlight_status = false;
        ret = tps61310_write(tps61310_client, TPS61310_REG_0 , STATE_BRIGHT_OFF);
        if (ret) 
        {
            dev_err(&tps61310_client->dev, "failed to set leds lightness!\n");
        }
        gpio_set_value(g_gpio_rst, 0);
        gpio_set_value(g_gpio_strb0, 0);
        gpio_set_value(g_gpio_strb1, 0);
        
        if(vreg_lvs5 && regulator_is_enabled(vreg_lvs5))
        {
            regulator_disable(vreg_lvs5);

        }
    }
}
/* solve kernel warning prompt because of regulator enable & disable do not match end*/

static void tps61310_flashlight_low(void)
{
    int ret;
    if (vreg_lvs5 != NULL && !regulator_is_enabled(vreg_lvs5)) 
    {
        if (regulator_enable(vreg_lvs5)) 
        {
            dev_err(&tps61310_client->dev, "%s: VREG VIO  8960_lvs5 enable failed\n", __func__);
            vreg_lvs5 = NULL;
        }
    }
    if (!flashlight_status) 
    {
        flashlight_status = true;
        tps61310_flashlight_gpio_set();	
        msleep(5);
        ret = tps61310_register_config();
        if (ret)
        {
            dev_err(&tps61310_client->dev, "failed to config register!\n");
        }
    } 
    ret = tps61310_write(tps61310_client, TPS61310_REG_0 , STATE_BRIGHT_LOW);
    if (ret)
    {
        dev_err(&tps61310_client->dev, "failed to set leds lightness!\n");
    }
}

static void tps61310_flashlight_medium(void)
{
    int ret;
    if (vreg_lvs5 != NULL && !regulator_is_enabled(vreg_lvs5)) 
    {
        if (regulator_enable(vreg_lvs5)) 
        {
            dev_err(&tps61310_client->dev, "%s: VREG VIO  8960_lvs5 enable failed\n", __func__);
            vreg_lvs5 = NULL;
        }
    }
    if (!flashlight_status) 
    {
        flashlight_status = true;
        tps61310_flashlight_gpio_set();	
        msleep(5);
        ret = tps61310_register_config();
        if (ret) 
        {
            dev_err(&tps61310_client->dev, "failed to config register!\n");
        }
    } 
    ret = tps61310_write(tps61310_client, TPS61310_REG_0 , STATE_BRIGHT_MEDIUM);
    if (ret) 
    {
        dev_err(&tps61310_client->dev, "failed to set leds lightness!\n");
    }
}

static void tps61310_flashlight_high(void)
{
    int ret;
    if (vreg_lvs5 != NULL && !regulator_is_enabled(vreg_lvs5)) 
    {
        if (regulator_enable(vreg_lvs5)) 
        {
            dev_err(&tps61310_client->dev, "%s: VREG VIO  8960_lvs5 enable failed\n", __func__);
            vreg_lvs5 = NULL;
        }
    }
    if (!flashlight_status) 
    {
        flashlight_status = true;
        tps61310_flashlight_gpio_set();
        msleep(5);
        ret = tps61310_register_config();
        if (ret) 
        {
            dev_err(&tps61310_client->dev, "failed to config register!\n");
        }
    }
    ret = tps61310_write(tps61310_client, TPS61310_REG_0, flashlight_select);
    if (ret) 
    {
        dev_err(&tps61310_client->dev, "failed to set leds lightness!\n");
    }
}

static void tps61310_release_mode(void)
{
    if(vreg_lvs5 && regulator_is_enabled(vreg_lvs5))
    {
        regulator_disable(vreg_lvs5);            
    }
    gpio_set_value(g_gpio_rst, 0);
    gpio_set_value(g_gpio_strb0, 0);
    gpio_set_value(g_gpio_strb1, 0);
}

static void flash_tps61310_low_mode(struct i2c_client *led_client)
{
    flash_tps61310_i2c_write_b(led_client,TPS61310_REG_0,TPS61310_LOW_MODE_REG_0_VAL);
    flash_tps61310_i2c_write_b(led_client,TPS61310_REG_1,TPS61310_LOW_MODE_REG_1_VAL);
    flash_tps61310_i2c_write_b(led_client,TPS61310_REG_2,TPS61310_LOW_MODE_REG_2_VAL);
    flash_tps61310_i2c_write_b(led_client,TPS61310_REG_5,TPS61310_LOW_MODE_REG_5_VAL);
}

static void flash_tps61310_high_mode(struct i2c_client *led_client)
{
    flash_tps61310_i2c_write_b(led_client,TPS61310_REG_5,TPS61310_HIGH_MODE_REG_5_VAL);
    flash_tps61310_i2c_write_b(led_client,TPS61310_REG_3,TPS61310_HIGH_MODE_REG_3_SFT_DIS_VAL);
    flash_tps61310_i2c_write_b(led_client,TPS61310_REG_0,TPS61310_HIGH_MODE_REG_0_VAL);
    //if c8869l set output current as 600 mA else set current as 400mA
    if(POWER_SEQ_C8869L==get_camera_power_seq_type())
    {
    	flash_tps61310_i2c_write_b(led_client,TPS61310_REG_1,TPS61310_HIGH_MODE_REG_1_VAL_C8869L);
    	flash_tps61310_i2c_write_b(led_client,TPS61310_REG_2,TPS61310_HIGH_MODE_REG_2_VAL_C8869L);
    }
    else
    {
    	flash_tps61310_i2c_write_b(led_client,TPS61310_REG_1,TPS61310_HIGH_MODE_REG_1_VAL);
    	flash_tps61310_i2c_write_b(led_client,TPS61310_REG_2,TPS61310_HIGH_MODE_REG_2_VAL);
    }
    flash_tps61310_i2c_write_b(led_client,TPS61310_REG_3,TPS61310_HIGH_MODE_REG_3_SFT_EN_VAL);
}

static void flash_tps61310_shutdown_mode(struct i2c_client *led_client)
{
    flash_tps61310_i2c_write_b(led_client,TPS61310_REG_0,TPS61310_SHUTDOWN_MODE_REG_0_VAL);
    flash_tps61310_i2c_write_b(led_client,TPS61310_REG_1,TPS61310_SHUTDOWN_MODE_REG_1_VAL);
    flash_tps61310_i2c_write_b(led_client,TPS61310_REG_2,TPS61310_SHUTDOWN_MODE_REG_2_VAL);
}

static void work_flash_tps61310_low_mode(struct work_struct *work)
{
    hrtimer_cancel(&flash_timer);
    flash_tps61310_low_mode(tps61310_client);
    hrtimer_start(&flash_timer,
                    ktime_set(TPS61310_LOW_MODE_RESTART / 1000, 
                    (TPS61310_LOW_MODE_RESTART % 1000) * 1000000),
                    HRTIMER_MODE_REL);
}

static void flash_tps61310_low_mode_on(void)
{
    schedule_work(&work_flash_on);
}

static enum hrtimer_restart flash_tps61310_timer_func(struct hrtimer *timer)
{
    if (flash_low_mode_on)
    {
        flash_tps61310_low_mode_on();
    }
    return HRTIMER_NORESTART;
}

int msm_camera_flash_ext_ps61310(unsigned led_state)
{
    int rc = 0;

    TPS61310G("enter %s led_state=%d\n",__func__,led_state);
    down(&felica_sem);
    switch (led_state) 
    {

    case TPS61310_LED_INIT:
        tps61310_flashlight_off();
        if(!tps61310_init)
        {
            tps61310_i2c_init();
            tps61310_init = true;
        }
        break;
    case TPS61310_LED_RELEASE:
        if (tps61310_init)
        {
            flash_low_mode_on = false;
            flash_tps61310_shutdown_mode(tps61310_client);
            tps61310_release_mode();
            tps61310_init = false;
        }
        break;

    case TPS61310_LED_OFF:
        if (tps61310_init)
        {
            flash_low_mode_on = false;
            flash_tps61310_shutdown_mode(tps61310_client);
        }
        break;

    case TPS61310_LED_LOW:
        if (tps61310_init) 
        {
            flash_low_mode_on = true;
            flash_tps61310_low_mode_on();
        }
        break;

    case TPS61310_LED_HIGH:
        if (tps61310_init) 
        {
            flash_low_mode_on = false;
            flash_tps61310_high_mode(tps61310_client);
        }
        break;

    case GET_GPIO_20_21_CONFIG:	
        rc = gpio_func_config_read(20);
        printk("%s gpio_20_config=%d\n",__func__,rc);
        rc = gpio_func_config_read(21);
        printk("%s gpio_21_config=%d\n",__func__,rc);
        break;

    case SET_GPIO_20_21_CONFIG_4:
        gpio_func_config_write(20,4);
        gpio_func_config_write(21,4);
        break;

    case TPS61310_FLASHLIGHT_OFF:
        tps61310_flashlight_off();
        break;

    case TPS61310_FLASHLIGHT_LOW:
        tps61310_flashlight_low();
        break;

    case TPS61310_FLASHLIGHT_MEDIUM:
        tps61310_flashlight_medium();
        break;

    case TPS61310_FLASHLIGHT_HIGH:
        tps61310_flashlight_high();
        break;

    default:
        rc = -EFAULT;
        break;
    }
    up(&felica_sem);
    return rc;
}
EXPORT_SYMBOL(msm_camera_flash_ext_ps61310);

#if CONFIG_DEBUG_FS 
static void led_flash_debug_set(unsigned long status)
{
    printk("%s \n",__func__);
    msm_camera_flash_ext_ps61310(status);
    }

    static int led_flash_debug_open(struct inode *inode, struct file *file)
    {
    file->private_data = inode->i_private;
    pr_info("debug in %s\n", (char *) file->private_data);
    return 0;
}

static int led_flash_parameters(char *buf, long int *param1, int num_of_par)
{
    char *token;
    int base, cnt;

    token = strsep(&buf, " ");

    for (cnt = 0; cnt < num_of_par; cnt++) {
        if (token != NULL) {
            if ((token[1] == 'x') || (token[1] == 'X'))
                base = 16;
            else
                base = 10;

            if (strict_strtoul(token, base, &param1[cnt]) != 0)
                return -EINVAL;

            token = strsep(&buf, " ");
        } else
            return -EINVAL;
    }
    return 0;
}

static ssize_t led_flash_debug_write(struct file *filp,
    const char __user *ubuf, size_t cnt, loff_t *ppos)
    {
    char *lb_str = filp->private_data;
    char lbuf[32];
    int rc;
    unsigned long param[5];

    if (cnt > sizeof(lbuf) - 1)
        return -EINVAL;

    rc = copy_from_user(lbuf, ubuf, cnt);
    if (rc)
        return -EFAULT;

    lbuf[cnt] = '\0';

    if (!strcmp(lb_str, "led_flash")) 
    {
        rc = led_flash_parameters(lbuf, param, 3);
        if (!rc) 
        {
            pr_info("%s %lu %lu %lu\n", lb_str, param[0], param[1],
                param[2]);

            printk("IN LED_FLASH DEBUG_FS\n");
            led_flash_debug_set(param[0]);
            printk("OUT LED_FLASH DEBUG_FS\n");
        }
        else 
        {
            pr_err("%s: Error, invalid parameters\n", __func__);
            rc = -EINVAL;
            goto led_flash_error;
        }
    }

led_flash_error:
    if (rc == 0)
        rc = cnt;
    else
        pr_err("%s: rc = %d\n", __func__, rc);

    return rc;
}

static const struct file_operations led_flash_debug_fops = {
    .open = led_flash_debug_open,
    .write = led_flash_debug_write
    };
    #endif

static ssize_t tps61310_led_set_brightness(struct device *dev, struct device_attribute *attr,
             const char *buf, size_t count)
{    
    if (buf[0] == '0')//close
    {
//        dev_info(&tps61310_client->dev, "%s:shutdown flash led.\n", __func__);
        msm_camera_flash_ext_ps61310(TPS61310_FLASHLIGHT_OFF);
    }
    else if (buf[0] == '1')//class 1
    {
//        dev_info(&tps61310_client->dev, "%s:set flash led brightness to level 1.\n", __func__);
        msm_camera_flash_ext_ps61310(TPS61310_FLASHLIGHT_LOW);
    }
    else if (buf[0] == '2')//class 2
    {
//        dev_info(&tps61310_client->dev, "%s:set flash led brightness to level 2.\n", __func__);
        msm_camera_flash_ext_ps61310(TPS61310_FLASHLIGHT_MEDIUM);
    }
    else  if(buf[0] == '3')//class 3
    {
//        dev_info(&tps61310_client->dev, "%s:set flash led brightness to level 3.\n", __func__);
        flashlight_select = STATE_BRIGHT_HIGH;
        msm_camera_flash_ext_ps61310(TPS61310_FLASHLIGHT_HIGH);
    }else if(buf[0] == '4'){
        flashlight_select = STATE_LEFT_BRIGHT_HIGH;
        msm_camera_flash_ext_ps61310(TPS61310_FLASHLIGHT_HIGH);
    }else if(buf[0] == '5'){
        flashlight_select = STATE_RIGHT_BRIGHT_HIGH;
        msm_camera_flash_ext_ps61310(TPS61310_FLASHLIGHT_HIGH);
    }else{
        dev_err(&tps61310_client->dev, "overstep the value that set leds lightness \n");
    }
    return count;
}

static ssize_t tps61310_led_get_brightness(struct device *dev, struct device_attribute *attr,
            char *buf)
{
    return 1;
}

static struct device_attribute tps61310_led=
    __ATTR(tps61310_led_lightness, 0664, tps61310_led_get_brightness,
                        tps61310_led_set_brightness);

static int tps61310_i2c_probe(struct i2c_client *client,
    const struct i2c_device_id *id)
{
    int err = 0;

    TPS61310G("tps61310_i2c_probe enter \n");
    err = i2c_check_functionality(client->adapter, I2C_FUNC_I2C);
    if (!err)
    {
        pr_err("i2c_check_functionality failed\n");
        goto tps61310_out;
    }

    vreg_lvs5 = regulator_get(NULL, "8921_lvs5");
    if (IS_ERR(vreg_lvs5)) 
        {
        dev_err(&tps61310_client->dev, "%s: VREG VIO 8960_lvs5 get failed\n", __func__);
        vreg_lvs5 = NULL;	
        goto tps61310_out;
    }
    g_gpio_rst = get_gpio_num_by_name(TPS61310_RESET);
    if(g_gpio_rst < 0)
    {
        printk(KERN_ERR"%s get_gpio_num_by_name fail\n",__func__);
        goto lvs5_err;
    }
    err = gpio_request(g_gpio_rst, "tps61310");
    if (err < 0) 
    {
        gpio_free(g_gpio_rst);
        err = gpio_request(g_gpio_rst, "tps61310");
        if(err < 0)
        {
            TPS61310G("gpio_request(g_gpio_rst,tps61310) again fail \n");
            goto lvs5_err;
        }
        TPS61310G("gpio_request(g_gpio_rst,tps61310) again success\n");
    }
    gpio_direction_output(g_gpio_rst, 0);

    g_gpio_strb0 = get_gpio_num_by_name(TPS61310_STRB0);
    if(g_gpio_strb0 < 0)
    {
        printk(KERN_ERR"%s get_gpio_num_by_name fail\n",__func__);
        goto gpio_rst_err;
    }
    err = gpio_request(g_gpio_strb0, "tps61310");
    if (err < 0) 
    {
        gpio_free(g_gpio_strb0);
        err = gpio_request(g_gpio_strb0, "tps61310");
        if(err < 0)
        {
            TPS61310G("gpio_request(g_gpio_strb0,tps61310) again fail \n");
            goto gpio_rst_err;
        }
        TPS61310G("gpio_request(g_gpio_strb0,tps61310) again success\n");
    }
    gpio_direction_output(g_gpio_strb0, 0);

    g_gpio_strb1 = get_gpio_num_by_name(TPS61310_STRB1);
    if(g_gpio_strb1 < 0)
    {
        printk(KERN_ERR"%s get_gpio_num_by_name fail\n",__func__);
        goto gpio_strb0_err;
    }
    err = gpio_request(g_gpio_strb1, "tps61310");
    if (err < 0) 
    {
        gpio_free(g_gpio_strb1);
        err = gpio_request(g_gpio_strb1, "tps61310");
        if(err < 0)
        {
            TPS61310G("gpio_request(g_gpio_strb1,tps61310) again fail \n");
            goto gpio_strb0_err;
        }
        TPS61310G("gpio_request(g_gpio_strb1,tps61310) again success\n");
    }
    gpio_direction_output(g_gpio_strb1, 0);

    tps61310_client = client;
    if (device_create_file(&client->dev, &tps61310_led))
    {
        dev_err(&client->dev, "%s:Unable to create interface\n", __func__);
        goto gpio_strb1_err;
    }
    sema_init(&felica_sem, 1);
    return 0;
gpio_strb1_err:
    gpio_free(g_gpio_strb1);
gpio_strb0_err:
    gpio_free(g_gpio_strb0);
gpio_rst_err:
    gpio_free(g_gpio_rst);
lvs5_err:
    pr_err("tps61310_i2c_probe failed! err = %d\n", err);
    regulator_put(vreg_lvs5);
    vreg_lvs5 = NULL;
tps61310_out:
    return -1;
}

static int tps61310_i2c_remove(struct i2c_client *client)
{
    TPS61310G("tps61310_i2c_remove enter \n");	
    if(vreg_lvs5)
    {
        regulator_disable(vreg_lvs5);
        regulator_put(vreg_lvs5);
        vreg_lvs5 = NULL;
    }
    gpio_free(g_gpio_rst);
    gpio_free(g_gpio_strb0);
    gpio_free(g_gpio_strb1);
    return 0;
}

static int tps61310_i2c_suspend(struct i2c_client *client, pm_message_t mesg)
{
    msm_camera_flash_ext_ps61310(TPS61310_FLASHLIGHT_OFF);
    return 0;
    }

static int tps61310_i2c_resume(struct i2c_client *client)
{

    return 0;
}

static struct i2c_driver tps61310_i2c_driver = {
    .id_table = tps61310_i2c_id,
    .probe    = tps61310_i2c_probe,
    .remove   = __devexit_p(tps61310_i2c_remove),
    .suspend = tps61310_i2c_suspend,
    .resume = tps61310_i2c_resume,
    .driver   = {
        .name = "tps61310",
    },
};

static int __init tps61310_module_init(void)
{
    #ifdef CONFIG_DEBUG_FS
    debugfs_led_flash = debugfs_create_file("led_flash",
                                    S_IFREG | S_IWUSR|S_IWGRP, NULL, (void *) "led_flash",
                                    &led_flash_debug_fops);
    #endif

    INIT_WORK(&work_flash_on, work_flash_tps61310_low_mode);

    hrtimer_init(&flash_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    flash_timer.function = flash_tps61310_timer_func;

    return i2c_add_driver(&tps61310_i2c_driver);
}

static void __exit tps61310_module_exit(void)
{
    #ifdef CONFIG_DEBUG_FS
    if (debugfs_led_flash)
        debugfs_remove(debugfs_led_flash);
    #endif

    i2c_del_driver(&tps61310_i2c_driver);
}

module_init(tps61310_module_init);
module_exit(tps61310_module_exit);
MODULE_AUTHOR("www.huawei.com");
MODULE_DESCRIPTION("TPS61310 Fun Light /flash");
MODULE_LICENSE("GPL");

