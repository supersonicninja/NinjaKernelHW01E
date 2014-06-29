/* drivers/input/misc/tmd2771.h
 *
 * Copyright (C) 2011 HUAWEI, Inc.
 * Author: chenlian <chenlian@huawei.com>
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

#ifndef _LINUX_TMD2771_H
#define _LINUX_TMD2771_H

#define	PL_POWER_NAME	"PROXIMITY_LIGHT_VDD_SENSOR"
#define TAOS_DEVICE_NAME		"taos"
#define TAOS_DEVICE_ID			"tritonFN"
#define TAOS_ID_NAME_SIZE		10
#define TAOS_TRITON_CHIPIDVAL   	0x00
#define TAOS_TRITON_MAXREGS     	32
#define TAOS_DEVICE_ADDR1		0x29
#define TAOS_DEVICE_ADDR2       	0x39
#define TAOS_DEVICE_ADDR3       	0x49
#define TAOS_MAX_NUM_DEVICES		3
#define TAOS_MAX_DEVICE_REGS		32
#define I2C_MAX_ADAPTERS		8

// TRITON register offsets
#define TAOS_TRITON_CNTRL 		0x00
#define TAOS_TRITON_ALS_TIME 		0X01
#define TAOS_TRITON_PRX_TIME		0x02
#define TAOS_TRITON_WAIT_TIME		0x03
#define TAOS_TRITON_ALS_MINTHRESHLO	0X04
#define TAOS_TRITON_ALS_MINTHRESHHI 	0X05
#define TAOS_TRITON_ALS_MAXTHRESHLO	0X06
#define TAOS_TRITON_ALS_MAXTHRESHHI	0X07
#define TAOS_TRITON_PRX_MINTHRESHLO 	0X08
#define TAOS_TRITON_PRX_MINTHRESHHI 	0X09
#define TAOS_TRITON_PRX_MAXTHRESHLO 	0X0A
#define TAOS_TRITON_PRX_MAXTHRESHHI 	0X0B
#define TAOS_TRITON_INTERRUPT		0x0C
#define TAOS_TRITON_PRX_CFG		0x0D
#define TAOS_TRITON_PRX_COUNT		0x0E
#define TAOS_TRITON_GAIN		0x0F
#define TAOS_TRITON_REVID		0x11
#define TAOS_TRITON_CHIPID      	0x12
#define TAOS_TRITON_STATUS		0x13
#define TAOS_TRITON_ALS_CHAN0LO		0x14
#define TAOS_TRITON_ALS_CHAN0HI		0x15
#define TAOS_TRITON_ALS_CHAN1LO		0x16
#define TAOS_TRITON_ALS_CHAN1HI		0x17
#define TAOS_TRITON_PRX_LO		0x18
#define TAOS_TRITON_PRX_HI		0x19
#define TAOS_TRITON_TEST_STATUS		0x1F

// Triton cmd reg masks
#define TAOS_TRITON_CMD_REG		0X80
#define TAOS_TRITON_CMD_BYTE_RW		0x00
#define TAOS_TRITON_CMD_WORD_BLK_RW	0x20
#define TAOS_TRITON_CMD_SPL_FN		0x60
#define TAOS_TRITON_CMD_PROX_INTCLR	0X05
#define TAOS_TRITON_CMD_ALS_INTCLR	0X06
#define TAOS_TRITON_CMD_PROXALS_INTCLR 	0X07
#define TAOS_TRITON_CMD_TST_REG		0X08
#define TAOS_TRITON_CMD_USER_REG	0X09

// Triton cntrl reg masks
#define TAOS_TRITON_CNTL_PROX_INT_ENBL	0X20
#define TAOS_TRITON_CNTL_ALS_INT_ENBL	0X10
#define TAOS_TRITON_CNTL_WAIT_TMR_ENBL	0X08
#define TAOS_TRITON_CNTL_PROX_DET_ENBL	0X04
#define TAOS_TRITON_CNTL_ADC_ENBL	0x02
#define TAOS_TRITON_CNTL_PWRON		0x01

// Triton status reg masks
#define TAOS_TRITON_STATUS_ADCVALID	0x01
#define TAOS_TRITON_STATUS_PRXVALID	0x02
#define TAOS_TRITON_STATUS_ADCINTR	0x10
#define TAOS_TRITON_STATUS_PRXINTR	0x20

// lux constants
#define	TAOS_MAX_LUX			65535000
#define TAOS_SCALE_MILLILUX		3
#define TAOS_FILTER_DEPTH		3

#define TIMER_DEFAULT_DELAY 1000 // 1S
#define DISTENCE_CLOSE  0
#define DISTENCE_FAR    1
#define TAOS_ECS_IOCTL_APP_SET_DELAY	    _IOW(0xA1, 0x18, short)
#define TAOS_ECS_IOCTL_APP_GET_DELAY       _IOR(0xA1, 0x30, short)
#define TAOS_ECS_IOCTL_APP_SET_LFLAG		_IOW(0xA1, 0x1C, short)
#define TAOS_ECS_IOCTL_APP_GET_LFLAG		_IOR(0xA1, 0x1B, short)
#define TAOS_ECS_IOCTL_APP_SET_PFLAG		_IOW(0xA1, 0x1E, short)
#define TAOS_ECS_IOCTL_APP_GET_PFLAG		_IOR(0xA1, 0x1D, short)

// device configuration
struct taos_cfg
{
    u32     calibrate_target;
    u16     als_time;
    u16     scale_factor;
    u16     gain_trim;
    u8      filter_history;
    u8      filter_count;
    u8      gain;
    u16     prox_threshold_hi;
    u16     prox_threshold_lo;
//    u8	    prox_int_time;
    u8	    prox_adc_time;
    u8	    prox_wait_time;
    u8	    prox_intr_filter;
    u8	    prox_config;
    u8	    prox_pulse_cnt;
    u8	    prox_gain;
};

// proximity data
struct taos_prox_info 
{
    u16     prox_clear;
    u16     prox_data;
    s32     prox_event;
};

// lux time scale
struct time_scale_factor  {
	u16     numerator;
	u16	    denominator;
	u16	    saturation;
};

// lux data
struct lux_data {
	u16	    ratio;
	u16	    clear;
	u16	    ir;
};
struct tmd2771_platform_data {
	int (*power_on)(struct device*);
	int (*power_off)(void);
};

#endif /* _LINUX_TMD2771_H */

