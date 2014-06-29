/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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



#include <linux/i2c.h>
#include <linux/i2c/sx150x.h>
#include <linux/interrupt.h>
#include <linux/interrupt.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/mfd/pm8xxx/pm8xxx-adc.h>
#include <linux/leds.h>
#include <linux/leds-pm8xxx.h>
#include <linux/msm_ssbi.h>
#include <asm/mach-types.h>
#include <mach/msm_bus_board.h>
#include <mach/restart.h>
#include "devices.h"
#include "board-8960.h"
#include <board-mahimahi-tpa2028d1.h> 
#include <hsad/config_interface.h>

#define I2C_SURF 1
#define I2C_FFA  (1 << 1)
#define I2C_RUMI (1 << 2)
#define I2C_SIM  (1 << 3)
#define I2C_FLUID (1 << 4)
#define I2C_DRAGON (1 << 5)

int  msm8960_init_audio_acdb(void)
{
	acdb_debugfs();
	return 0;
}

/*add TPA2028d1 for 8960*/
static struct tpa2028d1_platform_data tpa2028_data = {
	.gpio_tpa2028_spk_en = (PM8921_GPIO_PM_TO_SYS(18)),
};

static struct i2c_board_info dev_PA_i2c_devices[] __initdata = {
	{
		I2C_BOARD_INFO("tpa2028d1",0x58),
		.platform_data = &tpa2028_data,
	},	
};

struct i2c_registry {
	u8                     machs;
	int                    bus;
	struct i2c_board_info *info;
	int                    len;
};

static struct i2c_registry msm8960_i2c_devices[] __initdata = {
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_8960_GSBI5_QUP_I2C_BUS_ID,
		dev_PA_i2c_devices,
		ARRAY_SIZE(dev_PA_i2c_devices),
	},
};

void __init msm8960_init_audio_pa(void)
{
	int i = 0;
	for (i = 0; i < ARRAY_SIZE(msm8960_i2c_devices); ++i) 
	{
		i2c_register_board_info(msm8960_i2c_devices[i].bus,
			msm8960_i2c_devices[i].info,
			msm8960_i2c_devices[i].len);
	}
}

void __init msm8960_init_audio(void)
{
        msm8960_init_audio_acdb();
        msm8960_init_audio_pa();
}

