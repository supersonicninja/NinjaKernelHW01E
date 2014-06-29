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

#include "msm_sensor.h"
#include "msm.h"
#include "msm_ispif.h"
#include <hsad/config_interface.h>
#define SENSOR_NAME "ov5647_sunny"
#define PLATFORM_DRIVER_NAME "msm_camera_ov5647_sunny"
#define ov5647_sunny_obj ov5647_sunny_##obj
#define MSB                             1
#define LSB                             0

#define OV5647_OTP_FEATURE              1

#ifdef OV5647_OTP_FEATURE
struct otp_struct {
	int iProduct_Year;
	int iProduct_Month;
	int iProduct_Date;
	int iCamera_Id;
	int iSupplier_Version_Id;
	int iWB_RG_H;
	int iWB_RG_L;
	int iWB_BG_H;
	int iWB_BG_L;
	int iWB_GbGr_H;
	int iWB_GbGr_L;
	int iVCM_Start;
	int iVCM_End;
};
static int check_otp_group(struct msm_sensor_ctrl_t *s_ctrl);
static int read_otp(struct msm_sensor_ctrl_t *s_ctrl, int index, struct otp_struct * otp_ptr);
static int update_awb_gain(struct msm_sensor_ctrl_t *s_ctrl, int R_gain, int G_gain, int B_gain);
static int update_otp(struct msm_sensor_ctrl_t *s_ctrl);
#endif

DEFINE_MUTEX(ov5647_sunny_mut);
static struct msm_sensor_ctrl_t ov5647_sunny_s_ctrl;

static struct msm_camera_i2c_reg_conf ov5647_sunny_start_settings[] = {
	{0x4202, 0x00},
};

static struct msm_camera_i2c_reg_conf ov5647_sunny_stop_settings[] = {
	{0x4202, 0x0f},
};

static struct msm_camera_i2c_reg_conf ov5647_sunny_groupon_settings[] = {
    {0x0104, 0x01},
};

static struct msm_camera_i2c_reg_conf ov5647_sunny_groupoff_settings[] = {
    {0x0104, 0x00},
};

static struct msm_camera_i2c_reg_conf ov5647_sunny_prev_settings[] ={
	/*1296*972 Reference Setting 24M MCLK 2lane 280Mbps/lane 30fps
	for back to preview*/
	//{0x0100, 0x00},//software standby
	{0x3035, 0x21},//PLL   0x21 30fps   0x41 15fps
	{0x3036, 0x66},//PLL,  0x64
	{0x303c, 0x11},//PLL   
#if 0
	{0x3821,0x00},//ISP mirror on, Sensor mirror on
    {0x3820,0x47},
#else
	{0x3821, 0x07},// defualt
    {0x3820, 0x41},
#endif 
    {0x3612, 0x59}, 
    {0x3618, 0x00}, 
    {0x380c, 0x09}, 
    {0x380d, 0x70}, 
    {0x380e, 0x04}, 
    {0x380f, 0x66}, 
    {0x3814, 0x31}, 
    {0x3815, 0x31}, 
    {0x3708, 0x64}, 
    {0x3709, 0x52}, 
    {0x3808, 0x05}, //X OUTPUT SIZE = 1296
    {0x3809, 0x10}, 
    {0x380a, 0x03}, //Y OUTPUT SIZE = 972
    {0x380b, 0xcc}, 
    {0x3800, 0x00}, 
    {0x3801, 0x08}, 
    {0x3802, 0x00}, 
    {0x3803, 0x00}, 
    {0x3804, 0x0a}, 
    {0x3805, 0x37}, 
    {0x3806, 0x07}, 
    {0x3807, 0xa7}, 
	

    {0x4004, 0x02},//black line number

	{0x4005, 0x18},//add
    {0x4051, 0x8F},//add
    
    {0x4837, 0x19},     // 0x3035, 0x41    0x4837 37
    //{0x0100, 0x01}, 
};

static struct msm_camera_i2c_reg_conf ov5647_sunny_snap_settings[] = {
	//{0x0100, 0x00},//software standby
    {0x3035, 0x21},
    {0x3036, 0x66},
    {0x303c, 0x11},
#if 1
   	{0x3821, 0x00},//ISP mirror of, Sensor mirror of 0x00  
    {0x3820, 0x06},
#else
	{0x3821, 0x06},//ISP mirror on, Sensor mirror on  defualt
    {0x3820, 0x00},//ISP flip off, Sensor flip off
#endif	
    {0x3612, 0x5b},
    {0x3618, 0x04},
    {0x380c, 0x0a},
    {0x380d, 0xc0},
    {0x380e, 0x07},
    {0x380f, 0xb6},
    {0x3814, 0x11},
    {0x3815, 0x11},
    {0x3708, 0x64},
    {0x3709, 0x12},

    /*2608 * 1952 */
	{0x3808, 0x0a}, //X OUTPUT SIZE = 2608
    {0x3809, 0x30},
    {0x380a, 0x07}, //Y OUTPUT SIZE = 1952
    {0x380b, 0xa0},
    {0x3800, 0x00},
    {0x3801, 0x04},
    {0x3802, 0x00},
    {0x3803, 0x00},
    {0x3804, 0x0a},
    {0x3805, 0x3b},
    {0x3806, 0x07},
    {0x3807, 0xa3},

    {0x4004, 0x04},//black line number

	{0x4005, 0x1a},// add
	
    {0x4837, 0x19},//MIPI pclk period
   // {0x0100, 0x01},//wake up from software standby
};

static struct msm_camera_i2c_reg_conf ov5647_sunny_recommend_settings[] = {
//{0x0100,0x00},                 
//{0x0103,0x01},                 
//delay(5ms)                   
{0x3035,0x21},               
{0x303c,0x11},               
{0x370c,0x03},                 
{0x5000,0x06},                 
{0x5003,0x08},                 
{0x5a00,0x08},                 
{0x3000,0xff},                 
{0x3001,0xff},                 
{0x3002,0xff},                 
{0x301d,0xf0},                 
{0x3a18,0x00},                 
{0x3a19,0xf8},                 
{0x3c01,0x80},                 
{0x3b07,0x0c},                 
{0x3708,0x64},              
{0x3630,0x2e},                 
{0x3632,0xe2},                 
{0x3633,0x23},                 
{0x3634,0x44},                 
{0x3620,0x64},                 
{0x3621,0xe0},                 
{0x3600,0x37},                 
{0x3704,0xa0},                 
{0x3703,0x5a},                 
{0x3715,0x78},                 
{0x3717,0x01},                 
{0x3731,0x02},                 
{0x370b,0x60},                 
{0x3705,0x1a},                 
{0x3f05,0x02},                 
{0x3f06,0x10},                 
{0x3f01,0x0a},                 
{0x3a08,0x01},            
{0x3a0f,0x58},                 
{0x3a10,0x50},                 
{0x3a1b,0x58},                 
{0x3a1e,0x50},                 
{0x3a11,0x60},                 
{0x3a1f,0x28},                 
{0x4001,0x02},                 
{0x4000,0x09},                 
{0x3000,0x00},                 
{0x3001,0x00},                 
{0x3002,0x00},                 
{0x3017,0xe0},                 
{0x301c,0xfc},                 
{0x3636,0x06},                 
{0x3016,0x08},                 
{0x3827,0xec},                 
{0x3018,0x44},                 
{0x3035,0x21},                 
{0x3106,0xf5},                 
{0x3034,0x1a},                 
{0x301c,0xf8},                 
// manual AWB,manual AE,close L
{0x3503,0x03}, //manual AE     
{0x3501,0x10},                 
{0x3502,0x80},                 
{0x350a,0x00},                 
{0x350b,0x7f},                 
{0x5001,0x01}, //manual AWB    
{0x5180,0x08},                 
{0x5186,0x04},                 
{0x5187,0x00},                 
{0x5188,0x04},                 
{0x5189,0x00},                 
{0x518a,0x04},                 
{0x518b,0x00},                 
{0x5000,0x86}, // lenc on ,WBC on

{0x5800,0xc },
{0x5801,0x8 },
{0x5802,0x6 },
{0x5803,0x6 },
{0x5804,0x7 },
{0x5805,0xc },
{0x5806,0x5 },
{0x5807,0x4 },
{0x5808,0x2 },
{0x5809,0x2 },
{0x580a,0x4 },
{0x580b,0x6 },
{0x580c,0x4 },
{0x580d,0x1 },
{0x580e,0x0 },
{0x580f,0x0 },
{0x5810,0x1 },
{0x5811,0x4 },
{0x5812,0x4 },
{0x5813,0x1 },
{0x5814,0x0 },
{0x5815,0x0 },
{0x5816,0x1 },
{0x5817,0x4 },
{0x5818,0x5 },
{0x5819,0x4 },
{0x581a,0x2 },
{0x581b,0x2 },
{0x581c,0x3 },
{0x581d,0x6 },
{0x581e,0xb },
{0x581f,0x8 },
{0x5820,0x7 },
{0x5821,0x7 },
{0x5822,0x7 },
{0x5823,0xa },
{0x5824,0x26},
{0x5825,0x6 },
{0x5826,0x6 },
{0x5827,0x26},
{0x5828,0x44},
{0x5829,0x26},
{0x582a,0x42},
{0x582b,0x42},
{0x582c,0x44},
{0x582d,0x24},
{0x582e,0x6 },
{0x582f,0x62},
{0x5830,0x60},
{0x5831,0x62},
{0x5832,0x24},
{0x5833,0x26},
{0x5834,0x44},
{0x5835,0x42},
{0x5836,0x44},
{0x5837,0x24},
{0x5838,0x26},
{0x5839,0x26},
{0x583a,0x6 },
{0x583b,0x28},
{0x583c,0x26},
{0x583d,0xae},


{0x4800,0x24},   
{0x4837,0x0d},
{0x4819,0xff},
{0x481f,0x32}, //decrease mipi prepare timing
{0x0100,0x01}, 
};

static struct v4l2_subdev_info ov5647_sunny_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_conf_array ov5647_sunny_init_conf[] = {
	{&ov5647_sunny_recommend_settings[0],
	ARRAY_SIZE(ov5647_sunny_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array ov5647_sunny_confs[] = {
	{&ov5647_sunny_snap_settings[0],
	ARRAY_SIZE(ov5647_sunny_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov5647_sunny_prev_settings[0],
	ARRAY_SIZE(ov5647_sunny_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_sensor_output_info_t ov5647_sunny_dimensions[] = {
	/* snapshot */
    {
        .x_output = 0X0A30,
		.y_output = 0X07A0,
		.line_length_pclk = 0X0Ac0,  
		.frame_length_lines = 0X07B6,
		.vt_pixel_clk =82000000,
		.op_pixel_clk =192000000, 
		.binning_factor = 0,
    },
	/* preview */
    {
		.x_output = 0x0510, 
		.y_output = 0x03cc,
		.line_length_pclk = 0X0970, 
		.frame_length_lines = 0X0466,
		.vt_pixel_clk =82000000, 
		.op_pixel_clk =192000000, 
		.binning_factor = 1,
    },
}; 

static struct msm_camera_csid_vc_cfg ov5647_sunny_cid_cfg[] = {
	{0, CSI_RAW10, CSI_DECODE_10BIT},
    {1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params ov5647_sunny_csi_params = {
	.csid_params = {
		.lane_assign = 0xe4,
		.lane_cnt = 2,
		.lut_params = {
			.num_cid = ARRAY_SIZE(ov5647_sunny_cid_cfg),
			.vc_cfg = ov5647_sunny_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt = 2,
		.settle_cnt = 0x18, //0x1b,
	},
};

static struct msm_camera_csi2_params *ov5647_sunny_csi_params_array[] = {
	&ov5647_sunny_csi_params,
	&ov5647_sunny_csi_params,
};

static struct msm_sensor_output_reg_addr_t ov5647_sunny_reg_addr = {
	.x_output = 0x3808,
	.y_output = 0x380a,
	.line_length_pclk = 0x380c,
	.frame_length_lines = 0x380e,
};

static struct msm_sensor_id_info_t ov5647_sunny_id_info = {
	.sensor_id_reg_addr = 0x300A,
	.sensor_id = 0x5647,
};

static struct msm_sensor_exp_gain_info_t ov5647_sunny_exp_gain_info = {
	.coarse_int_time_addr = 0x3500,
	.global_gain_addr = 0x350a,
	.vert_offset = 4,
};

#ifdef OV5647_OTP_FEATURE

static int RG_Ratio_Typical = 0x28f; 
static int BG_Ratio_Typical = 0x2d1; 

static int check_otp_group(struct msm_sensor_ctrl_t *s_ctrl)
{
	uint16_t temp;
	int i;
	uint16_t address;
	uint16_t index;
    int rc;
	// check group 1 first
	index =  1;

	// read otp into buffer
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3d21, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
    msleep(3);
    
	// read R/G data from OTP to judge data empty or not
	address = 0x3d05 + index*13 + 5;
    rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, address, &temp, MSM_CAMERA_I2C_WORD_DATA);

	// disable otp read
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3d21, 0x00, MSM_CAMERA_I2C_BYTE_DATA);
    
	// clear otp buffer
	for (i = 0; i < 32; i++) {
        msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3d00 + i, 0x00, MSM_CAMERA_I2C_BYTE_DATA);
	}
	
	if(!temp == 0) {
			return 1;
	}

	// check group 2 data
	index =  0;

	// read otp into buffer
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3d21, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
    msleep(3);
    
    address = 0x3d05 + index*13 + 5;
    rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, address, &temp, MSM_CAMERA_I2C_WORD_DATA);
	// disable otp read
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3d21, 0x00, MSM_CAMERA_I2C_BYTE_DATA);
	// clear otp buffer
	for (i = 0; i < 32; i++) {
        msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3d00 + i, 0x00, MSM_CAMERA_I2C_BYTE_DATA);
	}
	
	if (!temp == 0) {
		return 0;
	} else {
		return 2;
	}
}

static int read_otp(struct msm_sensor_ctrl_t *s_ctrl, int index, struct otp_struct * otp_ptr)
{
	int i;
	uint16_t address;
    uint16_t temp;
    int rc;
    
	// read otp into buffer
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3d21, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
    msleep(3);
	address = 0x3d05 + index*13;
    rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, address, &temp, MSM_CAMERA_I2C_BYTE_DATA);
    (*otp_ptr).iProduct_Year = temp;

    rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, address+1, &temp, MSM_CAMERA_I2C_BYTE_DATA);
    (*otp_ptr).iProduct_Month = temp;

    rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, address+2, &temp, MSM_CAMERA_I2C_BYTE_DATA);
    (*otp_ptr).iProduct_Date = temp;

    rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, address+3, &temp, MSM_CAMERA_I2C_BYTE_DATA);
    (*otp_ptr).iCamera_Id = temp;

    rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, address+4, &temp, MSM_CAMERA_I2C_BYTE_DATA);
    (*otp_ptr).iSupplier_Version_Id = temp;

    rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, address+5, &temp, MSM_CAMERA_I2C_BYTE_DATA);
    (*otp_ptr).iWB_RG_H = temp;

    rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, address+6, &temp, MSM_CAMERA_I2C_BYTE_DATA);
    (*otp_ptr).iWB_RG_L = temp;

    rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, address+7, &temp, MSM_CAMERA_I2C_BYTE_DATA);
    (*otp_ptr).iWB_BG_H = temp;

    rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, address+8, &temp, MSM_CAMERA_I2C_BYTE_DATA);
    (*otp_ptr).iWB_BG_L = temp;

    rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, address+9, &temp, MSM_CAMERA_I2C_BYTE_DATA);
    (*otp_ptr).iWB_GbGr_H = temp;

    rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, address+10, &temp, MSM_CAMERA_I2C_BYTE_DATA);
    (*otp_ptr).iWB_GbGr_L = temp;

    rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, address+11, &temp, MSM_CAMERA_I2C_BYTE_DATA);
    (*otp_ptr).iVCM_Start = temp;

    rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, address+12, &temp, MSM_CAMERA_I2C_BYTE_DATA);
    (*otp_ptr).iVCM_End = temp;

	// disable otp read
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3d21, 0x00, MSM_CAMERA_I2C_BYTE_DATA);
	// clear otp buffer
	for (i = 0; i < 32; i++) {
        msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3d00 + i, 0x00, MSM_CAMERA_I2C_BYTE_DATA);        
	}

	return 0;	
}

/*
 * R_gain, sensor red gain of AWB, 0x400 =1
 * G_gain, sensor green gain of AWB, 0x400 =1
 * B_gain, sensor blue gain of AWB, 0x400 =1
 * return 0;
 */
static int update_awb_gain(struct msm_sensor_ctrl_t *s_ctrl, int R_gain, int G_gain, int B_gain)
{
	if (R_gain > 0x400) {
        msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x5186, R_gain, MSM_CAMERA_I2C_WORD_DATA);
	}

	if (G_gain > 0x400) {
        msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x5188, G_gain, MSM_CAMERA_I2C_WORD_DATA);
	}

	if (B_gain > 0x400) {
        msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x518a, B_gain, MSM_CAMERA_I2C_WORD_DATA);
	}
	return 0;
}

/*
 * call this function after OV5647 initialization
 * return value: 0, update success
 *		         1, no OTP
 */
static int update_otp(struct msm_sensor_ctrl_t *s_ctrl)
{
	struct otp_struct current_otp;
	int otp_index;
	int R_gain, G_gain, B_gain, G_gain_R, G_gain_B;
	int rg,bg;
	
	// Check OTP group
	otp_index = check_otp_group(s_ctrl);
	if(otp_index == 2) {
        printk("ov5647 sensor no data in OTP\n");
		// no data in OTP
		return 1;
	}

	// R/G and B/G of current camera module is read out from sensor OTP
	read_otp(s_ctrl, otp_index, &current_otp);

	rg = (current_otp.iWB_RG_H << 8) + current_otp.iWB_RG_L;
	bg = (current_otp.iWB_BG_H << 8) + current_otp.iWB_BG_L;

	//calculate G gain
	//0x400 = 1x gain
	if(bg < BG_Ratio_Typical) {
		if (rg< RG_Ratio_Typical) {
			// current_otp.bg_ratio < BG_Ratio_typical &&  
			// current_otp.rg_ratio < RG_Ratio_typical
   			G_gain = 0x400;
			B_gain = 0x400 * BG_Ratio_Typical / bg;
    		R_gain = 0x400 * RG_Ratio_Typical / rg; 
		} else {
			// current_otp.bg_ratio < BG_Ratio_typical &&  
			// current_otp.rg_ratio >= RG_Ratio_typical
    		R_gain = 0x400;
   	 		G_gain = 0x400 * rg / RG_Ratio_Typical;
    		B_gain = G_gain * BG_Ratio_Typical /bg;
		}
	} else {
		if (rg < RG_Ratio_Typical) {
			// current_otp.bg_ratio >= BG_Ratio_typical &&  
			// current_otp.rg_ratio < RG_Ratio_typical
    		B_gain = 0x400;
    		G_gain = 0x400 * bg / BG_Ratio_Typical;
    		R_gain = G_gain * RG_Ratio_Typical / rg;
		} else {
			// current_otp.bg_ratio >= BG_Ratio_typical &&  
			// current_otp.rg_ratio >= RG_Ratio_typical
    		G_gain_B = 0x400 * bg / BG_Ratio_Typical;
   	 		G_gain_R = 0x400 * rg / RG_Ratio_Typical;

    		if(G_gain_B > G_gain_R ) {
        		B_gain = 0x400;
        		G_gain = G_gain_B;
 	     		R_gain = G_gain * RG_Ratio_Typical /rg;
  			} else {
        		R_gain = 0x400;
       			G_gain = G_gain_R;
        		B_gain = G_gain * BG_Ratio_Typical / bg;
			}
    	}    
	}
//    printk("%s: R_gain=0x%x, G_gain=0x%x, B_gain=0x%x\n",__func__, R_gain, G_gain, B_gain);
	update_awb_gain(s_ctrl, R_gain, G_gain, B_gain);

	return 0;
}
#endif

static int32_t ov5647_sunny_write_prev_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
						uint16_t gain, uint32_t line)
{
	static uint16_t max_line = 984;
	u8 intg_time_hsb, intg_time_msb, intg_time_lsb;
	uint8_t gain_lsb, gain_hsb;

	CDBG(KERN_ERR "preview exposure setting 0x%x, 0x%x, %d",
		 gain, line, line);

	gain_lsb = (uint8_t) (gain);
	gain_hsb = (uint8_t)((gain & 0x300)>>8);

	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);

	/* adjust frame rate */
	if (line > 980 && line <= 984) {
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->frame_length_lines,
		(uint8_t)((line+4) >> 8),
		MSM_CAMERA_I2C_BYTE_DATA);

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->frame_length_lines + 1,
		(uint8_t)((line+4) & 0x00FF),
		MSM_CAMERA_I2C_BYTE_DATA);
		max_line = line + 4;
	} else if (max_line > 984) {

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->frame_length_lines,
		(uint8_t)(984 >> 8),
		MSM_CAMERA_I2C_BYTE_DATA);

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->frame_length_lines + 1 ,
		(uint8_t)(984 & 0x00FF),
		MSM_CAMERA_I2C_BYTE_DATA);
		max_line = 984;
	}

	line = line<<4;
	/* ov5647 need this operation */
	intg_time_hsb = (u8)(line>>16);
	intg_time_msb = (u8) ((line & 0xFF00) >> 8);
	intg_time_lsb = (u8) (line & 0x00FF);


	/* Coarse Integration Time */
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr,
		intg_time_hsb,
		MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 1,
		intg_time_msb,
		MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 2,
		intg_time_lsb,
		MSM_CAMERA_I2C_BYTE_DATA);

	/* gain */

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr,
		gain_hsb,
		MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr + 1,
		gain_lsb,
		MSM_CAMERA_I2C_BYTE_DATA);

	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);

	return 0;
}

static int32_t ov5647_sunny_write_pict_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t gain, uint32_t line)
{
	static uint16_t max_line = 1964;
	uint8_t gain_lsb, gain_hsb;
	u8 intg_time_hsb, intg_time_msb, intg_time_lsb;

	gain_lsb = (uint8_t) (gain);
	gain_hsb = (uint8_t)((gain & 0x300)>>8);

	CDBG(KERN_ERR "snapshot exposure seting 0x%x, 0x%x, %d"
		, gain, line, line);

	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
	if (line > 1964) {
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_output_reg_addr->frame_length_lines,
			(uint8_t)((line+4) >> 8),
			MSM_CAMERA_I2C_BYTE_DATA);

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_output_reg_addr->frame_length_lines + 1,
			(uint8_t)((line+4) & 0x00FF),
			MSM_CAMERA_I2C_BYTE_DATA);
		max_line = line + 4;
	} else if (max_line > 1968) {
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_output_reg_addr->frame_length_lines,
			(uint8_t)(1968 >> 8),
			MSM_CAMERA_I2C_BYTE_DATA);

		 msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_output_reg_addr->frame_length_lines + 1,
			(uint8_t)(1968 & 0x00FF),
			MSM_CAMERA_I2C_BYTE_DATA);
			max_line = 1968;
	}


	line = line<<4;
	/* ov5647 need this operation */
	intg_time_hsb = (u8)(line>>16);
	intg_time_msb = (u8) ((line & 0xFF00) >> 8);
	intg_time_lsb = (u8) (line & 0x00FF);

	/* FIXME for BLC trigger */
	/* Coarse Integration Time */
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr,
		intg_time_hsb,
		MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 1,
		intg_time_msb,
		MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 2,
		intg_time_lsb,
		MSM_CAMERA_I2C_BYTE_DATA);

	/* gain */

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr,
		gain_hsb,
		MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr + 1,
		gain_lsb^0x1,
		MSM_CAMERA_I2C_BYTE_DATA);

	/* Coarse Integration Time */
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr,
		intg_time_hsb,
		MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 1,
		intg_time_msb,
		MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 2,
		intg_time_lsb,
		MSM_CAMERA_I2C_BYTE_DATA);

	/* gain */

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr,
		gain_hsb,
		MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr + 1,
		gain_lsb,
		MSM_CAMERA_I2C_BYTE_DATA);


	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	return 0;
}

int32_t ov5647_sunny_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
    int rc=0;
	int reset_gpio;
	int vcm_pwd_gpio;
	int shdn_gpio;
    reset_gpio = get_gpio_num_by_name("CAM_RST");
    if(reset_gpio < 0) {
		printk(KERN_ERR"%s get_gpio_num_by_name fail\n",__func__);
		return reset_gpio;
	}
    rc = gpio_request(reset_gpio,"ov5647_sunny");
	if (rc) {
		gpio_free(reset_gpio);
		rc = gpio_request(reset_gpio,"ov5647_sunny");
		if(rc) {
		    printk("%s gpio_request(%d) again fail \n",__func__,reset_gpio);
			return rc;
		}
		printk("%s gpio_request(%d) again success\n",__func__,reset_gpio);
	}
   
    rc=gpio_direction_output(reset_gpio, 0);
    if (rc != 0) {
        printk("%s: gpio gpio_direction_output fail", __func__);
    }
   
    shdn_gpio = get_gpio_num_by_name("CAM_SHDN");
    if(shdn_gpio < 0) {
		printk(KERN_ERR"%s get_gpio_num_by_name fail\n",__func__);
		return shdn_gpio;
	}
    rc = gpio_request(shdn_gpio,"ov5647_sunny");
    if (rc) {
	    gpio_free(shdn_gpio);
	    rc = gpio_request(shdn_gpio,"ov5647_sunny");
	    if(rc) {
	        printk("%s gpio_request(%d) again fail \n",__func__,shdn_gpio);
	        return rc;
	    }
	    printk("%s gpio_request(%d) again success\n",__func__,shdn_gpio);
    }
    gpio_direction_output(shdn_gpio, 0);
    
	msm_sensor_expand_power_up(s_ctrl);
	mdelay(5);    
	
	vcm_pwd_gpio = get_gpio_num_by_name("CAM_AF_SHDN");    
	
	if(vcm_pwd_gpio < 0) {
		printk(KERN_ERR"%s get_gpio_num_by_name fail\n",__func__);
		return vcm_pwd_gpio;
	}

    rc = gpio_request(vcm_pwd_gpio,"ov5647_sunny");
    if (rc) {
	    gpio_free(vcm_pwd_gpio);
	    rc = gpio_request(vcm_pwd_gpio,"ov5647_sunny");
	    if(rc) {
	        printk("%s gpio_request(%d) again fail \n",__func__,vcm_pwd_gpio);
	        return rc;
	    }
	    printk("%s gpio_request(%d) again success\n",__func__,vcm_pwd_gpio);
    }
    gpio_direction_output(vcm_pwd_gpio, 1);
    mdelay(2);        
    gpio_direction_output(vcm_pwd_gpio, 0);
    gpio_direction_output(shdn_gpio, 1); 
    mdelay(2);
    gpio_direction_output(vcm_pwd_gpio, 1);
    gpio_direction_output(reset_gpio, 1);

    mdelay(20);
    return rc;
}

int32_t ov5647_sunny_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	int reset_gpio;
	int vcm_pwd_gpio;
	int shdn_gpio;
    
	reset_gpio = get_gpio_num_by_name("CAM_RST");
	vcm_pwd_gpio = get_gpio_num_by_name("CAM_AF_SHDN");
    shdn_gpio = get_gpio_num_by_name("CAM_SHDN");
	if(reset_gpio < 0) {
		printk(KERN_ERR"%s get_gpio_num_by_name fail\n",__func__);
		return reset_gpio;
	}
	if(vcm_pwd_gpio < 0) {
		printk(KERN_ERR"%s get_gpio_num_by_name fail\n",__func__);
		return vcm_pwd_gpio;
	}
    if(shdn_gpio < 0) {
		printk(KERN_ERR"%s get_gpio_num_by_name fail\n",__func__);
		return shdn_gpio;
	}	
    gpio_direction_output(reset_gpio, 0);
    gpio_direction_output(shdn_gpio, 0);
	gpio_free(vcm_pwd_gpio);
    gpio_free(reset_gpio);
    gpio_free(shdn_gpio);
    mdelay(3);
    
    msm_sensor_expand_power_down(s_ctrl);    
    return 0;
}

#if 0 /* not use temply */
static struct sensor_calib_data ov5647_sunny_calib_data;
#endif

static const struct i2c_device_id ov5647_sunny_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&ov5647_sunny_s_ctrl},
	{ }
};

static struct i2c_driver ov5647_sunny_i2c_driver = {
	.id_table = ov5647_sunny_i2c_id,
	.probe  = msm_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};
static struct msm_camera_i2c_client ov5647_sunny_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

#if 0 /* not use temply */
static struct msm_camera_i2c_client ov5647_sunny_eeprom_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
};

static struct msm_camera_eeprom_read_t ov5647_sunny_eeprom_read_tbl[] = {
	{0x10, &ov5647_sunny_calib_data.r_over_g, 2, 1},
	{0x12, &ov5647_sunny_calib_data.b_over_g, 2, 1},
	{0x14, &ov5647_sunny_calib_data.gr_over_gb, 2, 1},
};

static struct msm_camera_eeprom_data_t ov5647_sunny_eeprom_data_tbl[] = {
	{&ov5647_sunny_calib_data, sizeof(struct sensor_calib_data)},
};

static struct msm_camera_eeprom_client ov5647_sunny_eeprom_client = {
	.i2c_client = &ov5647_sunny_eeprom_i2c_client,
	.i2c_addr = 0xA4,

	.func_tbl = {
		.eeprom_set_dev_addr = NULL,
		.eeprom_init = msm_camera_eeprom_init,
		.eeprom_release = msm_camera_eeprom_release,
		.eeprom_get_data = msm_camera_eeprom_get_data,
	},

	.read_tbl = ov5647_sunny_eeprom_read_tbl,
	.read_tbl_size = ARRAY_SIZE(ov5647_sunny_eeprom_read_tbl),
	.data_tbl = ov5647_sunny_eeprom_data_tbl,
	.data_tbl_size = ARRAY_SIZE(ov5647_sunny_eeprom_data_tbl),
};
#endif

static int __init ov5647_sunny_init_module(void)
{
	return i2c_add_driver(&ov5647_sunny_i2c_driver);
}

static struct v4l2_subdev_core_ops ov5647_sunny_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops ov5647_sunny_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops ov5647_sunny_subdev_ops = {
	.core = &ov5647_sunny_subdev_core_ops,
	.video  = &ov5647_sunny_subdev_video_ops,
};

int32_t ov5647_sunny_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
			                        int update_type, int res)
{
	int32_t rc = 0;
    static int csi_config;
	v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
		NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(PIX_0, ISPIF_OFF_IMMEDIATELY));
	s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);

	if (csi_config == 0 || res == 0) {
        mdelay(66); 
    } else {
        mdelay(266);
    }    
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x4800, 0x25,MSM_CAMERA_I2C_BYTE_DATA);

	if (update_type == MSM_SENSOR_REG_INIT) {
		s_ctrl->curr_csi_params = NULL;
        msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0100, 0x00,MSM_CAMERA_I2C_BYTE_DATA);
        msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0103, 0x01,MSM_CAMERA_I2C_BYTE_DATA);
        mdelay(5);
        msm_sensor_enable_debugfs(s_ctrl);
		msm_sensor_write_init_settings(s_ctrl);
#ifdef OV5647_OTP_FEATURE  
        update_otp(s_ctrl);
#endif 
        csi_config = 0;
	} else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
			msm_sensor_write_res_settings(s_ctrl, res);   
              msleep(20);
        if (!csi_config) {
		    v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			    NOTIFY_PCLK_CHANGE, &s_ctrl->msm_sensor_reg->
			    output_settings[res].op_pixel_clk);
        
            s_ctrl->curr_csi_params = s_ctrl->csi_params[res];          
            CDBG("CSI config in progress\n");          
            v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_CSID_CFG,
				&s_ctrl->curr_csi_params->csid_params);
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
						NOTIFY_CID_CHANGE, NULL);
			mb();
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_CSIPHY_CFG,
				&s_ctrl->curr_csi_params->csiphy_params); 
            CDBG("CSI config is done\n");          
            mb();          
            mdelay(20);        
            csi_config = 1;   
            msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0100, 0x01,MSM_CAMERA_I2C_BYTE_DATA);
        }   
          v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
			PIX_0, ISPIF_ON_FRAME_BOUNDARY));        
          msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x4800, 0x04,MSM_CAMERA_I2C_BYTE_DATA);
          mdelay(266);   

		s_ctrl->func_tbl->sensor_start_stream(s_ctrl);

		mdelay(30);        
	}
	return rc;
}

static struct msm_sensor_fn_t ov5647_sunny_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain = ov5647_sunny_write_prev_exp_gain,
	.sensor_write_snapshot_exp_gain = ov5647_sunny_write_pict_exp_gain,
	.sensor_setting = ov5647_sunny_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = ov5647_sunny_sensor_power_up,
	.sensor_power_down = ov5647_sunny_sensor_power_down,	
};

static struct msm_sensor_reg_t ov5647_sunny_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = ov5647_sunny_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(ov5647_sunny_start_settings),
	.stop_stream_conf = ov5647_sunny_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(ov5647_sunny_stop_settings),
	.group_hold_on_conf = ov5647_sunny_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(ov5647_sunny_groupon_settings),
	.group_hold_off_conf = ov5647_sunny_groupoff_settings,
	.group_hold_off_conf_size = ARRAY_SIZE(ov5647_sunny_groupoff_settings),
	.init_settings = &ov5647_sunny_init_conf[0],
	.init_size = ARRAY_SIZE(ov5647_sunny_init_conf),
	.mode_settings = &ov5647_sunny_confs[0],
	.output_settings = &ov5647_sunny_dimensions[0],
	.num_conf = ARRAY_SIZE(ov5647_sunny_confs),
};

static struct msm_sensor_ctrl_t ov5647_sunny_s_ctrl = {
	.msm_sensor_reg = &ov5647_sunny_regs,
	.sensor_i2c_client = &ov5647_sunny_sensor_i2c_client,
	.sensor_i2c_addr = 0x6c,
	#if 0 /* not use temply */
	.sensor_eeprom_client = &ov5647_sunny_eeprom_client,
	#endif
	.sensor_output_reg_addr = &ov5647_sunny_reg_addr,
	.sensor_id_info = &ov5647_sunny_id_info,
	.sensor_exp_gain_info = &ov5647_sunny_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csi_params = &ov5647_sunny_csi_params_array[0],
	.msm_sensor_mutex = &ov5647_sunny_mut,
	.sensor_i2c_driver = &ov5647_sunny_i2c_driver,
	.sensor_v4l2_subdev_info = ov5647_sunny_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(ov5647_sunny_subdev_info),
	.sensor_v4l2_subdev_ops = &ov5647_sunny_subdev_ops,
	.func_tbl = &ov5647_sunny_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
};

module_init(ov5647_sunny_init_module);
MODULE_DESCRIPTION("ov5647_sunny 5m Bayer sensor driver");
MODULE_LICENSE("GPL v2");
