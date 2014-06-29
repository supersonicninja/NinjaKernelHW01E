/*
 * include/linux/touch_platfrom_config.h - platform data structure for touchscreen
 *
 * Copyright (C) 2010 Google, Inc.
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



#ifndef _TOUCH_PLATFORM_CONFIG_H
#define _TOUCH_PLATFORM_CONFIG_H

#define LCD_X_QVGA  	   320
#define LCD_Y_QVGA    	   240
#define LCD_X_HVGA         320
#define LCD_Y_HVGA         480
#define LCD_X_WVGA         480
#define LCD_Y_WVGA         800
#define LCD_JS_WVGA        882
#define LCD_X_720P         720
#define LCD_Y_720P         1280
#define LCD_JS_720P        1362
#define LCD_JS_HVGA        510

struct tp_resolution_conversion{
    int lcd_x;
    int lcd_y;
    int jisuan;
};

/*unify atmel platform_data struct and synaptics platform_data struct*/
struct tp_i2c_platform_data {
	int gpio_irq;
	int (*power_on)(struct device *dev, char *name);	/* Only valid in first array entry */
	int (*power_off)(void);
	int (*get_gpio_irq)(void);
	int (*touch_gpio_config_interrupt)(int enable);/*it will config the gpio*/
	void (*set_touch_probe_flag)(int detected);/*we use this to detect the probe is detected*/
	int (*read_touch_probe_flag)(void);/*when the touch is find we return a value*/
	int (*touch_reset)(void);
	int (*get_touch_reset_pin)(void);
	int (*get_phone_version)(struct tp_resolution_conversion *tp_resolution_type);/*add this function for judge the tp type*/
	void* (*tp_get_config_data)(void);
};

/*use new struct to manage platform data, separated config datas*/
struct atmel_data {
	uint16_t version;
	uint16_t source;
	uint16_t abs_x_min;
	uint16_t abs_x_max;
	uint16_t abs_y_min;
	uint16_t abs_y_max;
	uint8_t abs_pressure_min;
	uint8_t abs_pressure_max;
	uint8_t abs_width_min;
	uint8_t abs_width_max;
	int8_t config_T6[6];
	int8_t config_T7[3];
	int8_t config_T8[10];
	int8_t config_T9[35];
	int8_t config_T15[11];
	int8_t config_T19[16];
//	int8_t config_T20[12];
//	int8_t config_T22[17];
	int8_t config_T23[15];
	int8_t config_T24[19];
	int8_t config_T25[14];
//	int8_t config_T27[7];
//	int8_t config_T28[6];
    int8_t config_T40[5];
    int8_t config_T42[8];
    int8_t config_T46[9];
    int8_t config_T47[10];
    int8_t config_T48[54];
//	uint8_t object_crc[3];
//	int8_t cable_config[4];
//	int8_t cable_config_T7[3];
//	int8_t cable_config_T8[10];
//	int8_t cable_config_T9[35];
	int8_t cable_config_T46[9];
	int8_t cable_config_T48[54];
//	int8_t cable_config_T22[17];
//	int8_t cable_config_T28[6];
//	int8_t noise_config[3];
//	uint16_t filter_level[4];
//	uint8_t GCAF_level[5];
};

#endif /*_TOUCH_PLATFORM_CONFIG_H */
