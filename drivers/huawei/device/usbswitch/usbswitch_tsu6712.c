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
/*========================================
  * header file include
  *=======================================*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/bug.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <mach/gpio.h>
#include "usbswitch_tsu6712.h"


extern struct usbswitch_gpio_src usbswitch_gpio;	
extern struct i2c_client *i2cClient_usbswitch_main;


bool tsu6712_config_mhl_or_cradle(void)
{
	bool rtn = true;
	
	if(i2c_smbus_write_byte_data(i2cClient_usbswitch_main, TSU_MSW1, TSU_MSW1_USB)){
		rtn = false;
	}
	else{
		gpio_direction_output(usbswitch_gpio.sel1_gpio, 1);
		gpio_direction_output(usbswitch_gpio.sel2_gpio, 1);
		gpio_direction_output(usbswitch_gpio.id_gpio, 1);
		gpio_direction_output(usbswitch_gpio.mic_gpio, 0);
	}
	return rtn;
}

bool tsu6712_config_usb(void)
{
	bool rtn = true;
	
	if(i2c_smbus_write_byte_data(i2cClient_usbswitch_main, TSU_MSW1, TSU_MSW1_USB)){
		rtn = false;
	}
	else{
		gpio_direction_output(usbswitch_gpio.sel1_gpio, 0);
		gpio_direction_output(usbswitch_gpio.sel2_gpio, 1);
		gpio_direction_output(usbswitch_gpio.id_gpio, 0);
		gpio_direction_output(usbswitch_gpio.mic_gpio, 0);
	}
	return rtn;
}

bool tsu6712_config_headphone(void)
{
	bool rtn = true;

	if(i2c_smbus_write_byte_data(i2cClient_usbswitch_main, TSU_MSW1, TSU_MSW1_HEADPHONE)){
		rtn = false;
	}
	else{
		gpio_direction_output(usbswitch_gpio.sel1_gpio, 1);
		gpio_direction_output(usbswitch_gpio.sel2_gpio, 0);
		gpio_direction_output(usbswitch_gpio.id_gpio, 0);
		gpio_direction_output(usbswitch_gpio.mic_gpio, 1);
	}
	return rtn;
}

bool tsu6712_config_default(void)
{
	bool rtn = true;
	
	if(i2c_smbus_write_byte_data(i2cClient_usbswitch_main, TSU_MSW1, TSU_MSW1_USB)){
		rtn = false;
	}
	else{
		gpio_direction_output(usbswitch_gpio.sel1_gpio, 0);
		gpio_direction_output(usbswitch_gpio.sel2_gpio, 1);
		gpio_direction_output(usbswitch_gpio.id_gpio, 0);
		gpio_direction_output(usbswitch_gpio.mic_gpio, 0);
	}
	return rtn;	
}

static enum tsu_device_type tsu_read_device_type(void)
{
	enum tsu_device_type dev_type = TSU_DEVICE_NULL; 
	int read_val = 0;
	
	read_val = i2c_smbus_read_byte_data(i2cClient_usbswitch_main, TSU_DEVICE_TYPE1);	
	printk("usbswitch device type1 value : 0x%x\n", read_val);
	if(read_val>=0){
		switch(read_val&0xff){
			case TSU_DEVICE_USB:
				dev_type = TSU_DEVICE_USB;
				break;
			default:
				break;
		}
	}
#if 1	
	if(dev_type == TSU_DEVICE_NULL){
		read_val = i2c_smbus_read_byte_data(i2cClient_usbswitch_main, TSU_DEVICE_TYPE2);	
		printk("usbswitch device type2 value : 0x%x\n", read_val);
		if(read_val>=0){
			switch(read_val&0xff){
				case 0:
					break;
				default:
					break;
			}
		}
	}
#endif
	return dev_type;
}

bool tsu_vbus_is_high(void)
{
	//int vbus_st = 0;
	//vbus_st = i2c_smbus_read_byte_data(i2cClient_usbswitch_main, TSU_VBUS); 
	//printk("usbswitch vbus read value : 0x%x\n", vbus_st);
	//return (vbus_st&TSU_MASK_VBUS)?true:false;
	return (TSU_DEVICE_USB == tsu_read_device_type())?true:false;
}

static inline int tsu_read_adc_val(void)
{
	int adc_val = 0;
	adc_val = i2c_smbus_read_byte_data(i2cClient_usbswitch_main, TSU_ADC);	
	printk("usbswitch ADC read value : 0x%x\n", adc_val);

	return adc_val;
}

enum usbswitchstatus tsu_state_detect(void)
{	
	enum usbswitchstatus new_st = DEVICE_DISCONNECT;

	switch (tsu_read_adc_val())
	{
		case TSU_ADC_USBCHARGE_OR_DATA:
			//if(tsu_vbus_is_high()) //usb mode vbus must high
			if(TSU_DEVICE_USB == tsu_read_device_type())
			{
				new_st = USBCHARGE_OR_DATA;				
			}
			break;
		case TSU_ADC_USBHEADPHONE1:
		case TSU_ADC_USBHEADPHONE2:
			new_st = USBHEADPHONE_2LINE;
			break;
		case TSU_ADC_USBHEADPHONE3:
			new_st = USBHEADPHONE;
			break;
		case TSU_ADC_MHL_OR_CRADLE1:
		case TSU_ADC_MHL_OR_CRADLE2:
			//if(tsu_vbus_is_high()) //mhl mode vbus must high
			//{
				new_st = MHL_OR_CRADLE;
			//}
			break;
		default:
			new_st = DEVICE_DISCONNECT;
			break;
	}	
	return new_st;
}

bool tsu_int_detect(struct usbswitch_int_val* ptr)
{
	bool rtn = false;
	int int1 = 0;
	int int2 = 0;

	int1 = i2c_smbus_read_byte_data(i2cClient_usbswitch_main, TSU_CARKIT_INT1);//clear carkit interrupt 1 status
	int2 = i2c_smbus_read_byte_data(i2cClient_usbswitch_main, TSU_CARKIT_INT2);//clear carkit interrupt 2 status
	printk("usbswitch carkit INT register : 0x%x, 0x%x\n", int1, int2);
	
	ptr->int1 = i2c_smbus_read_byte_data(i2cClient_usbswitch_main, TSU_INT1);
	if(unlikely(ptr->int1 < 0)){
		ptr->int1 = 0;
	}
	
	ptr->int2 = i2c_smbus_read_byte_data(i2cClient_usbswitch_main, TSU_INT2);
	if(unlikely(ptr->int2 < 0)){
		ptr->int2 = 0;
	}

	if((ptr->int1&0xFF)||(ptr->int2&0xFF)){
		rtn = true;
	}
	printk("usbswitch INT register : 0x%x, 0x%x\n", ptr->int1, ptr->int2);
	return rtn; // true - vaild int occur; false - unvaild int occur
}

void tsu_enable_raw_mode(void)
{
	s32 control_reg;
	control_reg = i2c_smbus_read_byte_data(i2cClient_usbswitch_main, TSU_CONTROL); 
	control_reg=control_reg&TSU_MASK_ENABLERAW;
	i2c_smbus_write_byte_data(i2cClient_usbswitch_main, TSU_CONTROL, control_reg);
}

void tsu_disable_raw_mode(void)
{
	s32 control_reg = 0;
	control_reg = i2c_smbus_read_byte_data(i2cClient_usbswitch_main, TSU_CONTROL); 
	control_reg = control_reg|TSU_MASK_DISABLERAW;
	i2c_smbus_write_byte_data(i2cClient_usbswitch_main, TSU_CONTROL, control_reg);
}

bool tsu_keypress_valid(void)
{
	bool rtn = true;
	int adc_val = 0;
	adc_val = tsu_read_adc_val();
	if(unlikely(adc_val<0)){
		rtn = false;
	}
	if((adc_val&0xff)==0x1f){
		rtn = false;
	}
	return rtn;
}

void tsu_chip_init(void)
{
	//s32 ControlData = 0;
	struct usbswitch_int_val int_val;
	//ControlData = i2c_smbus_read_byte_data(i2cClient_usbswitch_main, TSU_CONTROL); 
	
	//ControlData=ControlData&TSU_MASK_MANUAL;      
	//ControlData=ControlData&TSU_MASK_INT;
	i2c_smbus_write_byte_data(i2cClient_usbswitch_main, TSU_CONTROL, TSU_CONTROL_INIT_VAL);
	
	i2c_smbus_write_byte_data(i2cClient_usbswitch_main, TSU_CARKIT_INT1_MASK, TSU_MASK_CARKIT_INT);//disable carkit interrupt 1
	i2c_smbus_write_byte_data(i2cClient_usbswitch_main, TSU_CARKIT_INT2_MASK, TSU_MASK_CARKIT_INT);//disable carkit interrupt 2
	i2c_smbus_read_byte_data(i2cClient_usbswitch_main, TSU_CARKIT_INT1);//clear carkit interrupt 1 status
	i2c_smbus_read_byte_data(i2cClient_usbswitch_main, TSU_CARKIT_INT2);//clear carkit interrupt 2 status
	
	tsu_int_detect(&int_val); //clear interrupt status
}

struct usbswitch_struct tsu6712_func_ptr = {
	.init_chip = tsu_chip_init,
	.state_detect = tsu_state_detect,
	.int_detect = tsu_int_detect,
	.config_default = tsu6712_config_default,
	.config_mhl_cradle = tsu6712_config_mhl_or_cradle,
	.config_headphone = tsu6712_config_headphone,
	.config_usb_charge = tsu6712_config_usb,
	.enable_raw = tsu_enable_raw_mode,
	.disable_raw = tsu_disable_raw_mode,
	.keypress_valid = tsu_keypress_valid,
	.vbus_status = tsu_vbus_is_high,
};

