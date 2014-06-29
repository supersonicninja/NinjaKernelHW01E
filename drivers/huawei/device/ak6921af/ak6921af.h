/* Copyright (c) 2011, Huawei Technology Inc. All rights reserved.
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
 
#ifndef __AK6921AF_H__
#define __AK6921AF_H__

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/regulator/consumer.h>
#include <hsad/config_interface.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/types.h>

#define CMD_RELOAD                  0
#define CMD_REVERSE_OUTPUT          1
#define CMD_WRITE_ENABLE            2
#define CMD_WRITE_DISABLE           3

#define CMD_REG_EEPROM_SWITCH       4
#define REA0    0

#define CMD_GET_STATUS              5
#define OUT     0
#define MOD     1
#define WEN     2
#define POR     3

#define CMD_OUTPUT                  6
#define CTR0    0

#define CMD_SET_ADDRESS             7
#define S0      0
#define S1      1
#define S2      2

#define AK6921AF_I2C_RETRY_TIMES    10

#endif

 