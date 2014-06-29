/* Copyright (c) 2011, Huawei co. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */



//#include <linux/i2c.h>
//#include <linux/i2c/sx150x.h>
#include <asm/mach-types.h>
#include <mach/board.h>
#include <mach/msm_bus_board.h>
#include <mach/gpio.h>
#include <mach/gpiomux.h>
#include "devices.h"
#include "board-8960.h"

#include <hsad/config_interface.h>  

#include <mach/msm_serial_hs.h> 
#ifdef GSBI11_HIGH_SPEED
#include <mach/msm_iomap.h>
#include <mach/scm-io.h>
#define TCSR_ADM_0_A_CRCI_MUX_SEL        (MSM_TCSR_BASE + 0x70)
#define ADM0_CRCI_GSBI11_RX_SEL (1<<11)
#define ADM0_CRCI_GSBI11_TX_SEL (1<<12)
#define ADM0_CRCI_GSBI11_MASK (ADM0_CRCI_GSBI11_RX_SEL|ADM0_CRCI_GSBI11_TX_SEL)
#endif

//added for using new tz begin
#define NEW_TZ_DMOV_HSUART_GSBI11_TX_CHAN 9
#define NEW_TZ_DMOV_HSUART_GSBI11_RX_CHAN 8
//added for using new tz end

//#define BT_REG_ON PM8921_GPIO_PM_TO_SYS (21)
static int bt_reg_on=0; 
enum
{
	BCM_MSM2BT_WAKE_GPIO,
	BCM_BT_RST_N_GPIO,
	BCM_BT2MSM_WAKE_GPIO,
};
static struct bt_config_gpio
{
	char* name;
	int data;
}bt_config_gpio_array[] = {
	{"BCM_MSM2BT_WAKE",0},
	{"BCM_BT_RST_N",0},
	{"BCM_BT2MSM_WAKE",0},
};
static struct gpiomux_setting gsbi11 = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config_data
{
	struct msm_gpiomux_config msm8960_bluetooth_configs;
	char* name;
} msm_gpiomux_config_data_array[]={
   	{
		.msm8960_bluetooth_configs = {
		.gpio      = 0,	
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi11,
			[GPIOMUX_ACTIVE] = &gsbi11,
			},
		},
		.name = "GSBI11_3",
	},
	{
		.msm8960_bluetooth_configs = {
		.gpio      = 0,	
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi11,
			[GPIOMUX_ACTIVE] = &gsbi11,
			},
		},
		.name = "GSBI11_2",
	},
	{
		.msm8960_bluetooth_configs = {
			.gpio      = 0,	
			.settings = {
				[GPIOMUX_SUSPENDED] = &gsbi11,
				[GPIOMUX_ACTIVE] = &gsbi11,
			},
		},
		.name = "GSBI11_1",
	},
	{
		.msm8960_bluetooth_configs = {
			.gpio      = 0,	
			.settings = {
				[GPIOMUX_SUSPENDED] = &gsbi11,
				[GPIOMUX_ACTIVE] = &gsbi11,
			},
		},
		.name = "GSBI11_0",
	},		
};


static struct platform_device msm_bt_power_device = {
	.name = "bt_power",
};

static struct resource bluesleep_resources[] = {
	{
		.name	= "gpio_host_wake",
		.start	= 0,
		.end	= 0,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "gpio_ext_wake",
		.start	= 0,
		.end	= 0,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "host_wake",
		.start	= 0,
		.end	= 0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device msm_bluesleep_device = {
	.name = "bluesleep",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(bluesleep_resources),
	.resource	= bluesleep_resources,
};

static struct bt_config_power
{
	unsigned bt_config_power_on;
	unsigned bt_config_power_off;
}bt_config_power_array[] = {
	{GPIO_CFG(0, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG(0, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA)},// WAKE 
	{GPIO_CFG(0, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG(0, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA)},//BT_RST
	{GPIO_CFG(0, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG(0, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA)}// HOST_WAKE
};

static int bluetooth_power(int on)
{
	int pin, rc;

	printk(KERN_DEBUG "%s\n", __func__);
	if (on) 
	{
			for (pin = 0; pin < ARRAY_SIZE(bt_config_power_array); pin++) 
			{
			rc = gpio_tlmm_config( bt_config_power_array[pin].bt_config_power_on,
					      GPIO_CFG_ENABLE);
				if (rc) 
				{
				printk(KERN_ERR
				       "%s: gpio_tlmm_config(%#x)=%d\n",
				       __func__, bt_config_power_array[pin].bt_config_power_on, rc);
				return -EIO;
			}
		}
        printk(KERN_ERR "bt power on");
        rc = gpio_direction_output(PM8921_GPIO_PM_TO_SYS(bt_reg_on), 1);  /*bt on :BT_REG_ON -->1*/
        if (rc) 
        {
            printk(KERN_ERR "%s: gpio_direction_output BT power failed (%d)\n",
                   __func__, rc);
            return -EIO;
        }
		mdelay(50);

		rc = gpio_direction_output(bt_config_gpio_array[BCM_BT_RST_N_GPIO].data, 1);  /*bton:BT_RST -->1*/
		if (rc) {
			printk(KERN_ERR "%s: gpio_direction_output BT rst failed (%d)\n",
			       __func__, rc);
			return -EIO;
		}
		rc = gpio_direction_output(bt_config_gpio_array[BCM_MSM2BT_WAKE_GPIO].data, 1);
		if (rc) {
			printk(KERN_ERR "%s: gpio_direction_output BT msm2bt failed (%d)\n",
			       __func__, rc);
			return -EIO;
		}		
		mdelay(50);
	}
	else 

		{
	    rc = gpio_direction_output(bt_config_gpio_array[BCM_BT_RST_N_GPIO].data, 0);  /*bt off:BT_RST -->0*/
        if (rc) 
        {
            printk(KERN_ERR "%s:  bt power rst off fail (%d)\n",
                   __func__, rc);
            return -EIO;
        }

        rc = gpio_direction_output(PM8921_GPIO_PM_TO_SYS(bt_reg_on), 0);  /*bt_reg_on off on :BT_REG_ON -->0*/
        if (rc) 
        {
            printk(KERN_ERR "%s:  bt power off fail (%d)\n",
                   __func__, rc);
            return -EIO;
        }

		
	for (pin = 0; pin < ARRAY_SIZE(bt_config_power_array); pin++) {
					rc = gpio_tlmm_config(bt_config_power_array[pin].bt_config_power_off,
					      GPIO_CFG_ENABLE);
			if (rc) {
				printk(KERN_ERR
				       "%s: gpio_tlmm_config(%#x)=%d\n",
				       __func__, bt_config_power_array[pin].bt_config_power_off, rc);
				return -EIO;
			}
		}
	}
	return 0;
}
static void __init bt_power_init(void)
{
    int pin = 0, rc = 0 ,gpioget = 0;
	bluesleep_resources[0].start=bt_config_gpio_array[BCM_BT2MSM_WAKE_GPIO].data;
	bluesleep_resources[0].end=bt_config_gpio_array[BCM_BT2MSM_WAKE_GPIO].data;
	bluesleep_resources[1].start=bt_config_gpio_array[BCM_MSM2BT_WAKE_GPIO].data;
	bluesleep_resources[1].end=bt_config_gpio_array[BCM_MSM2BT_WAKE_GPIO].data;
	bluesleep_resources[2].start=bt_config_gpio_array[BCM_BT2MSM_WAKE_GPIO].data;
	bluesleep_resources[2].end=bt_config_gpio_array[BCM_BT2MSM_WAKE_GPIO].data;
	msm_bt_power_device.dev.platform_data = &bluetooth_power;
	if (gpio_request(bt_config_gpio_array[BCM_BT_RST_N_GPIO].data, "BT_POWER"))		
  		  printk("Failed to request gpio for BT_POWER\n");	
    
	if (gpio_request(PM8921_GPIO_PM_TO_SYS(bt_reg_on), "BT_REG_ON"))		
  		  printk("Failed to request gpio for BT_REG_ON\n");	

	//if (gpio_request(bt_config_gpio_array[BCM_BT2MSM_WAKE_GPIO].data, "BT2MSM_WAKE"))		
  		  //printk("Failed to request gpio for BT_REG_ON\n");	
	
	//if (gpio_request(bt_config_gpio_array[BCM_MSM2BT_WAKE_GPIO].data, "MSM2BT_WAKE"))		
  		  //printk("Failed to request gpio for BT_REG_ON\n");	

	for (pin = 0; pin < ARRAY_SIZE(bt_config_power_array); pin++) {
		rc = gpio_tlmm_config(bt_config_power_array[pin].bt_config_power_on,
				      GPIO_CFG_ENABLE);
		if (rc) {
			printk(KERN_ERR
			       "%s: gpio_tlmm_config(%#x)=%d\n",
			       __func__, bt_config_power_array[pin].bt_config_power_on, rc);
		}
	}
    if((gpioget=get_gpio_num_by_name("BCM_BT_RST_N"))<0)
	{
		printk(KERN_ERR "%s: can't get gpio num BCM_BT_RST_N\n",
		__func__);
	}
    rc = gpio_direction_output(gpioget, 0);  /*bt off:BT_RST -->0*/
    if (rc) 
    {
        printk(KERN_ERR "%s:  bt power off fail (%d)\n",
               __func__, rc);
    }

    rc = gpio_direction_output(PM8921_GPIO_PM_TO_SYS(bt_reg_on), 0);  /*bt_reg_on off on :BT_REG_ON -->0*/
    if (rc) 
    {
        printk(KERN_ERR "%s:  bt power off fail (%d)\n",
               __func__, rc);
    }


	for (pin = 0; pin < ARRAY_SIZE(bt_config_power_array); pin++) {
		rc = gpio_tlmm_config(bt_config_power_array[pin].bt_config_power_off,
				      GPIO_CFG_ENABLE);
		if (rc) {
			printk(KERN_ERR
			       "%s: gpio_tlmm_config(%#x)=%d\n",
			       __func__, bt_config_power_array[pin].bt_config_power_off, rc);
		}
	}

	printk(KERN_ERR"init bluetooth_power ");
}

static struct platform_device *common_devices[] __initdata = {
#ifdef GSBI11_LOW_SPEED 
	&msm8960_device_uart_gsbi11,
#endif
#ifdef GSBI11_HIGH_SPEED
	&msm_device_uart_dm11,
#endif
	&msm_bt_power_device, 
	&msm_bluesleep_device,
};

void __init msm8960_init_bluetooth(void)
{
	int gpioget=0;
	int i=0;
	for(i = 0; i < ARRAY_SIZE(msm_gpiomux_config_data_array); i++)
	{
		if((gpioget=get_gpio_num_by_name(msm_gpiomux_config_data_array[i].name))<0)
		{
			printk(KERN_ERR "%s: can't get gpio num (%s)\n",
                 	  __func__, msm_gpiomux_config_data_array[i].name);
                 	break;
		}
		msm_gpiomux_config_data_array[i].msm8960_bluetooth_configs.gpio = gpioget;
		msm_gpiomux_install(&msm_gpiomux_config_data_array[i].msm8960_bluetooth_configs,1);
	}
    //added for using new tz begin
    if(is_use_new_tz())
    {
     common_devices[0]->resource[3].start=NEW_TZ_DMOV_HSUART_GSBI11_TX_CHAN;
     common_devices[0]->resource[3].end=NEW_TZ_DMOV_HSUART_GSBI11_RX_CHAN;
    }
    //added for using new tz end
	platform_add_devices(common_devices, ARRAY_SIZE(common_devices));
	for (i = 0; i < ARRAY_SIZE(bt_config_gpio_array); i++) {
		if((gpioget=get_gpio_num_by_name(bt_config_gpio_array[i].name))<0)
		{
			printk(KERN_ERR "%s: can't get gpio num (%s)\n",
			__func__, bt_config_gpio_array[i].name);
		}
		bt_config_gpio_array[i].data = gpioget;
		bt_config_power_array[i].bt_config_power_on=(((bt_config_gpio_array[i].data & 0x3FF) << 4)| bt_config_power_array[i].bt_config_power_on);
		bt_config_power_array[i].bt_config_power_off=(((bt_config_gpio_array[i].data & 0x3FF) << 4)| bt_config_power_array[i].bt_config_power_off);	
	}
	if((bt_reg_on = get_pm_gpio_num_by_name("BCM_BT_REG_ON"))<0)
	{
		printk(KERN_ERR "%s: can't get gpio num BCM_BT_REG_ON\n",__func__);
	}
	bt_power_init();
}
