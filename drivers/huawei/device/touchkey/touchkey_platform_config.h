/*
 * touchkey_platfrom_config.h - platform data structure for touchkey
 *
 * Copyright (C) 2011 huawei, Inc.
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
#ifndef _TOUCHKEY_PLATFORM_CONFIG_H
#define _TOUCHKEY_PLATFORM_CONFIG_H


struct touchkey_platform_data {
	int (*touchkey_gpio_config_interrupt)(int enable);/*it will config the gpio*/
	int (*touchkey_reset)(void);/* touch reset */
	unsigned short* (*get_key_map)(void);/*get key map*/
	u8 (*get_sensitive)(void);/*get sensitive*/
};


#endif /*_TOUCHKEY_PLATFORM_CONFIG_H */
