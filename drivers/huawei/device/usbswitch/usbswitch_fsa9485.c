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
#include "usbswitch_fsa9485.h"


extern struct usbswitch_gpio_src usbswitch_gpio;	
extern struct i2c_client *i2cClient_usbswitch_main;


bool fsa9485_config_mhl_or_cradle(void)
{
	bool rtn = true;
	
	if(i2c_smbus_write_byte_data(i2cClient_usbswitch_main, FSA_MSW1, FSA_MSW1_USB)){
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

bool fsa9485_config_usb(void)
{
	bool rtn = true;
	
	if(i2c_smbus_write_byte_data(i2cClient_usbswitch_main, FSA_MSW1, FSA_MSW1_USB)){
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

bool fsa9485_config_headphone(void)
{
	bool rtn = true;

	if(i2c_smbus_write_byte_data(i2cClient_usbswitch_main, FSA_MSW1, FSA_MSW1_HEADPHONE)){
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

bool fsa9485_config_default(void)
{
	bool rtn = true;
	
	if(i2c_smbus_write_byte_data(i2cClient_usbswitch_main, FSA_MSW1, FSA_MSW1_USB)){
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

bool fsa_vbus_is_high(void)
{
	int vbus_st = 0;
	vbus_st = i2c_smbus_read_byte_data(i2cClient_usbswitch_main, FSA_VBUS); 
	if(unlikely(vbus_st<0))
	{
		vbus_st = 0;
	}
	return (vbus_st&FSA_MASK_VBUS)?true:false;
}

static inline int fsa_read_adc_val(void)
{
	int adc_val = 0;
	adc_val = i2c_smbus_read_byte_data(i2cClient_usbswitch_main, FSA_ADC);	
	return adc_val;
}

enum usbswitchstatus fsa_state_detect(void)
{	
	enum usbswitchstatus new_st = DEVICE_DISCONNECT;

	switch (fsa_read_adc_val())
	{
		case FSA_ADC_USBCHARGE_OR_DATA:
			if(fsa_vbus_is_high()) //usb mode vbus must high
			{
				new_st = USBCHARGE_OR_DATA;
			}
			break;
		case FSA_ADC_USBHEADPHONE1:
		case FSA_ADC_USBHEADPHONE2:
			new_st = USBHEADPHONE_2LINE;
			break;
		case FSA_ADC_USBHEADPHONE3:
			new_st = USBHEADPHONE;
			break;
		case FSA_ADC_MHL_OR_CRADLE1:
		case FSA_ADC_MHL_OR_CRADLE2:
			if(fsa_vbus_is_high()) //mhl mode vbus must high
			{
				new_st = MHL_OR_CRADLE;
			}
			break;
		default:
			new_st = DEVICE_DISCONNECT;
			break;
	}	
	return new_st;
}

bool fsa_int_detect(struct usbswitch_int_val* ptr)
{
	bool rtn = false;
	
	ptr->int1 = i2c_smbus_read_byte_data(i2cClient_usbswitch_main, FSA_INT1);
	if(unlikely(ptr->int1 < 0)){
		ptr->int1 = 0;
	}
	
	ptr->int2 = i2c_smbus_read_byte_data(i2cClient_usbswitch_main, FSA_INT2);
	if(unlikely(ptr->int2 < 0)){
		ptr->int2 = 0;
	}

	if((ptr->int1&FSA_CLR_ALL_BITS_MASK)||(ptr->int2&FSA_CLR_ALL_BITS_MASK)){
		rtn = true;
	}
	printk("usbswitch INT register : 0x%x, 0x%x\n", ptr->int1, ptr->int2);
	
	return rtn; // true - vaild int occur; false - unvaild int occur
}

void fsa_enable_raw_mode(void)
{
	s32 control_reg;
	control_reg = i2c_smbus_read_byte_data(i2cClient_usbswitch_main, FSA_CONTROL); 
	control_reg=control_reg&FSA_MASK_ENABLERAW;
	i2c_smbus_write_byte_data(i2cClient_usbswitch_main, FSA_CONTROL, control_reg);
}

void fsa_disable_raw_mode(void)
{
	s32 control_reg = 0;
	control_reg = i2c_smbus_read_byte_data(i2cClient_usbswitch_main, FSA_CONTROL); 
	control_reg = control_reg|FSA_MASK_DISABLERAW;
	i2c_smbus_write_byte_data(i2cClient_usbswitch_main, FSA_CONTROL, control_reg);
}

bool fsa_keypress_valid(void)
{
	bool rtn = true;
	int adc_val = 0;
	adc_val = fsa_read_adc_val();
	if(unlikely(adc_val<0)){
		rtn = false;
	}
	if((adc_val&0xff)==0x1f){
		rtn = false;
	}
	return rtn;
}

void fsa_chip_init(void)
{
	struct usbswitch_int_val int_val;

       i2c_smbus_write_byte_data(i2cClient_usbswitch_main, FSA_RESET, FSA_RESET_DATA);
      
	i2c_smbus_write_byte_data(i2cClient_usbswitch_main, FSA_CONTROL, FSA_CONTROL_INIT_VAL);

	i2c_smbus_write_byte_data(i2cClient_usbswitch_main, FSA_OVP_FUSE, FSA_OVP_7_2);//config OVP to 7.2V
	
	fsa_int_detect(&int_val); //clear interrupt status
}

struct usbswitch_struct fsa9485_func_ptr = {
	.init_chip = fsa_chip_init,
	.state_detect = fsa_state_detect,
	.int_detect = fsa_int_detect,
	.config_default = fsa9485_config_default,
	.config_mhl_cradle = fsa9485_config_mhl_or_cradle,
	.config_headphone = fsa9485_config_headphone,
	.config_usb_charge = fsa9485_config_usb,
	.enable_raw = fsa_enable_raw_mode,
	.disable_raw = fsa_disable_raw_mode,
	.keypress_valid = fsa_keypress_valid,
	.vbus_status = fsa_vbus_is_high,
};
