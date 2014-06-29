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
#include <hsad/config_interface.h>
#define SENSOR_NAME "s5k4e1gx_semco"
#define PLATFORM_DRIVER_NAME "msm_camera_s5k4e1gx_semco"
#define s5k4e1gx_semco_obj s5k4e1gx_semco_##obj
#define MSB                             1
#define LSB                             0

DEFINE_MUTEX(s5k4e1gx_semco_mut);
static struct msm_sensor_ctrl_t s5k4e1gx_semco_s_ctrl;

static struct msm_camera_i2c_reg_conf s5k4e1gx_semco_start_settings[] = {
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf s5k4e1gx_semco_stop_settings[] = {
	{0x0100, 0x00},
};

static struct msm_camera_i2c_reg_conf s5k4e1gx_semco_groupon_settings[] = {
	{0x0104, 0x01},
};

static struct msm_camera_i2c_reg_conf s5k4e1gx_semco_groupoff_settings[] = {
	{0x0104, 0x00},
};

static struct msm_camera_i2c_reg_conf s5k4e1gx_semco_prev_settings[] ={
  
   	{0x0202, 0x03},
	{0x0203, 0xD7},
	{0x0204, 0x00},  
	{0x0205, 0x80},

	{0x0340, 0x03},
	{0x0341, 0xE0},
	{0x0342, 0x0a},
	{0x0343, 0xb2},

	{0x0305, 0x06},
	{0x0306, 0x00},
	{0x0307, 0x65},
	{0x30b5, 0x01},
	{0x30e2, 0x02},
	{0x30f1, 0xa0},

	{0x30a9, 0x02},
	{0x300e, 0xEB},
	{0x0380, 0x00},
	{0x0381, 0x01},
	{0x0382, 0x00},
	{0x0383, 0x01},
	{0x0384, 0x00},
	{0x0385, 0x01},
	{0x0386, 0x00},
	{0x0387, 0x03},

	{0x0344, 0x00},
	{0x0345, 0x00},
	{0x0346, 0x00},
	{0x0347, 0x00},
	{0x0348, 0x0a},
	{0x0349, 0x2f},
	{0x034a, 0x07},
	{0x034b, 0xa7},


	{0x034c, 0x05},
	{0x034d, 0x18},
	{0x034e, 0x03},
	{0x034f, 0xD4},
	{0x30bf, 0xab},
	{0x30c0, 0xc0},
	{0x30c8, 0x06},
	{0x30c9, 0x5E},

};

static struct msm_camera_i2c_reg_conf s5k4e1gx_semco_snap_settings[] = {
// Integration setting ... 
	{0x0202, 0x07},
	{0x0203, 0xaf},
	{0x0204, 0x00},
	{0x0205, 0x80},
// Frame Length
	{0x0340, 0x07},
	{0x0341, 0xB7},
// Line Length
	{0x0342, 0x0A},//2738
	{0x0343, 0xB2},

// PLL setting ...
//// input clock 24MHz
	{0x0305, 0x06},
	{0x0306, 0x00},
	{0x0307, 0x65},
	{0x30B5, 0x01},
	{0x30E2, 0x02},//num lanes[1:0] = 2
	{0x30F1, 0xA0},
// MIPI Size Setting
	{0x30A9, 0x03},//Horizontal Binning Off
	{0x300E, 0xE8},//Vertical Binning Off
	{0x0380, 0x00},//x_even_inc1
	{0x0381, 0x01},
	{0x0382, 0x00},//x_odd_inc1
	{0x0383, 0x01},
	{0x0384, 0x00},//y_even_inc1 
	{0x0385, 0x01},
	{0x0386, 0x00},//y_odd_inc3
	{0x0387, 0x01},

	{0x0344, 0x00},//x_odd_start
	{0x0345, 0x00},
	{0x0346, 0x00},//y_odd_start
	{0x0347, 0x00},
	{0x0348, 0x0a},//x_odd_end
	{0x0349, 0x1f},
	{0x034a, 0x07},//y_odd_end
	{0x034b, 0xa7},

	{0x034C, 0x0A},//x_output size
	{0x034D, 0x20},
	{0x034E, 0x07},//y_output size
	{0x034F, 0xA8},

	{0x30BF, 0xAB},//outif_enable[7], data_type[5:0](2Bh = bayer 10bit)
	{0x30C0, 0xc0},//video_offset[7:4] 3260%12
	{0x30C8, 0x0C},//video_data_length 3260 = 2608 * 1.25
	{0x30C9, 0xa8},
};

static struct msm_camera_i2c_reg_conf s5k4e1gx_semco_recommend_settings[] = {
 // Analog Setting
	{0x3000, 0x05},	
	{0x3001, 0x03},	
	{0x3002, 0x08},	
	{0x3003, 0x09},	
	{0x3004, 0x2E},	
	{0x3005, 0x06},	
	{0x3006, 0x34},	
	{0x3007, 0x00},	
	{0x3008, 0x3C},	
	{0x3009, 0x3C},	
	{0x300A, 0x28},	
	{0x300B, 0x04},	
	{0x300C, 0x0A},	
	{0x300D, 0x02},	
	{0x300E, 0xE8},	
	{0x300F, 0x82},	
	{0x3010, 0x00},	
	{0x3011, 0x4C},	
	{0x3012, 0x30},	
	{0x3013, 0xC0},	
	{0x3014, 0x00},	
	{0x3015, 0x00},	
	{0x3016, 0x2C},	
	{0x3017, 0x94},		
	{0x3018, 0x78},
	{0x301B, 0x83},
	{0x301C, 0x04},
	{0x301D, 0xD4},
	{0x3021, 0x02},
	{0x3022, 0x24},
	{0x3024, 0x40},
	{0x3027, 0x08},
	{0x3029, 0xC6},
	{0x30BC, 0xb0},
	{0x302B, 0x01},
	{0x30D8, 0x3F},
// ADLC setting ...
	{0x3070, 0x5F},
	{0x3071, 0x00},
	{0x3080, 0x04},
	{0x3081, 0x38},
// MIPI setting
	{0x30BD, 0x00},//SEL_CCP[0]
	{0x3084, 0x15},//SYNC Mode
	{0x30BE, 0x1A},//M_PCLKDIV_AUTO[4], M_DIV_PCLK[3:0]
	{0x30C1, 0x01},//pack video enable [0]
	{0x30EE, 0x02},//DPHY enable [1]
	{0x3111, 0x86},//Embedded data off [5]
};

static struct v4l2_subdev_info s5k4e1gx_semco_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_conf_array s5k4e1gx_semco_init_conf[] = {
	{&s5k4e1gx_semco_recommend_settings[0],
	ARRAY_SIZE(s5k4e1gx_semco_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array s5k4e1gx_semco_confs[] = {
	{&s5k4e1gx_semco_snap_settings[0],
	ARRAY_SIZE(s5k4e1gx_semco_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k4e1gx_semco_prev_settings[0],
	ARRAY_SIZE(s5k4e1gx_semco_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_sensor_output_info_t s5k4e1gx_semco_dimensions[] = {
	/* snapshot */
    {
		.x_output = 0XA20,//is 2592x1960
		.y_output = 0X7A8,
		.line_length_pclk = 0X0AB2,
		.frame_length_lines = 0X07B4,
		.vt_pixel_clk =80800000,
		.op_pixel_clk =80800000,//80800000,
		.binning_factor = 1,
    },
	/* preview */
    {
		.x_output = 0X518,//is 1304*980
		.y_output = 0X3d4,
		.line_length_pclk = 0X0AB2,
		.frame_length_lines = 0X03E0,
		.vt_pixel_clk =80800000,
		.op_pixel_clk =80800000,//80800000,
		.binning_factor = 1,
    },
}; 

static struct msm_camera_csid_vc_cfg s5k4e1gx_semco_cid_cfg[] = {
	{0, CSI_RAW10, CSI_DECODE_10BIT},
    {1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params s5k4e1gx_semco_csi_params = {
	.csid_params = {
		.lane_assign = 0xe4,
		.lane_cnt = 2,
		.lut_params = {
			.num_cid = ARRAY_SIZE(s5k4e1gx_semco_cid_cfg),
			.vc_cfg = s5k4e1gx_semco_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt = 2,
		.settle_cnt = 0x1B,
	},
};

static struct msm_camera_csi2_params *s5k4e1gx_semco_csi_params_array[] = {
	&s5k4e1gx_semco_csi_params,
	&s5k4e1gx_semco_csi_params,
};

static struct msm_sensor_output_reg_addr_t s5k4e1gx_semco_reg_addr = {
	.x_output = 0x34C,
	.y_output = 0x34E,
	.line_length_pclk = 0x342,
	.frame_length_lines = 0x340,
};

static struct msm_sensor_id_info_t s5k4e1gx_semco_id_info = {
	.sensor_id_reg_addr = 0x0000,
	.sensor_id = 0x4E10,
};

static struct msm_sensor_exp_gain_info_t s5k4e1gx_semco_exp_gain_info = {
	.coarse_int_time_addr = 0x0202,
	.global_gain_addr = 0x0204,
	.vert_offset = 4,
};

static inline uint8_t s5k4e1_byte(uint16_t word, uint8_t offset)
{
	return word >> (offset * BITS_PER_BYTE);
}

static int32_t s5k4e1gx_semco_write_prev_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
						uint16_t gain, uint32_t line)
{
	uint16_t max_legal_gain = 0x0200;
	int32_t rc = 0;
	static uint32_t fl_lines, offset;

//	pr_info("s5k4e1_write_prev_exp_gain :%d %d\n", gain, line);
	offset = s_ctrl->sensor_exp_gain_info->vert_offset;
	if (gain > max_legal_gain) {
		CDBG("Max legal gain Line:%d\n", __LINE__);
		gain = max_legal_gain;
	}

	/* Analogue Gain */
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr,
		s5k4e1_byte(gain, MSB),
		MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr + 1,
		s5k4e1_byte(gain, LSB),
		MSM_CAMERA_I2C_BYTE_DATA);

	if (line > (s_ctrl->curr_frame_length_lines - offset)) {
		fl_lines = line + offset;
		s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_output_reg_addr->frame_length_lines,
			s5k4e1_byte(fl_lines, MSB),
			MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_output_reg_addr->frame_length_lines + 1,
			s5k4e1_byte(fl_lines, LSB),
			MSM_CAMERA_I2C_BYTE_DATA);
		/* Coarse Integration Time */
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr,
			s5k4e1_byte(line, MSB),
			MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 1,
			s5k4e1_byte(line, LSB),
			MSM_CAMERA_I2C_BYTE_DATA);
		s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	} else if (line < (fl_lines - offset)) {
		fl_lines = line + offset;
		if (fl_lines < s_ctrl->curr_frame_length_lines)
			fl_lines = s_ctrl->curr_frame_length_lines;

		s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
		/* Coarse Integration Time */
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr,
			s5k4e1_byte(line, MSB),
			MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 1,
			s5k4e1_byte(line, LSB),
			MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_output_reg_addr->frame_length_lines,
			s5k4e1_byte(fl_lines, MSB),
			MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_output_reg_addr->frame_length_lines + 1,
			s5k4e1_byte(fl_lines, LSB),
			MSM_CAMERA_I2C_BYTE_DATA);
		s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	} else {
		fl_lines = line+4;
		s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
		/* Coarse Integration Time */
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr,
			s5k4e1_byte(line, MSB),
			MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 1,
			s5k4e1_byte(line, LSB),
			MSM_CAMERA_I2C_BYTE_DATA);
		s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	}
	return rc;
}

static int32_t s5k4e1gx_semco_write_pict_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t gain, uint32_t line)
{
	uint16_t max_legal_gain = 0x0200;
	uint16_t min_ll_pck = 0x0AB2;
	uint32_t ll_pck, fl_lines;
	uint32_t ll_ratio;
	uint8_t gain_msb, gain_lsb;
	uint8_t intg_time_msb, intg_time_lsb;
	uint8_t ll_pck_msb, ll_pck_lsb;

	if (gain > max_legal_gain) {
		CDBG("Max legal gain Line:%d\n", __LINE__);
		gain = max_legal_gain;
	}

	//gain = 32;
	//line = 1465;
	//pr_info("s5k4e1_write_exp_gain : gain = %d line = %d\n", gain, line);
	line = (uint32_t) (line * s_ctrl->fps_divider);
	fl_lines = s_ctrl->curr_frame_length_lines * s_ctrl->fps_divider / Q10;
	ll_pck = s_ctrl->curr_line_length_pclk;

	if (fl_lines < (line / Q10))
		ll_ratio = (line / (fl_lines - 4));
	else
		ll_ratio = Q10;

	ll_pck = ll_pck * ll_ratio / Q10;
	line = line / ll_ratio;
	if (ll_pck < min_ll_pck)
		ll_pck = min_ll_pck;

	gain_msb = (uint8_t) ((gain & 0xFF00) >> 8);
	gain_lsb = (uint8_t) (gain & 0x00FF);

	intg_time_msb = (uint8_t) ((line & 0xFF00) >> 8);
	intg_time_lsb = (uint8_t) (line & 0x00FF);

	ll_pck_msb = (uint8_t) ((ll_pck & 0xFF00) >> 8);
	ll_pck_lsb = (uint8_t) (ll_pck & 0x00FF);

	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr,
		gain_msb,
		MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr + 1,
		gain_lsb,
		MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->line_length_pclk,
		ll_pck_msb,
		MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->line_length_pclk + 1,
		ll_pck_lsb,
		MSM_CAMERA_I2C_BYTE_DATA);

	/* Coarse Integration Time */
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr,
		intg_time_msb,
		MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 1,
		intg_time_lsb,
		MSM_CAMERA_I2C_BYTE_DATA);
	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);

	return 0;
}

int32_t s5k4e1gx_semco_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
    int rc=0;
	int reset_gpio;
	int vcm_pwd_gpio;
	
	msm_sensor_expand_power_up(s_ctrl);
	
	reset_gpio = get_gpio_num_by_name("CAM_RST");
	vcm_pwd_gpio = get_gpio_num_by_name("CAM_AF_SHDN");
	if(reset_gpio < 0)
	{
		printk(KERN_ERR"%s get_gpio_num_by_name fail\n",__func__);
		return reset_gpio;
	}
	if(vcm_pwd_gpio < 0)
	{
		printk(KERN_ERR"%s get_gpio_num_by_name fail\n",__func__);
		return vcm_pwd_gpio;
	}


	rc = gpio_request(reset_gpio,"s5k4e1gx_8960");
	if (rc) 
	{
		gpio_free(reset_gpio);
		rc = gpio_request(reset_gpio,"s5k4e1gx_8960");
		if(rc)
		{
		    printk("%s gpio_request(%d) again fail \n",__func__,reset_gpio);
			return rc;
		}
		printk("%s gpio_request(%d) again success\n",__func__,reset_gpio);
	}
    rc = gpio_request(vcm_pwd_gpio,"s5k4e1gx_8960");
    if (rc) 
    {
	    gpio_free(vcm_pwd_gpio);
	    rc = gpio_request(vcm_pwd_gpio,"s5k4e1gx_8960");
	    if(rc)
	    {
	        printk("%s gpio_request(%d) again fail \n",__func__,vcm_pwd_gpio);
	        return rc;
	    }
	    printk("%s gpio_request(%d) again success\n",__func__,vcm_pwd_gpio);
    }
    gpio_direction_output(vcm_pwd_gpio, 1);
	rc=gpio_direction_output(reset_gpio, 1);
    if (rc != 0) 
    {
        printk("%s: gpio gpio_direction_output fail", __func__);
    }
        
    mdelay(2);        
    gpio_direction_output(vcm_pwd_gpio, 0);

    mdelay(5);
    gpio_direction_output(vcm_pwd_gpio, 1);
    mdelay(3);

    return rc;
}
int32_t s5k4e1gx_semco_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	int reset_gpio;
	int vcm_pwd_gpio;
	
	reset_gpio = get_gpio_num_by_name("CAM_RST");
	vcm_pwd_gpio = get_gpio_num_by_name("CAM_AF_SHDN");
	if(reset_gpio < 0)
	{
		printk(KERN_ERR"%s get_gpio_num_by_name fail\n",__func__);
		return reset_gpio;
	}
	if(vcm_pwd_gpio < 0)
	{
		printk(KERN_ERR"%s get_gpio_num_by_name fail\n",__func__);
		return vcm_pwd_gpio;
	}
	
    gpio_direction_output(reset_gpio, 0);
	gpio_free(vcm_pwd_gpio);
    gpio_free(reset_gpio);

    msm_sensor_expand_power_down(s_ctrl);
    return 0;
}

#if 0 /* not use temply */
static struct sensor_calib_data s5k4e1gx_semco_calib_data;
#endif

static const struct i2c_device_id s5k4e1gx_semco_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&s5k4e1gx_semco_s_ctrl},
	{ }
};

static struct i2c_driver s5k4e1gx_semco_i2c_driver = {
	.id_table = s5k4e1gx_semco_i2c_id,
	.probe  = msm_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};
static struct msm_camera_i2c_client s5k4e1gx_semco_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

#if 0 /* not use temply */
static struct msm_camera_i2c_client s5k4e1gx_semco_eeprom_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
};

static struct msm_camera_eeprom_read_t s5k4e1gx_semco_eeprom_read_tbl[] = {
	{0x10, &s5k4e1gx_semco_calib_data.r_over_g, 2, 1},
	{0x12, &s5k4e1gx_semco_calib_data.b_over_g, 2, 1},
	{0x14, &s5k4e1gx_semco_calib_data.gr_over_gb, 2, 1},
};

static struct msm_camera_eeprom_data_t s5k4e1gx_semco_eeprom_data_tbl[] = {
	{&s5k4e1gx_semco_calib_data, sizeof(struct sensor_calib_data)},
};

static struct msm_camera_eeprom_client s5k4e1gx_semco_eeprom_client = {
	.i2c_client = &s5k4e1gx_semco_eeprom_i2c_client,
	.i2c_addr = 0xA4,

	.func_tbl = {
		.eeprom_set_dev_addr = NULL,
		.eeprom_init = msm_camera_eeprom_init,
		.eeprom_release = msm_camera_eeprom_release,
		.eeprom_get_data = msm_camera_eeprom_get_data,
	},

	.read_tbl = s5k4e1gx_semco_eeprom_read_tbl,
	.read_tbl_size = ARRAY_SIZE(s5k4e1gx_semco_eeprom_read_tbl),
	.data_tbl = s5k4e1gx_semco_eeprom_data_tbl,
	.data_tbl_size = ARRAY_SIZE(s5k4e1gx_semco_eeprom_data_tbl),
};
#endif

static int __init s5k4e1gx_liteon_init_module(void)
{
	return i2c_add_driver(&s5k4e1gx_semco_i2c_driver);
}

static struct v4l2_subdev_core_ops s5k4e1gx_semco_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops s5k4e1gx_semco_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops s5k4e1gx_semco_subdev_ops = {
	.core = &s5k4e1gx_semco_subdev_core_ops,
	.video  = &s5k4e1gx_semco_subdev_video_ops,
};

static struct msm_sensor_fn_t s5k4e1gx_semco_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain = s5k4e1gx_semco_write_prev_exp_gain,
	.sensor_write_snapshot_exp_gain = s5k4e1gx_semco_write_pict_exp_gain,

	.sensor_setting = msm_sensor_setting,

	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = s5k4e1gx_semco_sensor_power_up,
	.sensor_power_down = s5k4e1gx_semco_sensor_power_down,
	.sensor_csi_setting = msm_sensor_setting1,
};

static struct msm_sensor_reg_t s5k4e1gx_semco_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = s5k4e1gx_semco_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(s5k4e1gx_semco_start_settings),
	.stop_stream_conf = s5k4e1gx_semco_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(s5k4e1gx_semco_stop_settings),
	.group_hold_on_conf = s5k4e1gx_semco_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(s5k4e1gx_semco_groupon_settings),
	.group_hold_off_conf = s5k4e1gx_semco_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(s5k4e1gx_semco_groupoff_settings),
	.init_settings = &s5k4e1gx_semco_init_conf[0],
	.init_size = ARRAY_SIZE(s5k4e1gx_semco_init_conf),
	.mode_settings = &s5k4e1gx_semco_confs[0],
	.output_settings = &s5k4e1gx_semco_dimensions[0],
	.num_conf = ARRAY_SIZE(s5k4e1gx_semco_confs),
};

static struct msm_sensor_ctrl_t s5k4e1gx_semco_s_ctrl = {
	.msm_sensor_reg = &s5k4e1gx_semco_regs,
	.sensor_i2c_client = &s5k4e1gx_semco_sensor_i2c_client,
	.sensor_i2c_addr = 0x6E,
	#if 0 /* not use temply */
	.sensor_eeprom_client = &s5k4e1gx_semco_eeprom_client,
	#endif
	.sensor_output_reg_addr = &s5k4e1gx_semco_reg_addr,
	.sensor_id_info = &s5k4e1gx_semco_id_info,
	.sensor_exp_gain_info = &s5k4e1gx_semco_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csi_params = &s5k4e1gx_semco_csi_params_array[0],
	.msm_sensor_mutex = &s5k4e1gx_semco_mut,
	.sensor_i2c_driver = &s5k4e1gx_semco_i2c_driver,
	.sensor_v4l2_subdev_info = s5k4e1gx_semco_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(s5k4e1gx_semco_subdev_info),
	.sensor_v4l2_subdev_ops = &s5k4e1gx_semco_subdev_ops,
	.func_tbl = &s5k4e1gx_semco_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
};

module_init(s5k4e1gx_liteon_init_module);
MODULE_DESCRIPTION("SAMSUNG S5K4E1GX 5m Bayer sensor driver");
MODULE_LICENSE("GPL v2");
