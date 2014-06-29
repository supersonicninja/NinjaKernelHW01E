

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <hsad/config_interface.h>
#include <linux/earlysuspend.h>
#include <asm/gpio.h>
#include <linux/i2c.h>
#include <linux/leds.h>
#include <linux/delay.h>

#include <linux/pwm.h>
#include <linux/mfd/pm8xxx/pm8921.h>


#define RED_PORT 0
#define GREEN_PORT 1
#define BLUE_PORT 2

#define CMD_RED_ON 1
#define CMD_GREEN_ON 2
#define CMD_BLUE_ON 3
#define CMD_ORANGE_ON 4
#define CMD_ALL_ON 5
#define CMD_RED_OFF 6
#define CMD_GREEN_OFF 7
#define CMD_BLUE_OFF 8
#define CMD_ORANGE_OFF 9
#define CMD_ALL_OFF 10

#define CMD_RED_BLINK_EN 11
#define CMD_GREEN_BLINK_EN 12
#define CMD_BLUE_BLINK_EN 13
#define CMD_ORANGE_BLINK_EN 14
#define CMD_ALL_BLINK_EN 15
#define CMD_RED_BLINK_DIS 16
#define CMD_GREEN_BLINK_DIS 17
#define CMD_BLUE_BLINK_DIS 18
#define CMD_ORANGE_BLINK_DIS 19
#define CMD_ALL_BLINK_DIS 20
#define CMD_RED_BLINK_TIME_SET 21
#define CMD_GREEN_BLINK_TIME_SET 22

#define CMD_BRIGHTNESS_SET_BEGIN 39
#define CMD_BRIGHTNESS_SET_END 56

#define SELECT0 0x00
#define SELECT1 0x01
#define SELECT2 0x02
#define FADE_ON_TIME 0x03
#define FULLY_ON_TIME 0x04
#define FADE_OFF_TIME 0x05
#define FIRST_FULLY_OFF_TIME 0x06
#define SECOND_FULLY_OFF_TIME 0x07
#define MAX_INTENSITY 0x08
#define ONE_SHOT_MASTER_INTENSITY 0x09
#define INITLIZATION 0x10

#define INIT_VALUE_OF_SELECT0 0x00
#define INIT_VALUE_OF_SELECT1 0x01
#define INIT_VALUE_OF_SELECT2 0x00
#define INIT_VALUE_OF_FADE_ON_TIME 0x00
#define INIT_VALUE_OF_FULLY_ON_TIME 0xaa
#define INIT_VALUE_OF_FADE_OFF_TIME 0x00
#define INIT_VALUE_OF_FIRST_FULLY_OFF_TIME 0xaa
#define INIT_VALUE_OF_SECOND_FULLY_OFF_TIME 0xaa

#define INIT_VALUE_OF_MAX_INTENSITY 0x00  

#define INIT_VALUE_OF_ONE_SHOT_MASTER_INTENSITY 0x00

#define MAX_BRIGHTNESS_WE_SET 0x00

#define TIME_PARAMETER_1_BEGIN 0
#define TIME_PARAMETER_1_END 96
#define TIME_PARAMETER_2_BEGIN 95
#define TIME_PARAMETER_2_END 160
#define TIME_PARAMETER_3_BEGIN 159
#define TIME_PARAMETER_3_END 224
#define TIME_PARAMETER_4_BEGIN 223
#define TIME_PARAMETER_4_END 320
#define TIME_PARAMETER_5_BEGIN 319
#define TIME_PARAMETER_5_END 448
#define TIME_PARAMETER_6_BEGIN 447
#define TIME_PARAMETER_6_END 640
#define TIME_PARAMETER_7_BEGIN 639
#define TIME_PARAMETER_7_END 896
#define TIME_PARAMETER_8_BEGIN 895
#define TIME_PARAMETER_8_END 1280
#define TIME_PARAMETER_9_BEGIN 1279
#define TIME_PARAMETER_9_END 1792
#define TIME_PARAMETER_10_BEGIN 1791
#define TIME_PARAMETER_10_END 2560
#define TIME_PARAMETER_11_BEGIN 2559
#define TIME_PARAMETER_11_END 3584
#define TIME_PARAMETER_12_BEGIN 3583
#define TIME_PARAMETER_12_END 4928
#define TIME_PARAMETER_13_BEGIN 4927
#define TIME_PARAMETER_13_END 6944
#define TIME_PARAMETER_14_BEGIN 6943
#define TIME_PARAMETER_14_END 12224
#define TIME_PARAMETER_15_BEGIN 12223
#define MAX_LED_BRIGHTNESS 160

#define PM8921_GPIO_BASE		NR_GPIO_IRQS
#define PM8921_GPIO_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_GPIO_BASE)

static int gpio_enable;
static struct i2c_client *client_bl_ctrl;

static struct pm_gpio pwm_mode = {
	.direction        = PM_GPIO_DIR_OUT,
	.output_buffer    = PM_GPIO_OUT_BUF_CMOS,
	.output_value     = 0,
	.pull             = PM_GPIO_PULL_NO,
	.vin_sel          = PM_GPIO_VIN_S4,
	.out_strength     = PM_GPIO_STRENGTH_HIGH,
	.function         = PM_GPIO_FUNC_NORMAL,
	.inv_int_pol      = 0,
	.disable_pin      = 0,
};


static int bl_i2c_read(struct i2c_client *client, u8 reg)
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

static int bl_i2c_write(struct i2c_client *client, u8 reg, u8 data)
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

static void backlight_ctrl(struct led_classdev *led_cdev,
    enum led_brightness value)
{
    int ret;
    u8 reg_val;

    /*
      *************************************************************
      * For a large led brightness value, there will be a
      * large power dissipation.By the hardware team
      * confirmation, when led brightness value is 84,
      * power dissipation and brightness are appropriate.
      * So, config MAX_LED_BRIGHTNESS = 84.
      *************************************************************
      */
    if(value > MAX_LED_BRIGHTNESS)
    {
        value = MAX_LED_BRIGHTNESS;
    }
    reg_val = (value*15/255);

    dev_info(&client_bl_ctrl->dev, "^-^^-^HT: Entry %s (rawval outval)= (%d %d) \n",__FUNCTION__,value,reg_val);
    
    ret = bl_i2c_write(client_bl_ctrl, MAX_INTENSITY, reg_val);

    return ;
}

static struct led_classdev touchkey_backlight_ctrl = {
    .name              = "button-backlight-tk",
    .brightness_set    = backlight_ctrl,
    .brightness        = LED_OFF,
};


static ssize_t attr_get_tk_in(struct device *dev,
                     struct device_attribute *attr,
                     char *buf)
{
    int ret;
    ret = bl_i2c_read(client_bl_ctrl, MAX_INTENSITY);
	return snprintf(buf, 5, "%d\n", ret);    
}

static ssize_t attr_set_tk_in(struct device *dev,
                     struct device_attribute *attr,
                     const char *buf, size_t size)
{
    int ret;
    unsigned long val;
    u8 reg_val;
    if (strict_strtoul(buf, 10, &val))
        return -EINVAL;

    reg_val = val;
    ret = bl_i2c_write(client_bl_ctrl, MAX_INTENSITY, val);
    return size;
}


static struct device_attribute attributes_bl[] = {

    __ATTR(tk_in, 0664, attr_get_tk_in, attr_set_tk_in),

};

static int __devinit tk_backlight_ctrl_probe(struct i2c_client *client,
                                        const struct i2c_device_id *id)
{
    int ret, error,i;
    
    dev_info(&client->dev, "Three colors tk backlight probe begin!\n");

    ret = 0;
    client_bl_ctrl = client;
    
	/* If backlight is work on PWM mode, exit. */
    if(!is_leds_ctl_tca6507())
    {
        dev_err(&client->dev, "Entry %s the backlight is controled in pwm way\n",__FUNCTION__);
        goto err_free_mem;
    }
    
	ret = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(24),&pwm_mode);
	if (ret != 0)
    {   
        pr_err("%s: pwm_mode failed\n", __func__);
        goto err_free_mem;
    }   
    
    gpio_enable = PM8921_GPIO_PM_TO_SYS(24);
    
    ret = gpio_request(gpio_enable, "disp_rst_n");
    if (ret) {
        pr_err("request gpio 24 failed, ret=%d\n", ret);
    }

    gpio_set_value_cansleep(gpio_enable, 1);

    ret = 0;

    error = i2c_check_functionality(client->adapter,
                    I2C_FUNC_SMBUS_BYTE);
    if (!error)
    {
        dev_err(&client->dev, "%s adapter not supported\n",
                    dev_driver_string(&client->adapter->dev));
        return -ENODEV;
    }

    ret = bl_i2c_write(client, SELECT0, INIT_VALUE_OF_SELECT0);
    if (ret)
    {
        dev_err(&client->dev, "failed to wirte i2c register %d\n",SELECT0);
        goto err_free_mem;
    }
    ret = bl_i2c_write(client, SELECT1, INIT_VALUE_OF_SELECT1);
    if (ret)
    {
        dev_err(&client->dev, "failed to wirte i2c register %d\n",SELECT1);
        goto err_free_mem;
    }
    ret = bl_i2c_write(client, SELECT2, INIT_VALUE_OF_SELECT2);
    if (ret)
    {
        dev_err(&client->dev, "failed to wirte i2c register %d\n",SELECT2);
        goto err_free_mem;
    }

    ret = bl_i2c_write(client, FADE_ON_TIME, INIT_VALUE_OF_FADE_ON_TIME);
    if (ret)
    {
        dev_err(&client->dev, "failed to wirte i2c register %d\n",FADE_ON_TIME);
        goto err_free_mem;
    }
    ret = bl_i2c_write(client, FULLY_ON_TIME, INIT_VALUE_OF_FULLY_ON_TIME);
    if (ret)
    {
        dev_err(&client->dev, "failed to wirte i2c register %d\n",FULLY_ON_TIME);
        goto err_free_mem;
    }
        
    ret = bl_i2c_write(client, FADE_OFF_TIME, INIT_VALUE_OF_FADE_OFF_TIME);
    if (ret)
    {
        dev_err(&client->dev, "failed to wirte i2c register %d\n",FADE_OFF_TIME);
        goto err_free_mem;
    }
    ret = bl_i2c_write(client, FIRST_FULLY_OFF_TIME, INIT_VALUE_OF_FIRST_FULLY_OFF_TIME);
    if (ret)
    {
        dev_err(&client->dev, "failed to wirte i2c register %d\n",FIRST_FULLY_OFF_TIME);
        goto err_free_mem;
    }
    ret = bl_i2c_write(client, SECOND_FULLY_OFF_TIME, INIT_VALUE_OF_SECOND_FULLY_OFF_TIME);
    if (ret)
    {
        dev_err(&client->dev, "failed to wirte i2c register %d\n",SECOND_FULLY_OFF_TIME);
        goto err_free_mem;
    }
    ret = bl_i2c_write(client, MAX_INTENSITY, INIT_VALUE_OF_MAX_INTENSITY);
    if (ret)
    {
        dev_err(&client->dev, "failed to wirte i2c register %d\n",MAX_INTENSITY);
        goto err_free_mem;
    }
    ret = bl_i2c_write(client, ONE_SHOT_MASTER_INTENSITY, INIT_VALUE_OF_ONE_SHOT_MASTER_INTENSITY);
    if (ret)
    {
        dev_err(&client->dev, "failed to wirte i2c register %d\n",ONE_SHOT_MASTER_INTENSITY);
        goto err_free_mem;
    }

    for (i = 0; i < ARRAY_SIZE(attributes_bl); i++)
        if(device_create_file(&client->dev, attributes_bl+i))
            printk("create file node failed");


    ret = led_classdev_register(&client->dev, &touchkey_backlight_ctrl);
    if (ret)
    {
        dev_err(&client->dev, "create touchkey_backlight_ctrl failed!\n");
        gpio_free(gpio_enable);
    }     

err_free_mem:

    return ret;
}

static int __devexit tk_bl_ctrl_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id tk_bl_ctrl_idtable[] = {
    { "TK_backlight_ctrl", 0, },
    { }
};

MODULE_DEVICE_TABLE(i2c, tk_bl_ctrl_idtable);

static struct i2c_driver tk_bl_ctrl_driver = {
    .driver = {
            .name   = "TK_backlight_ctrl",
            .owner  = THIS_MODULE,
    },

    .id_table      = tk_bl_ctrl_idtable,
    .probe         = tk_backlight_ctrl_probe,
    .remove        = __devexit_p(tk_bl_ctrl_remove),
};
static int __init tk_bl_ctrl_init(void)
{
    return i2c_add_driver(&tk_bl_ctrl_driver);
}

static void __exit tk_bl_ctrl_exit(void)
{
    i2c_del_driver(&tk_bl_ctrl_driver);
}

module_init(tk_bl_ctrl_init);
module_exit(tk_bl_ctrl_exit);

MODULE_AUTHOR("hantao");
MODULE_DESCRIPTION("TCA6507 TK_backlight driver");
MODULE_LICENSE("GPL");
