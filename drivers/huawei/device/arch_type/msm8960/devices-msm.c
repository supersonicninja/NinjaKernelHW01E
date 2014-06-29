#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/regulator/consumer.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/delay.h>
#include "../../touchscreen/touch_platform_config.h"
#include "../../touchkey/touchkey_platform_config.h"
#include "../../accelerometer/lis3dh.h"
#include "../../accelerometer/adxl34x.h"
#include "../../accelerometer/gs_mma8452.h"
#include "../../compass/akm8963.h"
#include "../../compass/akm8975.h"
#include "../../gyroscope/l3g4200d.h"
#include "../../light/tmd2771.h"
#include "../../light/apds990x.h"
#include <hsad/config_interface.h>

#define HW_DEVICE_DEBUG
#ifdef HW_DEVICE_DEBUG
#define HW_DEV_DBG(fmt, args...)\
printk(KERN_DEBUG "[HW_DEVICE]: " fmt,## args);
#else
#define HW_DEV_DBG(fmt, args...)
#endif
/*Add TP self adapt*/
#define MAX_TP_VERSION_LEN 10
#define MAX_TK_KEYORDER_LEN 10
/*Add TP virtualkey's location self adapt,we can use it for synapics and atmel both*/
#define CONFIG_TOUCHSCREEN_VIRTUALKEY
#ifdef CONFIG_I2C
/* change i2c bus name */
#define MSM_8960_GSBI3_QUP_I2C_BUS_ID 3
#define MSM_8960_GSBI12_QUP_I2C_BUS_ID 12
#define MSM_8960_GSBI2_QUP_I2C_BUS_ID 2
/*delete 11 lines,for gpio unite*/
#define MSM_8960_GSBI5_QUP_I2C_BUS_ID 5
#endif
/* add power SENSOR_2P85_EN control*/
#define SENSOR_2P85_EN					17
#define PM8921_GPIO_BASE		NR_GPIO_IRQS
#define PM8921_GPIO_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_GPIO_BASE)
#define GET_GPIO_FAIL  -1
/**
 * is_gpio_connect - check gpio is connect
 *
 * return 1 if connect,0 if not connect
 */
static bool is_gpio_connect(int gpio)
{
    int gpio_state0,gpio_state1;

    gpio_direction_output(gpio,0);
    gpio_direction_input(gpio);
    gpio_state0 = gpio_get_value(gpio);
    gpio_direction_output(gpio,1);
    gpio_direction_input(gpio);
    gpio_state1 = gpio_get_value(gpio);

    return (gpio_state0 == gpio_state1);
}
/**
 * get_lcd_id - get lcd id of the product
 *
 * return lcd id;
 * bit0 and bit2 represent connected gpio state,
 * for example:if bit0 = 1 LCD_ID0_GPIO id is connect and state is 1;
 * bit1 and bit3 represent whether gpio connect or not,
 * for example:if bit1 = 1 LCD_ID0_GPIO is not connect
 */
/* use gpio unite func to get lcd id gpio num */
int get_lcd_id(void)
{
    int ret;
    int id0,id1;
    int gpio_id0 = GET_GPIO_FAIL;
    int gpio_id1 = GET_GPIO_FAIL;
    gpio_id0 = get_gpio_num_by_name("LCD_ID0");
    gpio_id1 = get_gpio_num_by_name("LCD_ID1");
    if(gpio_id0 <= GET_GPIO_FAIL ||gpio_id1 <= GET_GPIO_FAIL)
		return GET_GPIO_FAIL;
    HW_DEV_DBG("gpio_lcd_id0:%d gpio_lcd_id1:%d\n",gpio_id0,gpio_id1);
    ret = gpio_request(gpio_id0, "lcd_id0");
    if (ret) {
        printk(KERN_ERR"lcd_id0 gpio[%d] request failed\n", gpio_id0);
        ret = BIT(1) | BIT(3);// represent lcd not connected
        goto lcd_id0_req_fail;
    }
    ret = gpio_request(gpio_id1, "lcd_id1");
	if (ret) {
		printk(KERN_ERR"lcd_id1 gpio[%d] request failed\n", gpio_id1);
        ret = BIT(1) | BIT(3);// represent lcd not connected
        goto lcd_id1_req_fail;
	}

    if(is_gpio_connect(gpio_id0))
    {
        gpio_direction_input(gpio_id0);
        id0 = gpio_get_value(gpio_id0);
    }
    else
    {
        id0 = BIT(1);
    }

    if(is_gpio_connect(gpio_id1))
    {
        gpio_direction_input(gpio_id1);
        id1 = gpio_get_value(gpio_id1);
    }
    else
    {
        id1 = BIT(1);
    }

    gpio_free(gpio_id0);
    gpio_free(gpio_id1);

    ret = (id1<<2) | id0;

    return ret;

lcd_id1_req_fail:
    gpio_free(gpio_id0);
lcd_id0_req_fail:
    return ret;
}
EXPORT_SYMBOL(get_lcd_id);

/**
 * gpio_config_interrupt - config gpio irq
 * gpio:gpio num of the irq
 * name:irq name
 * enable:get gpio or free gpio
 * return irq num if success,error num if error occur
 */
int gpio_config_interrupt(int gpio,char * name,int enable)
{
    int err = 0;

    if(gpio<0||gpio>151)
    {
        printk(KERN_ERR "%s:gpio[%d] is invalid\n",__func__,gpio);
        return -EINVAL;
    }
    if(enable)
    {
        err = gpio_request(gpio, name);
        if (err) {
            printk(KERN_ERR"%s: gpio_request failed for intr %d\n", name,gpio);
            return err;
        }
        err = gpio_direction_input(gpio);
        if (err) {
            printk(KERN_ERR"%s: gpio_direction_input failed for intr %d\n",name, gpio);
            gpio_free(gpio);
            return err;
        }
        return MSM_GPIO_TO_INT(gpio);
    }
    else
    {
        gpio_free(gpio);
        return 0;
    }
}

/* delete function enable_vaux2_power_for_device,needn't control avdd in driver */
 static int i2c_power_init(void)
{
/*add 2 power supply for TP and Sensors*/
	struct regulator *lvs4,*lvs6;
	struct regulator *l9;
/*for pmic gpio17 control SENSOR_2P85_EN output */
	bool ret = 0;
	struct pm_gpio param = {
		.direction		= PM_GPIO_DIR_OUT,
		.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
		.output_value	= 1,
		.pull			= PM_GPIO_PULL_NO,
		.vin_sel		= PM_GPIO_VIN_S4,
		.out_strength	= PM_GPIO_STRENGTH_NO,
		.function		= PM_GPIO_FUNC_NORMAL,
	};
	ret = gpio_request(PM8921_GPIO_PM_TO_SYS(SENSOR_2P85_EN), "SENSOR_2P85_EN");
	ret = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(SENSOR_2P85_EN), &param);
	ret = gpio_direction_output(PM8921_GPIO_PM_TO_SYS(SENSOR_2P85_EN), 1);
	mdelay(2);
	lvs4 = regulator_get(NULL, "8921_lvs4");
	if (!IS_ERR(lvs4))
	{
		regulator_enable(lvs4);
		pr_info("get pm8921_chg_reg_switch\n");
	}
	else
	{
		pr_info("Unable to get pm8921_chg_reg_switch\n");
		return -1;
	}
	lvs6= regulator_get(NULL, "8921_lvs6");
	if (!IS_ERR(lvs6))
	{
		regulator_enable(lvs6);
		pr_info("get pm8921_chg_reg_switch\n");
	}
	else
	{
		pr_info("Unable to get pm8921_chg_reg_switch\n");
		return -1;
	}
	l9 = regulator_get(NULL, "8921_l9");
	if (!IS_ERR(l9))
	{
		regulator_enable(l9);
		pr_info("get pm8921_chg_reg_switch\n");
	}
	else
	{
		pr_info("Unable to get pm8921_chg_reg_switch\n");
		return -1;

	}

	regulator_set_voltage(lvs4, 1800000, 1800000);
	regulator_set_voltage(lvs6, 1800000, 1800000);
	regulator_set_voltage(l9, 2850000, 2850000);
	return 0;
} 
#ifdef CONFIG_TOUCHSCREEN_RMI4_SYNAPTICS
atomic_t touch_detected_yet = ATOMIC_INIT(0);

/*
 *use the touch_gpio_config_interrupt to config the gpio
 *which we used, but the gpio number can't exposure to user
 *so when the platform or the product changged please self self adapt
 */
int touch_power(int on)
{
	
	return 0;
}
int touch_gpio_config_interrupt(int enable)
{
    int gpio_tp_irq = GET_GPIO_FAIL;
    gpio_tp_irq = get_gpio_num_by_name("TP_INT");
    if(gpio_tp_irq <= GET_GPIO_FAIL)
		return GET_GPIO_FAIL;
    HW_DEV_DBG("gpio_tp_irq:%d enable:%d\n",gpio_tp_irq,enable);
    return gpio_config_interrupt(gpio_tp_irq,"touch_irq",enable);
}
/*
 *the fucntion set_touch_probe_flag when the probe is detected use this function can set the flag ture
 */

void set_touch_probe_flag(int detected)/*we use this to detect the probe is detected*/
{
    if(detected >= 0)
    {
	atomic_set(&touch_detected_yet, 1);
    }
    else
    {
	atomic_set(&touch_detected_yet, 0);
    }
    return;
}

/*
 *the fucntion read_touch_probe_flag when the probe is ready to detect first we read the flag
 *if the flag is set ture we will break the probe else we
 *will run the probe fucntion
 */

int read_touch_probe_flag(void)
{
    int ret = 0;
    ret = atomic_read(&touch_detected_yet);
    return ret;
}

/* add gpio free */
/*this function reset touch panel */
int touch_reset(void)
{
    int ret = 0;
    int gpio_tp_rst = GET_GPIO_FAIL;
    gpio_tp_rst = get_gpio_num_by_name("TP_RST");
    if(gpio_tp_rst <= GET_GPIO_FAIL)
		return GET_GPIO_FAIL;
    HW_DEV_DBG("gpio_tp_rst:%d\n",gpio_tp_rst);
    gpio_request(gpio_tp_rst,"TOUCH_RESET");
    //ret = gpio_tlmm_config(GPIO_CFG(TOUCH_MSM8960_RESET_PIN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
    ret = gpio_direction_output(gpio_tp_rst, 1);
    mdelay(5);
    ret = gpio_direction_output(gpio_tp_rst, 0);
    mdelay(10);
    ret = gpio_direction_output(gpio_tp_rst, 1);
    mdelay(50);//must more than 10ms.
    gpio_free(gpio_tp_rst);

	return 0;//ret;
}
int get_touch_reset_pin(void)
{
	int rst_pin = GET_GPIO_FAIL;
	rst_pin = get_gpio_num_by_name("TP_RST");
       if(rst_pin <= GET_GPIO_FAIL)
		return GET_GPIO_FAIL;
	return rst_pin;
}

int get_touch_irq_pin(void)
{
	int irq_pin = GET_GPIO_FAIL;
	irq_pin = get_gpio_num_by_name("TP_INT");
       if(irq_pin <= GET_GPIO_FAIL)
		return GET_GPIO_FAIL;
	return irq_pin;
}

/*this function get the tp  resolution*/
static int get_phone_version(struct tp_resolution_conversion *tp_resolution_type)
{
    int ret = 0;
/*Add TP self adapt*/
    char version[MAX_TP_VERSION_LEN];
    get_tp_resolution(version,MAX_TP_VERSION_LEN);
    if(!strncmp(version, "WVGA",MAX_TP_VERSION_LEN))
    {
        tp_resolution_type->lcd_x = LCD_X_WVGA;
        tp_resolution_type->lcd_y = LCD_Y_WVGA;
        tp_resolution_type->jisuan = LCD_JS_WVGA;
    }
    if(!strncmp(version, "720P",MAX_TP_VERSION_LEN))
    {
        tp_resolution_type->lcd_x = LCD_X_720P;
        tp_resolution_type->lcd_y = LCD_Y_720P;
        tp_resolution_type->jisuan = LCD_JS_720P;
    }
    ret = touch_reset();
    if(ret)
    {
	    printk(KERN_ERR "%s: reset failed \n", __func__);
        return -1;
    }
    return 0;
}

static struct tp_i2c_platform_data synaptics_tp_platform_data = {
    .power_on = NULL,
    .power_off = NULL,
    .get_gpio_irq = get_touch_irq_pin,
    .touch_gpio_config_interrupt = touch_gpio_config_interrupt,
    .set_touch_probe_flag = set_touch_probe_flag,
    .read_touch_probe_flag = read_touch_probe_flag,
    .touch_reset = touch_reset,
    .get_touch_reset_pin = get_touch_reset_pin,
    .get_phone_version = get_phone_version,
    .tp_get_config_data = NULL,
};
#endif
#ifdef CONFIG_TOUCHSCREEN_VIRTUALKEY
/* add the virtual key driver*/
static char buf_virtualkey[500];
static ssize_t  buf_vkey_size=0;
/* add the virtual key driver*/
static ssize_t virtual_keys_show(struct kobject *kobj,
			       struct kobj_attribute *attr, char *buf)
{
        memcpy( buf, buf_virtualkey, buf_vkey_size );
		return buf_vkey_size;
}

static struct kobj_attribute virtual_keys_attr[] ={
	{
	.attr = {
		.name = "virtualkeys.synaptics",
		.mode = S_IRUGO,
		},
		.show = &virtual_keys_show,
	},
	{
		.attr = {
			.name = "virtualkeys.atmel-touchscreen",
			.mode = S_IRUGO,
		},
		.show = &virtual_keys_show,
	}
};

static struct attribute *properties_attrs[] = {
	&virtual_keys_attr[0].attr,
	&virtual_keys_attr[1].attr,
	NULL
};
static struct attribute_group properties_attr_group = {
	.attrs = properties_attrs,
};
static void __init virtualkeys_init(void)
{
    struct kobject *properties_kobj;
    int ret;
/*Add TP self adapt*/
    char version[MAX_TP_VERSION_LEN];
    char keyorder[MAX_TK_KEYORDER_LEN];
    get_tp_resolution(version,MAX_TP_VERSION_LEN);
    get_touchkey_keyorder(keyorder,MAX_TK_KEYORDER_LEN);
    if(!strncmp(version, "WVGA",MAX_TP_VERSION_LEN))
    {
        if(!strncmp(keyorder, "MenuBack",MAX_TK_KEYORDER_LEN))
        {
            buf_vkey_size = sprintf(buf_virtualkey,
                    __stringify(EV_KEY) ":" __stringify(KEY_MENU)  ":65:850:170:100"
                    ":" __stringify(EV_KEY) ":" __stringify(KEY_HOME)   ":240:850:170:100"
                    ":" __stringify(EV_KEY) ":" __stringify(KEY_BACK)   ":415:850:170:100"
                    "\n");
    	}
	    if(!strncmp(keyorder, "BackMenu",MAX_TK_KEYORDER_LEN))
	    {
            buf_vkey_size = sprintf(buf_virtualkey,
                    __stringify(EV_KEY) ":" __stringify(KEY_MENU)  ":415:850:170:100"
                    ":" __stringify(EV_KEY) ":" __stringify(KEY_HOME)   ":240:850:170:100"
                    ":" __stringify(EV_KEY) ":" __stringify(KEY_BACK)   ":65:850:170:100"
                    "\n");
	    }
    }
    if(!strncmp(version, "720P",MAX_TP_VERSION_LEN))
    {
        buf_vkey_size = sprintf(buf_virtualkey,
				__stringify(EV_KEY) ":" __stringify(KEY_HOME)  ":360:1380:170:150"
				 ":" __stringify(EV_KEY) ":" __stringify(KEY_MENU)   ":600:1380:170:150"
				 ":" __stringify(EV_KEY) ":" __stringify(KEY_BACK)   ":120:1380:170:150"
  //      		   ":" __stringify(EV_KEY) ":" __stringify(KEY_SEARCH) ":620:1325:180:70"
                   "\n");
    }
	properties_kobj = kobject_create_and_add("board_properties", NULL);
	if (properties_kobj)
	/* to unify all of touch screen drivers, just support synaptics */
		ret = sysfs_create_group(properties_kobj,
					 &properties_attr_group);
	if (!properties_kobj || ret)
		pr_err("failed to create board_properties\n");
}
#endif

#ifdef CONFIG_TOUCHSCREEN_MXT224E_ATMEL
/*we have 2 sizes of atmel tp, so we use array to store these configs*/
static struct atmel_data atmel_tp_data[] ={
{
    .version = 0x10,
    .source = 0,
    .abs_x_min = 0x00,
    .abs_x_max = 719,
    .abs_y_min = 0x00,
    .abs_y_max = 1279,
    .abs_pressure_min = 0x00,
    .abs_pressure_max = 150,
    .abs_width_min = 0,
    .abs_width_max = 20,
    .config_T6 = {0, 0, 0, 0, 0,
    0},
    .config_T7 = {32, 255, 15},/*40HZ->80HZ*/
    .config_T8 = {21, 0, 1, 10, 0,
    0, 5, 70, 20, 192},
    .config_T9 = {139, 0, 0, 19, 11,
    0, 32, 80, 2, 3,
    0, 5, 2, 47, 10,
    15, 22, 10, 106, 5,/*XRANGE = 1386*/
    207, 2, 0, 0, 0,/* YRANGE = 719*/
    0, 161, 40, 183, 64,
    30, 20, 0, 0, 1},
    .config_T15 = {0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0},
    .config_T19 = {0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0},
    .config_T23 = {0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0},
    .config_T25 = {0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0},
    .config_T40 = {0, 0, 0, 0, 0},
    .config_T42 = {0, 40, 60, 80, 128,
    0, 0, 0},

    .config_T47 = {0, 20, 50, 5, 2,
    40, 40, 180, 0, 100},

//   ***********USB disconnected******
    .config_T46 = {0, 3, 32, 32, 0,
    0, 0, 0, 0},
    .config_T48 = {1, 4, 10, 1, 0,
    0, 0, 0, 1, 5,
    0, 0, 0, 6, 6,
    0, 0, 63, 6, 64,
    10, 0, 20, 5, 0,
    38, 0, 20, 0, 0,
    0, 0, 0, 0, 0,
    40, 2, 5, 2, 32,
    10, 12, 20, 241, 251,
    0, 0, 191, 40, 183,
    64, 30, 15, 0},
//   ***********USB connected******
    .cable_config_T46 = {0, 3, 40, 40, 0,
    0, 0, 0, 0},
    .cable_config_T48={1, 128, 114, 1, 0,
    0, 0, 0, 1, 5,
    0, 0, 0, 6, 6,
    0, 0, 63, 6, 64,
    10, 0, 20, 5, 0,
    38, 0, 20, 0, 0,
    0, 0, 0, 0, 0,
    40, 2, 5, 2, 32,
    10, 12, 20, 241, 251,
    0,0, 191, 40, 183,
    64, 30, 15, 0},
},
{
    .version = 0x10,
    .source = 0,
    .abs_x_min = 0x00,
    .abs_x_max = 539,//959,
    .abs_y_min = 0x00,
    .abs_y_max = 959,//539,
    .abs_pressure_min = 0x00,
    .abs_pressure_max = 120,
    .abs_width_min = 0,
    .abs_width_max = 20,
        .config_T6 = {0, 0, 0, 0, 0,
    0},
    .config_T7 = {32, 12, 25},/*40HZ->80HZ*/
    .config_T8 = {21, 0, 5, 5, 0,
    0, 1, 55, 10, 192},
    .config_T9 = {139, 0, 0, 19, 11,
    0, 32, 55, 1, 1,
    0, 5, 2, 45, 10,
    12, 20, 10, 191, 3,
    27, 2, 0, 0, 0,
    0, 170, 36, 151, 90,
    25, 15, 0, 0, 0},
    .config_T15 = {0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0},
    .config_T19 = {0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0},
    .config_T23 = {0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0},
    .config_T25 = {0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0},
    .config_T40 = {0, 0, 0, 0, 0},
    .config_T42 = {0, 40, 40, 80, 128,
    0, 0, 0},
    .config_T47 = {0, 20, 50, 5, 2,
    40, 40, 180, 0, 100},
    //   ***********  USB disconnected
    .config_T46 = {0, 3, 48, 63, 0,
    0, 0, 0, 0},
    .config_T48 = {1, 4, 66, 0, 0,
    0, 0, 0, 5, 9,
    0, 0, 0, 6, 6,
    0, 0, 63, 6, 64,
    10, 0, 20, 5, 0,
    38, 0, 20, 0, 0,
    0, 0, 0, 0, 0,
    50, 2, 3, 1, 0,
    5, 10, 40, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0},
//  ******** USB connected
    .cable_config_T46 = {4, 3, 32, 32, 0,
    0, 0, 0, 0},
    .cable_config_T48={1, 128, 114, 0, 0,
    0, 0, 0, 5, 9,
    0, 0, 0, 10, 10,
    0, 0, 63, 6, 64,
    0, 0, 0, 0, 0,
    0, 0, 20, 0, 0,
    0, 0, 0, 0, 0,
    38, 2, 5, 2, 15,
    10, 12, 20, 242, 242,
    0,0, 158, 48, 151,
    90, 25, 15, 0},
}
};
/*add this fuction for which atmel's tp configs we choose */
void* atmel_tp_get_config_data(void)
{
    char version[MAX_TP_VERSION_LEN];
    get_tp_resolution(version,MAX_TP_VERSION_LEN);
    if(!strncmp(version, "720P",MAX_TP_VERSION_LEN))
    {
        return &atmel_tp_data[0];
    }
    if(!strncmp(version, "QHD",MAX_TP_VERSION_LEN))
    {
        return &atmel_tp_data[1];
    }
    return &atmel_tp_data[1];
}
/*separate platform data and configs*/
static struct tp_i2c_platform_data atmel_tp_platform_data = {
    .power_on = NULL,
    .power_off = NULL,
    .get_gpio_irq = get_touch_irq_pin,
    .touch_gpio_config_interrupt = touch_gpio_config_interrupt,
    .set_touch_probe_flag = set_touch_probe_flag,
    .read_touch_probe_flag = read_touch_probe_flag,
    .touch_reset = touch_reset,
    .get_touch_reset_pin = get_touch_reset_pin,
    .get_phone_version = NULL,
    .tp_get_config_data = atmel_tp_get_config_data,
};
#endif
int power_for_device(struct device* dev)
{    
   	return 0;
}

int gsensor_interrupt_config(int enable)
{
    int gpio_gs_irq = GET_GPIO_FAIL;
    gpio_gs_irq = get_gpio_num_by_name("GSENSOR_INT1");
    if(gpio_gs_irq <= GET_GPIO_FAIL)
		return GET_GPIO_FAIL;
    HW_DEV_DBG("gpio_gs_irq:%d enable:%d\n",gpio_gs_irq,enable);
    return gpio_config_interrupt(gpio_gs_irq,"gsensor_irq",enable);
}
int compass_interrupt_config(int enable)
{
    int gpio_compass_irq = GET_GPIO_FAIL;
    gpio_compass_irq = get_gpio_num_by_name("COMPASS_INT");
    if(gpio_compass_irq <= GET_GPIO_FAIL)
		return GET_GPIO_FAIL;
    HW_DEV_DBG("gpio_compass_irq:%d enable:%d\n",gpio_compass_irq,enable);
    return gpio_config_interrupt(gpio_compass_irq,"compass_irq",enable);
}
/* add new driver for light-sensor */
#ifdef CONFIG_HUAWEI_FEATURE_PROXIMITY_APDS990X
int apds990x_interrupt_config(int enable)
{
    int gpio_prox_irq = GET_GPIO_FAIL;
    gpio_prox_irq = get_gpio_num_by_name("PROXIMITY_INT_N");
    if(gpio_prox_irq <= GET_GPIO_FAIL)
		return GET_GPIO_FAIL;
    HW_DEV_DBG("gpio_prox_irq:%d enable:%d\n",gpio_prox_irq,enable);
    return gpio_config_interrupt(gpio_prox_irq,"prox_irq",enable);
}
static uint16_t lsensor_adc_table[][LSENSOR_MAX_LEVEL] = {
	{1, 3, 50, 130, 220, 300, 1000},//softbank
	{6, 30, 500, 850, 1000, 1400, 2000},//docomo
	{2, 5, 15, 400, 600, 950, 1200},//verision
	{2, 5, 15, 400, 600, 950, 1200},//Cricket
};
static unsigned int apx_threshold[]= {
	800, 450,//softbank
	800,450,//docomo
	800,450,//verison
	800,450,//Cricket
};
void * get_lsensor_adc_tbl(int *pthl,int *pthh)
{
	char ls_ptype[LSENSOR_PNAME_LEN] = {0};
	get_lightsensor_type(ls_ptype,LSENSOR_PNAME_LEN);
	//printk("%s enter:%s\n",__func__,ls_ptype);
	if(!strncmp(ls_ptype,"product_softbank",LSENSOR_PNAME_LEN))
	{
		*pthl = apx_threshold[0];
		*pthh = apx_threshold[1];
		return 	lsensor_adc_table[0];
	}
	else if(!strncmp(ls_ptype,"product_docomo",LSENSOR_PNAME_LEN))
	{
		*pthl = apx_threshold[2];
		*pthh = apx_threshold[3];
		return 	lsensor_adc_table[1];
	}
	else if(!strncmp(ls_ptype,"product_verizion",LSENSOR_PNAME_LEN))
	{
		*pthl = apx_threshold[4];
		*pthh = apx_threshold[5];
		return 	lsensor_adc_table[2];
	}
	else if(!strncmp(ls_ptype,"product_Cricket",LSENSOR_PNAME_LEN))
	{
		*pthl = apx_threshold[6];
		*pthh = apx_threshold[7];
		return 	lsensor_adc_table[3];
	}

	*pthl = apx_threshold[0];
	*pthh = apx_threshold[1];
	return 	lsensor_adc_table[0];
}
struct apds990x_platform_data apds990x_platform_data = {
    .gpio_config_interrupt = apds990x_interrupt_config,
    .get_adc_tbl = get_lsensor_adc_tbl,
};
#endif
/*modify for adi346 unstable in interrupt mode*/
#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_ACCELEROMETER_ADI_ADXL346
static struct adxl34x_platform_data gs_adi346_platform_data= {
	.tap_threshold = 35,
	.tap_duration = 3,
	.tap_latency = 20,
	.tap_window = 20,
	//.tap_axis_control = ADXL_TAP_X_EN | ADXL_TAP_Y_EN | ADXL_TAP_Z_EN,
	.act_axis_control = 0xFF,
	.activity_threshold = 6,
	.inactivity_threshold = 4,
	.inactivity_time = 3,
	.free_fall_threshold = 8,
	.free_fall_time = 0x20,
	.data_rate = 0,
	//.data_rate = 8,
	.data_range = ADXL_FULL_RES,

	.ev_type = EV_ABS,
	.ev_code_x = ABS_X,	/* EV_REL */
	.ev_code_y = ABS_Y,	/* EV_REL */
	.ev_code_z = ABS_Z,	/* EV_REL */

	.ev_code_tap_x = BTN_TOUCH,	/* EV_KEY */
	.ev_code_tap_y = BTN_TOUCH,	/* EV_KEY */
	.ev_code_tap_z = BTN_TOUCH,	/* EV_KEY */
	.power_mode = ADXL_AUTO_SLEEP/* | ADXL_LINK*/,
	.fifo_mode = FIFO_BYPASS,
	.watermark = 0,
	.power_on = power_for_device,
	.gpio_config_interrupt = gsensor_interrupt_config,
};
#endif


#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_ACCELEROMETER_ST_LIS3XH
static struct lis3dh_acc_platform_data gs_st_lis3xh_platform_data = {

	.poll_interval = 10,
	.min_interval = 10,
	.g_range = 0x00,

	.axis_map_x = 0,
	.axis_map_y = 1,
	.axis_map_z = 2,
	.negate_x = 0,
	.negate_y = 0,
	.negate_z = 0,

    .gpio_int1 = -1,
    .gpio_int2 = -1,

};
#endif
#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_ACCELEROMETER_MMA8452
struct mma8452_acc_platform_data mma8452_acc_platform_data = {
    .power_on = power_for_device,
};
#endif
#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AK8975
static struct akm8975_platform_data akm8975_compass_platform_data = {
       .gpio_DRDY = 14,
	.gpio_config_interrupt = compass_interrupt_config,
	.power_on = power_for_device,
	.layout = 3,
	.outbit = 1,
};
#endif
#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AK8963
static struct akm8963_platform_data akm8963_compass_platform_data = {
	.gpio_DRDY = 70,
	.gpio_config_interrupt = compass_interrupt_config,
	.power_on = power_for_device,
	.gpio_RST = 48,
	.layout = 3,
	.outbit = 1,
};
#endif
#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_YAMAHA_COMPASS
static struct akm8963_platform_data yamaha_compass_platform_data = {
	.gpio_DRDY = 70,
	.gpio_config_interrupt = compass_interrupt_config,
	.power_on = power_for_device,
	.gpio_RST = 48,
	.layout = 3,
	.outbit = 1,
};
#endif
#ifdef CONFIG_HUAWEI_FEATURE_GYROSCOPE_L3G4200DH
static struct l3g4200d_gyr_platform_data l3g4200d_gyr_platform_data = {
	.poll_interval = 10,
	.min_interval =10,
	
	.fs_range = 0x30,
	
	.axis_map_x = 0,
	.axis_map_y = 1,
	.axis_map_z = 2,
	
	.negate_x = 1,
	.negate_y = 1,
	.negate_z = 0,
};
#endif
#ifdef CONFIG_HUAWEI_FEATURE_PROXIMITY_EVERLIGHT_TMD2771
struct tmd2771_platform_data tmd2771_platform_data = {
	.power_on = power_for_device,
};
#endif
int touchkey_irq_config(int enable)
{
    int gpio_tk_irq = GET_GPIO_FAIL;
    gpio_tk_irq = get_gpio_num_by_name("CAPKEY_INT");
    if(gpio_tk_irq <= GET_GPIO_FAIL)
		return GET_GPIO_FAIL;
    HW_DEV_DBG("gpio_tk_irq:%d enable:%d\n",gpio_tk_irq,enable);
	return gpio_config_interrupt(gpio_tk_irq,"touch_key_irq",enable);
}
/*Add TK reset*/
/*this function reset touchkey */
int touchkey_reset(void)
{
	int ret = 0;
	int gpio_tk_rst = GET_GPIO_FAIL;
	gpio_tk_rst = get_gpio_num_by_name("CAPKEY_RST_N");
	if(gpio_tk_rst <= GET_GPIO_FAIL)
		return GET_GPIO_FAIL;
	HW_DEV_DBG("gpio_tk_rst:%d\n",gpio_tk_rst);
	gpio_request(gpio_tk_rst,"TOUCHKEY_RESET");
	mdelay(10);
	ret = gpio_direction_output(gpio_tk_rst, 0);
	mdelay(10);
	ret = gpio_direction_output(gpio_tk_rst, 1);
	mdelay(30);//must more than 10ms.
	gpio_free(gpio_tk_rst);
	return 0;//ret;
}
static unsigned short qt1060_key1code[] =
{
	KEY_BACK,//158
	KEY_MENU,//139
	KEY_HOME,//102
	//	KEY_SEARCH//217
};
static unsigned short qt1060_key2code[] =
{
	KEY_MENU,//139
	KEY_BACK,//158
	KEY_HOME,//102
};
unsigned short* get_key_map(void)
{
	char keyorder[MAX_TK_KEYORDER_LEN];
	get_touchkey_keyorder(keyorder,MAX_TK_KEYORDER_LEN);
	if(!strncmp(keyorder, "MenuBack",MAX_TK_KEYORDER_LEN))
	{
		return qt1060_key1code;
	}
	if(!strncmp(keyorder, "BackMenu",MAX_TK_KEYORDER_LEN))
	{
		return qt1060_key2code;
	}
	return qt1060_key1code;
}
#ifdef CONFIG_HUAWEI_FEATURE_TOUCHKEY_ATMEL
static struct touchkey_platform_data tk_atmel1060_platform_data = {
	.touchkey_gpio_config_interrupt = touchkey_irq_config,
	.touchkey_reset = touchkey_reset,
	.get_key_map = get_key_map,
	.get_sensitive = NULL,
};
#endif

static struct i2c_board_info i2c_devices_gsbi2[]={

#ifdef CONFIG_HUAWEI_FEATURE_TOUCHKEY_ATMEL
    {
       I2C_BOARD_INFO("qt1060",0x12), 
	 .platform_data = &tk_atmel1060_platform_data,
    },
#endif
};
static struct i2c_board_info i2c_devices_gsbi12[] = {

#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_ACCELEROMETER_ADI_ADXL346
    {
        I2C_BOARD_INFO("adxl34x", 0xA6>>1),  /* actual address 0xA6, fake address 0xA8*/
	    .platform_data = &gs_adi346_platform_data,		
    },
#endif

#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AK8975
    {
        I2C_BOARD_INFO("akm8975", 0x18 >> 1),//7 bit addr, no write bit
        .platform_data = &akm8975_compass_platform_data,
    },
#endif
#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AK8963
    {
        I2C_BOARD_INFO("akm8963", 0x1c >> 1),//7 bit addr, no write bit
        .platform_data = &akm8963_compass_platform_data,
    },
#endif
#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_YAMAHA_COMPASS
    {
        I2C_BOARD_INFO("yamaha_geomagnetic", 0x2e),//7 bit addr, no write bit
        .platform_data = &yamaha_compass_platform_data,
    },
#endif		
#ifdef CONFIG_HUAWEI_FEATURE_PROXIMITY_EVERLIGHT_TMD2771
    {
        I2C_BOARD_INFO("tmd2771", 0x72 >> 1),
		.platform_data = &tmd2771_platform_data,
    },
#endif
#ifdef CONFIG_HUAWEI_FEATURE_PROXIMITY_APDS990X
	{
		I2C_BOARD_INFO("apds990x", 0x72 >> 1),
		.platform_data = &apds990x_platform_data,
	},
#endif
#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_ACCELEROMETER_MMA8452
    {
        I2C_BOARD_INFO("mma8452", 0x38 >> 1), 
		.platform_data=&mma8452_acc_platform_data,		
    },
#endif
#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_ACCELEROMETER_ST_LIS3XH
    {
        I2C_BOARD_INFO("lis3dh_acc", 0x30 >> 1),
	    .platform_data = &gs_st_lis3xh_platform_data,
    },
#endif

#ifdef CONFIG_HUAWEI_FEATURE_GYROSCOPE_L3G4200DH
    {
		I2C_BOARD_INFO("l3g4200d_gyr", 0x68),
		.platform_data = &l3g4200d_gyr_platform_data,
    },
#endif
};
/* change i2c bus name */
static struct i2c_board_info i2c_devices_gsbi3[] = {
#ifdef CONFIG_TOUCHSCREEN_MXT224E_ATMEL
   {
		I2C_BOARD_INFO("atmel_qt602240", 0x4a),
        .platform_data = &atmel_tp_platform_data,
   },
#endif

#ifdef CONFIG_TOUCHSCREEN_RMI4_SYNAPTICS
	{
		I2C_BOARD_INFO("Synaptics_rmi", 0x70),
        .platform_data = &synaptics_tp_platform_data,
        .flags = true, //this flags is the switch of the muti_touch
	},
#endif
};

static struct i2c_board_info i2c_devices_gsbi5[] = {
#ifdef CONFIG_BATTERY_BQ275x0
	{
		I2C_BOARD_INFO("bq275x0-battery",0x55),	//(0xAA >> 1)
	},
#endif	
    {
        I2C_BOARD_INFO("TK_backlight_ctrl", 0x45),
    },

};

/* change i2c bus name */
static int __devinit hw_devices_init(void)
{
    printk("all hw_devices_init\n");
    i2c_power_init();
    i2c_register_board_info(MSM_8960_GSBI12_QUP_I2C_BUS_ID, i2c_devices_gsbi12, ARRAY_SIZE(i2c_devices_gsbi12));
#ifdef CONFIG_BATTERY_BQ275x0
    if(is_use_fuel_gauge())
    {
    	i2c_register_board_info(MSM_8960_GSBI5_QUP_I2C_BUS_ID, i2c_devices_gsbi5, ARRAY_SIZE(i2c_devices_gsbi5));
    }
#endif
    i2c_register_board_info(MSM_8960_GSBI3_QUP_I2C_BUS_ID, i2c_devices_gsbi3, ARRAY_SIZE(i2c_devices_gsbi3));
    i2c_register_board_info(MSM_8960_GSBI2_QUP_I2C_BUS_ID, i2c_devices_gsbi2, ARRAY_SIZE(i2c_devices_gsbi2));
/*add macro to control this fuction*/
/* add the virtual key driver*/
#ifdef CONFIG_TOUCHSCREEN_VIRTUALKEY
    virtualkeys_init();
#endif
	return 0;
}
arch_initcall(hw_devices_init);

MODULE_DESCRIPTION("hawei devices init");
MODULE_LICENSE("GPL");

