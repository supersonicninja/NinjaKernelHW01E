/*
* USBSWITCH <driver>
*
* Copyright (C) 2011 Huawei Technology Inc. 
*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed .as is. WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
 * PURPOSE.  See the
 * GNU General Public License for more details.
*/
#ifndef __USBSWITCH__
#define __USBSWITCH__
#include <linux/input.h>

#define USBSWITCH_DEVICEID			0x1	//the address of device id register
#define IC_MODEL_TSU6712			0x62	//the device id of tsu6712
#define IC_MODEL_FSA9485			0x00	//the device id of fsa9485

#define INT2_MASK_ADCCHANGE     	0x4        // the flage of  ADC change
#define INT2_MASK_AVCABLE      		0x1        // the flage of a/v cable
#define INT1_MASK_ATTACH        		0x1        // the flage of accessory attach
#define INT1_MASK_DETACH        		0x2        // the flage  of accessory dettach


#define TABLA_JACK_MASK (SND_JACK_HEADSET | SND_JACK_OC_HPHL | SND_JACK_OC_HPHR| SND_JACK_HEADPHONE_MONO)

enum usbswitchstatus
{
	DEVICE_DISCONNECT,
	USBCHARGE_OR_DATA,               
	MHL_OR_CRADLE,
	USBHEADPHONE,
	USBHEADPHONE_2LINE,
};

struct usbswitch_int_val
{
	u32 int1;
	u32 int2;
};

struct usbswitch_raw_staus
{
	bool raw_enable;
	unsigned long  raw_enable_time;
};

struct usbswitch_gpio_src
{
	int sel1_gpio;
	int sel2_gpio;
	int id_gpio;
	int mic_gpio;
};

enum usbswitchbotton
{
	BOTTON_PRESS,               
	BOTTON_RELEASE
};

struct usbswitch_struct
{
	void (*init_chip)(void);
	enum usbswitchstatus (*state_detect)(void);
	bool (*int_detect)(struct usbswitch_int_val* ptr);
	bool (*config_default)(void);
	bool (*config_mhl_cradle)(void);
	bool (*config_headphone)(void);
	bool (*config_usb_charge)(void);
	void (*enable_raw)(void);
	void (*disable_raw)(void);
	bool (*keypress_valid)(void);
	bool (*vbus_status)(void);
};

extern void tabla_hs_button_report(int status, int mask);
extern void tabla_hs_jack_report(int status, int mask); 
#endif
