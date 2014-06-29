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
#include <linux/device.h>
#include <hsad/config_interface.h>
#define SENSOR_NAME "mt9p017_sunny"
#define PLATFORM_DRIVER_NAME "msm_camera_mt9p017_sunny"
#define mt9p017_sunny_obj mt9p017_sunny_##obj
#define WARN_FUNC	printk("%s start\n",__func__);
#define MT9P017_OTP_SUPPORT
DEFINE_MUTEX(mt9p017_sunny_mut);
static struct msm_sensor_ctrl_t mt9p017_sunny_s_ctrl;

static struct msm_camera_i2c_reg_conf mt9p017_sunny_start_settings[] = {
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf mt9p017_sunny_stop_settings[] = {
	{0x0100, 0x00},
};

static struct msm_camera_i2c_reg_conf mt9p017_sunny_groupon_settings[] = {
	{0x104, 0x01},
};

static struct msm_camera_i2c_reg_conf mt9p017_sunny_groupoff_settings[] = {
	{0x104, 0x00},
};

static struct msm_camera_i2c_reg_conf mt9p017_sunny_prev_settings[] ={

 {0x0104, 0x01},//group parameter Hold
 {0x0104, 0x1	},//Grouped Parameter Hold = 0x1          Byte-Access
 {0x034C, 0x0510	},//Output Width = 0x510
 {0x034E, 0x03D0	},//Output Height = 0x3D0
 {0x0344, 0x008	},//Column Start = 0x8
 {0x0346, 0x008	},//Row Start = 0x8
 {0x0348, 0xA27	},//Column End = 0xA25,2593-8=2585
 {0x034A, 0x7A7	},//Row End = 0x79D    1949-8=1941
 {0x1006, 0x4	},	//coarse_integration_time_max_margin
 {0x3040, 0x04C3	},//Read Mode = 0x4C3
 {0x3010, 0x0184	},//Fine Correction = 0x184
 {0x3012, 0x04A0	},//Coarse Integration Time = 0x4A0
 {0x3014, 0x0908	},//Fine Integration Time = 0x908
 {0x0340, 0x04A1	},//Frame Lines = 0x4A1
 {0x0342, 0x0C4C	},//Line Length = 0xC4C
 {0x0104, 0x00  },//group parameter Hold
};

static struct msm_camera_i2c_reg_conf mt9p017_sunny_snap_settings[] = {

{0x0104, 0x01}, //group parameter Hold
{0x3004, 0x0008}, //x_addr_start
{0x3008, 0x0A27}, //x_addr_end 2599-8=2591+1=2592
{0x3002, 0x0008}, //y_start_addr//8
{0x3006, 0x07A7}, //y_addr_end //1959-8=1951+1=1952
{0x3040, 0x0041}, //read_mode  //NO BINNING
{0x034C, 0x0A20}, //x_output_size //
{0x034E, 0x07A0}, //y_output_size
 {0x1006, 0x4	},	//coarse_integration_time_max_margin
{0x300C, 0x0E6E}, //line_length_pck
{0x300A, 0x07E5}, //frame_length_lines	
{0x3012, 0x07E4}, //coarse_integration_time
{0x3014, 0x02CE}, //fine_integration_time
{0x3010, 0x00A0}, //fine_correction
 {0x0104, 0x00}, //group parameter Hold
};


static struct msm_camera_i2c_reg_conf mt9p017_sunny_recommend_settings[] = {
{0x301A, 0x0018},	 //reset_register
{0x0112, 0x0A0A},		//CCP Data Format = 0xA0A
{0x3064, 0xB800},	 //smia_test_2lane_mipi
{0x31AE, 0x0202},	 //MIPI dual interface

 {0x03F0, 0x8010 },//vcm enable
 {0x03F4, 0x028A },//vcm code 650

//LOAD=REV4_recommended_settings
{0x316A, 0x8400}, // RESERVED
{0x316C, 0x8400}, // RESERVED
{0x316E, 0x8400}, // RESERVED
{0x3EFA, 0x1A1F}, // RESERVED
{0x3ED2, 0xD965}, // RESERVED
{0x3ED8, 0x7F1B}, // RESERVED
{0x3EDA, 0xAF11}, // RESERVED
{0x3EE2, 0x0060}, // RESERVED
{0x3EF2, 0xD965}, // RESERVED
{0x3EF8, 0x797F}, // RESERVED
{0x3EFC, 0xA8EF}, // RESERVED
{0x30d4, 0x9200}, // RESERVED
{0x30b2, 0xC000}, // RESERVED
{0x30bc, 0x0400}, // RESERVED
{0x306E, 0xB480}, // RESERVED
{0x3EFE, 0x1F0F}, // RESERVED
{0x31E0, 0x1F01}, // RESERVED
//[REV4_pixel_timing] // new Y3 timing
// @00 Jump Table
{0x3E00, 0x042F},
{0x3E02, 0xFFFF},
{0x3E04, 0xFFFF},
{0x3E06, 0xFFFF},
// @04 Read
{0x3E08, 0x8071},
{0x3E0A, 0x7281},
{0x3E0C, 0x4011},
{0x3E0E, 0x8010},
{0x3E10, 0x60A5},
{0x3E12, 0x4080},
{0x3E14, 0x4180},
{0x3E16, 0x0018},
{0x3E18, 0x46B7},
{0x3E1A, 0x4994},
{0x3E1C, 0x4997},
{0x3E1E, 0x4682},
{0x3E20, 0x0018},
{0x3E22, 0x4241},
{0x3E24, 0x8000},
{0x3E26, 0x1880},
{0x3E28, 0x4785},
{0x3E2A, 0x4992},
{0x3E2C, 0x4997},
{0x3E2E, 0x4780},
{0x3E30, 0x4D80},
{0x3E32, 0x100C},
{0x3E34, 0x8000},
{0x3E36, 0x184A},
{0x3E38, 0x8042},
{0x3E3A, 0x001A},
{0x3E3C, 0x9610},
{0x3E3E, 0x0C80},
{0x3E40, 0x4DC6},
{0x3E42, 0x4A80},
{0x3E44, 0x0018},
{0x3E46, 0x8042},
{0x3E48, 0x8041},
{0x3E4A, 0x0018},
{0x3E4C, 0x804B},
{0x3E4E, 0xB74B},
{0x3E50, 0x8010},
{0x3E52, 0x6056},
{0x3E54, 0x001C},
{0x3E56, 0x8211},
{0x3E58, 0x8056},
{0x3E5A, 0x827C},
{0x3E5C, 0x0970},
{0x3E5E, 0x8082},
{0x3E60, 0x7281},
{0x3E62, 0x4C40},
{0x3E64, 0x8E4D},
{0x3E66, 0x8110},
{0x3E68, 0x0CAF},
{0x3E6A, 0x4D80},
{0x3E6C, 0x100C},
{0x3E6E, 0x8440},
{0x3E70, 0x4C81},
{0x3E72, 0x7C5F},
{0x3E74, 0x7000},
{0x3E76, 0x0000},
{0x3E78, 0x0000},
{0x3E7A, 0x0000},
{0x3E7C, 0x0000},
{0x3E7E, 0x0000},
{0x3E80, 0x0000},
{0x3E82, 0x0000},
{0x3E84, 0x0000},
{0x3E86, 0x0000},
{0x3E88, 0x0000},
{0x3E8A, 0x0000},
{0x3E8C, 0x0000},
{0x3E8E, 0x0000},
{0x3E90, 0x0000},
{0x3E92, 0x0000},
{0x3E94, 0x0000},
{0x3E96, 0x0000},
{0x3E98, 0x0000},
{0x3E9A, 0x0000},
{0x3E9C, 0x0000},
{0x3E9E, 0x0000},
{0x3EA0, 0x0000},
{0x3EA2, 0x0000},
{0x3EA4, 0x0000},
{0x3EA6, 0x0000},
{0x3EA8, 0x0000},
{0x3EAA, 0x0000},
{0x3EAC, 0x0000},
{0x3EAE, 0x0000},
{0x3EB0, 0x0000},
{0x3EB2, 0x0000},
{0x3EB4, 0x0000},
{0x3EB6, 0x0000},
{0x3EB8, 0x0000},
{0x3EBA, 0x0000},
{0x3EBC, 0x0000},
{0x3EBE, 0x0000},
{0x3EC0, 0x0000},
{0x3EC2, 0x0000},
{0x3EC4, 0x0000},
{0x3EC6, 0x0000},
{0x3EC8, 0x0000},
{0x3ECA, 0x0000},
// dynamic power disable
{0x3170, 0x2150},  // Manufacturer-Specific
{0x317A, 0x0150},  // Manufacturer-Specific
{0x3ECC, 0x2200},  // Manufacturer-Specific
{0x3174, 0x0000},  // Manufacturer-Specific
{0x3176, 0X0000},  // Manufacturer-Specific
//MIPI timing settings
{0x31B0, 0x00C4}, //changed from 0x0083
{0x31B2, 0x0064},
{0x31B4, 0x0E77},
{0x31B6, 0x0D24},
{0x31B8, 0x020E},
{0x31BA, 0x0710},
{0x31BC, 0x2A0D},

// Minimum Analog Gain (Normalized 1x Gain)
{0x305E, 0x1824},

//Ext=24MHz, vt_pix_clk=112MHz, op_pix_clk=56MHz
{0x0300, 0x0005 },//vt_pix_clk_div
{0x0302, 0x0001 },//vt_sys_clk_div
{0x0304, 0x0003 },//pre_pll_clk_div
{0x0306, 0x0046 },//pll_multipler
{0x0308, 0x000A },//op_pix_clk_div
{0x030A, 0x0001 },//op_sys_clk_div
};

struct msm_camera_i2c_reg_conf  mt9p017_sunny_rolloff_tbl[] =
{
{0x3600, 0x00D0},
{0x3602, 0x0B0A},
{0x3604, 0x1171},
{0x3606, 0xB84C},
{0x3608, 0x14AD},
{0x360A, 0x0490},
{0x360C, 0xD08E},
{0x360E, 0x0E11},
{0x3610, 0x25CE},
{0x3612, 0x6F8D},
{0x3614, 0x06D0},
{0x3616, 0x102E},
{0x3618, 0x2970},
{0x361A, 0x906F},
{0x361C, 0x0DCF},
{0x361E, 0x0470},
{0x3620, 0xFC8E},
{0x3622, 0x3431},
{0x3624, 0x304A},
{0x3626, 0xE84E},
{0x3640, 0xC2AC},
{0x3642, 0xAF2C},
{0x3644, 0xF1EF},
{0x3646, 0x902F},
{0x3648, 0x3CD0},
{0x364A, 0x8C2C},
{0x364C, 0x738D},
{0x364E, 0x95EE},
{0x3650, 0xDE8D},
{0x3652, 0x3AAA},
{0x3654, 0x3ECD},
{0x3656, 0x308E},
{0x3658, 0x1DEE},
{0x365A, 0x892F},
{0x365C, 0xDF8E},
{0x365E, 0x100D},
{0x3660, 0xD9EE},
{0x3662, 0x0ED0},
{0x3664, 0x77CE},
{0x3666, 0x8151},
{0x3680, 0x2031},
{0x3682, 0x0D6E},
{0x3684, 0x1DD2},
{0x3686, 0x0E91},
{0x3688, 0xB053},
{0x368A, 0x1171},
{0x368C, 0xF3EF},
{0x368E, 0x7B12},
{0x3690, 0x4410},
{0x3692, 0xE2B3},
{0x3694, 0x7070},
{0x3696, 0x5C4D},
{0x3698, 0x59D1},
{0x369A, 0x04D1},
{0x369C, 0xD872},
{0x369E, 0x1E71},
{0x36A0, 0xF52E},
{0x36A2, 0x4DF0},
{0x36A4, 0x7E2C},
{0x36A6, 0xDCD1},
{0x36C0, 0x72AD},
{0x36C2, 0x96AE},
{0x36C4, 0x942F},
{0x36C6, 0x1FB1},
{0x36C8, 0x6F6E},
{0x36CA, 0x2C0D},
{0x36CC, 0x274E},
{0x36CE, 0xBE4F},
{0x36D0, 0x058D},
{0x36D2, 0x7F70},
{0x36D4, 0xA5ED},
{0x36D6, 0xC4CE},
{0x36D8, 0x606D},
{0x36DA, 0x0931},
{0x36DC, 0x9D70},
{0x36DE, 0x142D},
{0x36E0, 0x502E},
{0x36E2, 0xD850},
{0x36E4, 0x2DCD},
{0x36E6, 0x3B31},
{0x3700, 0xC9CE},
{0x3702, 0x2DEF},
{0x3704, 0x85F4},
{0x3706, 0xFA52},
{0x3708, 0x0A75},
{0x370A, 0x5F70},
{0x370C, 0x0730},
{0x370E, 0xC934},
{0x3710, 0xF850},
{0x3712, 0x1975},
{0x3714, 0x68AD},
{0x3716, 0x0FAB},
{0x3718, 0xB8B3},
{0x371A, 0x9652},
{0x371C, 0x5C94},
{0x371E, 0xD6EE},
{0x3720, 0xCC8C},
{0x3722, 0x81D3},
{0x3724, 0x13B2},
{0x3726, 0x0C93},
{0x3782, 0x04A4},
{0x3784, 0x03A0},
{0x37C0, 0xAE0C},
{0x37C2, 0xA84C},
{0x37C4, 0x96EC},
{0x37C6, 0xBF2C},
{0x3780, 0x8000},
};
/////////////////////////////// add for MT9P017 sunny OTP lens shading

static struct v4l2_subdev_info mt9p017_sunny_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_conf_array mt9p017_sunny_init_conf[] = {
	{&mt9p017_sunny_recommend_settings[0],
	ARRAY_SIZE(mt9p017_sunny_recommend_settings), 5, MSM_CAMERA_I2C_WORD_DATA}
};

static struct msm_camera_i2c_conf_array mt9p017_sunny_confs[] = {
	{&mt9p017_sunny_snap_settings[0],
	ARRAY_SIZE(mt9p017_sunny_snap_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
	{&mt9p017_sunny_prev_settings[0],
	ARRAY_SIZE(mt9p017_sunny_prev_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
};

static struct msm_sensor_output_info_t mt9p017_sunny_dimensions[] = {
	/* snapshot */
    {
		.x_output = 0x0A20,
		.y_output = 0x07A0,
		.line_length_pclk = 0x0E6E,
		.frame_length_lines = 0x07E5,
		.vt_pixel_clk = 112000000,
		.op_pixel_clk = 112000000,
		.binning_factor = 1,
    },
	/* preview */
    {
		.x_output = 0x510,
		.y_output = 0x3D0,
		.line_length_pclk = 0xc4c,//0xc4c,
		.frame_length_lines = 0x04A1,//0x4a1,
		.vt_pixel_clk = 112000000,
		.op_pixel_clk = 112000000,
		.binning_factor = 1,
    },
};

static struct msm_camera_csid_vc_cfg mt9p017_sunny_cid_cfg[] = {
	{0, CSI_RAW10, CSI_DECODE_10BIT},
    {1, CSI_EMBED_DATA, CSI_DECODE_8BIT},

};

static struct msm_camera_csi2_params mt9p017_sunny_csi_params = {
	.csid_params = {
		.lane_assign = 0xe4,
		.lane_cnt = 2,
		.lut_params = {
			.num_cid = ARRAY_SIZE(mt9p017_sunny_cid_cfg),
			.vc_cfg = mt9p017_sunny_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt = 2,
		.settle_cnt = 0x1B,
	},
};
static struct msm_camera_csi2_params *mt9p017_sunny_csi_params_array[] = {
	&mt9p017_sunny_csi_params,
	&mt9p017_sunny_csi_params,
};

static struct msm_sensor_output_reg_addr_t mt9p017_sunny_reg_addr = {
	.x_output = 0x34C,
	.y_output = 0x34E,
	.line_length_pclk = 0x342,
	.frame_length_lines = 0x340,
};

static struct msm_sensor_id_info_t mt9p017_sunny_id_info = {
	.sensor_id_reg_addr = 0x0000,
	.sensor_id = 0x4800,
};

static struct msm_sensor_exp_gain_info_t mt9p017_sunny_exp_gain_info = {
	.coarse_int_time_addr = 0x0202,//coarse_integration_time,( coarse integration time = frame_length_lines - coarse_integration_time_max_margin)
	.global_gain_addr = 0x305E,//analogue_gain_code_globa
	.vert_offset = 4,//coarse_integration_time_max_margin
};

#define TRUE    1
#define FALSE   0
//for read lens shading from eeprom 
#define LC_TABLE_SIZE 106//equal to the size of mt9p017_lc_tbl - 1.
static const unsigned short mt9p017_eeprom_table[LC_TABLE_SIZE] = {
	0x3800,
    0x3802,
    0x3804,
    0x3806,
    0x3808,
    0x380A,
    0x380C,
    0x380E,
    0x3810,
    0x3812,
    0x3814,
    0x3816,
    0x3818,
    0x381A,
    0x381C,
    0x381E,
    0x3820,
    0x3822,
    0x3824,
    0x3826,
    0x3828,
    0x382A,
    0x382C,
    0x382E,
    0x3830,
    0x3832,
    0x3834,
    0x3836,
    0x3838,
    0x383A,
    0x383C,
    0x383E,
    0x3840,
    0x3842,
    0x3844,
    0x3846,
    0x3848,
    0x384A,
    0x384C,
    0x384E,
    0x3850,
    0x3852,
    0x3854,
    0x3856,
    0x3858,
    0x385A,
    0x385C,
    0x385E,
    0x3860,
    0x3862,
    0x3864,
    0x3866,
    0x3868,
    0x386A,
    0x386C,
    0x386E,
    0x3870,
    0x3872,
    0x3874,
    0x3876,
    0x3878,
    0x387A,
    0x387C,
    0x387E,
    0x3880,
    0x3882,
    0x3884,
    0x3886,
    0x3888,
    0x388A,
    0x388C,
    0x388E,
    0x3890,
    0x3892,
    0x3894,
    0x3898,
    0x389A,
    0x389C,
    0x389E,
    0x38A0,
    0x38A2,
    0x38A4,
    0x38A6,
    0x38A8,
    0x38AA,
    0x38AC,
    0x38B0,
    0x38B2,
    0x38B4,
    0x38B6,
    0x38B8,
    0x38BA,
    0x38BC,
    0x38BE,
    0x38C0,
    0x38C2,
    0x38C4,
    0x38C6,
    0x38C8,
    0x38CA,
    0x38CC,
    0x38CE,
    0x38D0,
    0x38D2,
    0x38D4,
    0x38D6,
};
static bool bSuccess = FALSE;
static unsigned short mt9p017_rolloff_reg_data[LC_TABLE_SIZE];
static int32_t mt9p017_sunny_sensor_read_otp(struct msm_sensor_ctrl_t *s_ctrl)
{
    int32_t rc;
    bool bRWFinished = FALSE;
    bool bRWSuccess = FALSE;
    unsigned short j, i;
    unsigned short OTPCheckValue = 0;
    unsigned short DataStartType = 0x3100;

    CDBG("%s  Enter \n",__func__);
    bSuccess = FALSE;
    memset(mt9p017_rolloff_reg_data, 0, sizeof(unsigned short)*LC_TABLE_SIZE);

    CDBG("%s: Before read dataStartType = 0x%x\n", __func__, DataStartType);
    while(!bSuccess)
    {
       
        msm_camera_i2c_write(s_ctrl->sensor_i2c_client,//disable Stream
                       0x301A ,0x0610 , MSM_CAMERA_I2C_WORD_DATA);


        msm_camera_i2c_write(s_ctrl->sensor_i2c_client,//timing parameters for OTPM read
                       0x3134 ,0xCD95 , MSM_CAMERA_I2C_WORD_DATA);


        msm_camera_i2c_write(s_ctrl->sensor_i2c_client,//0x304C [15:8] for record type
                       0x304C ,DataStartType , MSM_CAMERA_I2C_WORD_DATA);


        msm_camera_i2c_write(s_ctrl->sensor_i2c_client,//disable Stream
                       0x304A ,0x0200 , MSM_CAMERA_I2C_WORD_DATA);

      
         msm_camera_i2c_write(s_ctrl->sensor_i2c_client,//auto Read Start
                     0x304A ,0x0210 , MSM_CAMERA_I2C_WORD_DATA);


        bRWFinished = FALSE;
        bRWSuccess = FALSE;

        for(j = 0; j<10; j++)//POLL Register 0x304A [6:5] = 11 //auto read success
        {
            mdelay(10);
            rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client,
                           0x304A, &OTPCheckValue , MSM_CAMERA_I2C_WORD_DATA);//auto Read Start

//            CDBG("%s:read count=%d, CheckValue=0x%x \n", __func__, j, OTPCheckValue);
            if(0xFFFF == (OTPCheckValue |0xFFDF))//finish
            {
                bRWFinished = TRUE;
                if(0xFFFF == (OTPCheckValue |0xFFBF))//success
                {
                    bRWSuccess = TRUE;
                }
                break;
            }
        }
//        CDBG("%s: read DataStartType = 0x%x, bRWFinished = %d, bRWSuccess = %d \n", __func__,DataStartType, bRWFinished, bRWSuccess);

        if(!bRWFinished)
        {
//            CDBG("%s: read DataStartType Fail! \n", __func__);
            goto OTPERR;
        }
        else
        {
            if(bRWSuccess)
            {
                switch(DataStartType)
                {
                    case 0x3000:
                    case 0x3200:
                    bSuccess = TRUE;
                    break;

                    case 0x3100:
                    bSuccess = FALSE;
                    DataStartType = 0x3000;
                    break;
                    
                    default:
                        break;
                }
            }
            else
            {
                switch(DataStartType)
                {
                    case 0x3200:
                    {
                        bSuccess = FALSE;
//                        CDBG("%s: read DataStartType Error Times Twice! \n", __func__);
                        goto OTPERR;
                    }
                    break;
                    case 0x3100:
                    {
                        bSuccess = FALSE;
                        DataStartType = 0x3200;
                    }
                    break;
                    case 0x3000:
                    {
                        bSuccess = TRUE;
                        DataStartType = 0x3100;
                    }
                    break;
                    default:
                        break;
                }
            }
        }
    }
    CDBG("%s: after read DataStartType = 0x%x \n", __func__,DataStartType);

    //read data
    bSuccess = FALSE;

    msm_camera_i2c_write(s_ctrl->sensor_i2c_client,//disable Stream
                   0x301A ,0x0610 , MSM_CAMERA_I2C_WORD_DATA);
    
    
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client,//timing parameters for OTPM read
                   0x3134 ,0xCD95 , MSM_CAMERA_I2C_WORD_DATA);
    
    
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client,//0x304C [15:8] for record type
                   0x304C ,DataStartType , MSM_CAMERA_I2C_WORD_DATA);
    
    
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client,//disable Stream
                   0x304A ,0x0200 , MSM_CAMERA_I2C_WORD_DATA);
    
    
     msm_camera_i2c_write(s_ctrl->sensor_i2c_client,//auto Read Start
                 0x304A ,0x0210 , MSM_CAMERA_I2C_WORD_DATA);

    
    for(j = 0; j<10; j++)//POLL Register 0x304A [6:5] = 11 //auto read success
    {
        mdelay(10);
        rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client,
                       0x304A, &OTPCheckValue , MSM_CAMERA_I2C_WORD_DATA);//auto Read Start

//        CDBG("%s:read count=%d, CheckValue=0x%x \n", __func__, j, OTPCheckValue);
        if(0xFFFF == (OTPCheckValue |0xFFDF))//finish
        {
            bRWFinished = TRUE;
            if(0xFFFF == (OTPCheckValue |0xFFBF))//success
            {
                bRWSuccess = TRUE;
            }
            break;
        }
    }

    CDBG("%s:: read DataStartType(step2) = 0x%x, bRWFinished = %d, bRWSuccess = %d \n" 
        , __func__,DataStartType, bRWFinished, bRWSuccess);
    if(!bRWFinished ||!bRWSuccess)
    {
        CDBG("%s: read DataStartType Error!Failed! \n", __func__);
        goto OTPERR;
    }
    else 
    {
//        CDBG("%s:read 0x3800 to 0x39FE for the written data \n", __func__);
        for(i = 0; i<LC_TABLE_SIZE; i++)//READ 0x3800 to 0x39FE for the written data
        {	
           msm_camera_i2c_read(s_ctrl->sensor_i2c_client,
                   mt9p017_eeprom_table[i], &OTPCheckValue , MSM_CAMERA_I2C_WORD_DATA );
            mt9p017_rolloff_reg_data[i] = OTPCheckValue;
//            CDBG("%s:read:mt9p017_eeprom_table[%d]=0x%x,mt9p017_reg_data[%d]=0x%x \n",
//                __func__,i,mt9p017_eeprom_table[i], i,mt9p017_rolloff_reg_data[i]);
        }
        
        bSuccess = TRUE;
    }
OTPERR:
    return rc;
}
static int32_t mt9p017_sunny_sensor_write_otp(struct msm_sensor_ctrl_t *s_ctrl)
{
    int32_t  rc;
    unsigned short  i;
    if (bSuccess)
    {
        //write lens shading to sensor registers
        for(i=0;i<LC_TABLE_SIZE;i++)
        {
        
        msm_camera_i2c_write(s_ctrl->sensor_i2c_client,//auto Read Start
                    mt9p017_sunny_rolloff_tbl[i].reg_addr ,
                    mt9p017_rolloff_reg_data[i]
                    , MSM_CAMERA_I2C_WORD_DATA);
        }
        rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 
            0x3780, 0x8000, MSM_CAMERA_I2C_WORD_DATA );

        CDBG("%s: OTP Check OK!!!  rc = %d \n", __func__,rc);
    }
    else
    {
        CDBG("ESEN __DEBUG %s: OTP Check Fail Write Fail! \n", __func__);
    //    msm_camera_i2c_write_tbl(s_ctrl->sensor_i2c_client,
       //     mt9p017_sunny_rolloff_tbl,
          //     ARRAY_SIZE(mt9p017_sunny_rolloff_tbl),MSM_CAMERA_I2C_WORD_DATA );
       //  CDBG("%s: OTP Check Fail rc = %d", __func__,rc);
    } 
    
    CDBG("%s  Exit \n",__func__);
    return rc;
}

int32_t mt9p017_sunny_sensor_setting_otp(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res)
{
	int32_t rc = 0;

	v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
		NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
		PIX_0, ISPIF_OFF_IMMEDIATELY));
	s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);
	mdelay(30);
	if (update_type == MSM_SENSOR_REG_INIT) {
		s_ctrl->curr_csi_params = NULL;
		msm_sensor_enable_debugfs(s_ctrl);
		mt9p017_sunny_sensor_read_otp(s_ctrl);
		msm_sensor_write_init_settings(s_ctrl);
		mt9p017_sunny_sensor_write_otp(s_ctrl);
	} else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
		msm_sensor_write_res_settings(s_ctrl, res);
		if (s_ctrl->curr_csi_params != s_ctrl->csi_params[res]) {
			s_ctrl->curr_csi_params = s_ctrl->csi_params[res];
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_CSID_CFG,
				&s_ctrl->curr_csi_params->csid_params);
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
						NOTIFY_CID_CHANGE, NULL);
			mb();
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_CSIPHY_CFG,
				&s_ctrl->curr_csi_params->csiphy_params);
			mb();
			mdelay(20);
		}

		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_PCLK_CHANGE, &s_ctrl->msm_sensor_reg->
			output_settings[res].op_pixel_clk);
		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
			PIX_0, ISPIF_ON_FRAME_BOUNDARY));
		s_ctrl->func_tbl->sensor_start_stream(s_ctrl);
		mdelay(30);
	}
	return rc;
}
#define SENSOR_PREVIEW_RES 1
#define SENSOR_SNAPSHOT_RES 0

int32_t mt9p017_sunny_write_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t gain, uint32_t line)
{
	uint32_t fl_lines;
	uint8_t offset;
	//uint16_t read_gain,read_coarse;
    uint16_t max_legal_gain = 0x1E7F;

    if (gain > max_legal_gain)
    {
        gain = max_legal_gain;
    }
	gain=gain|0x1000;
	
	fl_lines = s_ctrl->curr_frame_length_lines;
	fl_lines = (fl_lines * s_ctrl->fps_divider) / Q10;
	offset = s_ctrl->sensor_exp_gain_info->vert_offset;
	if (line > (fl_lines - offset))
		fl_lines = line + offset;

	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
//	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
//		s_ctrl->sensor_output_reg_addr->frame_length_lines, fl_lines,
//		MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr, line,
		MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr, gain,
		MSM_CAMERA_I2C_WORD_DATA);
	#if 0
	printk("ESEN: __debug: %s,addr_type=%d,coarse_int_time_addr=%x,\
		global_gain_addr=%x,set_gain=%x,set_coarse=%x \n",
		__func__,s_ctrl->sensor_i2c_client->addr_type,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr,
		s_ctrl->sensor_exp_gain_info->global_gain_addr,
		gain,line);
		msm_camera_i2c_read(s_ctrl->sensor_i2c_client,
		0x3012, &read_coarse,	MSM_CAMERA_I2C_WORD_DATA);//0x0202
		msm_camera_i2c_read(s_ctrl->sensor_i2c_client,
		0x305E, &read_gain,MSM_CAMERA_I2C_WORD_DATA);
	printk("ESEN: __debug:%s,,addr_type=%d,read_gain=%x,read_coarse=%x\n",
		__func__,s_ctrl->sensor_i2c_client->addr_type,read_gain,read_coarse);
	#endif
	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	return 0;
}

int32_t mt9p017_sunny_write_pict_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t gain, uint32_t line)
{
	mt9p017_sunny_write_exp_gain(s_ctrl,gain,line);
	mdelay(1);
	return 0;
}

//////////////////////////////////////////////////////////////////
int32_t mt9p017_sunny_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
    int rc=0;
    int reset_gpio;
	int vcm_pwd_gpio;
	printk("%s enter \n",__func__);

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


	rc = gpio_request(reset_gpio,"mt9p017_sunny");
	if(rc)
	{
		gpio_free(reset_gpio);
		rc = gpio_request(reset_gpio,"mt9p017_sunny");
		if(rc)
		{
		    printk("%s gpio_request(%d) again fail \n",__func__,reset_gpio);
			return rc;
		}
		printk("%s gpio_request(%d) again success\n",__func__,reset_gpio);
	}
	if (!rc) {
    rc = gpio_request(vcm_pwd_gpio,"mt9p017_sunny");
    if (rc) 
    {
	    gpio_free(vcm_pwd_gpio);
	    rc = gpio_request(vcm_pwd_gpio,"mt9p017_sunny");
	    if(rc)
	    {
	        printk("%s gpio_request(%d) again fail \n",__func__,vcm_pwd_gpio);
	        return rc;
	    }
	    printk("%s gpio_request(%d) again success\n",__func__,vcm_pwd_gpio);
    }

    gpio_direction_output(vcm_pwd_gpio, 1);
		rc=gpio_direction_output(reset_gpio, 1);
        if (rc != 0) {
                printk("%s: gpio gpio_direction_output fail", __func__);
            }

        mdelay(2);
    gpio_direction_output(vcm_pwd_gpio, 0);
		rc=gpio_direction_output(reset_gpio, 0);
        if (rc != 0) {
                printk("%s: gpio gpio_direction_output fail", __func__);
            }

        mdelay(5);
    gpio_direction_output(vcm_pwd_gpio, 1);
		rc=gpio_direction_output(reset_gpio, 1);
        if (rc != 0) {
                printk("%s: gpio gpio_direction_output fail", __func__);
            }
        mdelay(3);

	} else {
		printk("%s: gpio request fail", __func__);
	}

    return rc;
}

int32_t mt9p017_sunny_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
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
static struct sensor_calib_data mt9p017_sunny_calib_data;
#endif

static const struct i2c_device_id mt9p017_sunny_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&mt9p017_sunny_s_ctrl},
	{ }
};

static struct i2c_driver mt9p017_sunny_i2c_driver = {
	.id_table = mt9p017_sunny_i2c_id,
	.probe  = msm_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client mt9p017_sunny_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

#if 0 /* not use temply */
static struct msm_camera_i2c_client mt9p017_sunny_eeprom_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
};

static struct msm_camera_eeprom_read_t mt9p017_sunny_eeprom_read_tbl[] = {
	{0x10, &mt9p017_sunny_calib_data.r_over_g, 2, 1},
	{0x12, &mt9p017_sunny_calib_data.b_over_g, 2, 1},
	{0x14, &mt9p017_sunny_calib_data.gr_over_gb, 2, 1},
};

static struct msm_camera_eeprom_data_t mt9p017_sunny_eeprom_data_tbl[] = {
	{&mt9p017_sunny_calib_data, sizeof(struct sensor_calib_data)},
};

static struct msm_camera_eeprom_client mt9p017_sunny_eeprom_client = {
	.i2c_client = &mt9p017_sunny_eeprom_i2c_client,
	.i2c_addr = 0xA4,

	.func_tbl = {
		.eeprom_set_dev_addr = NULL,
		.eeprom_init = msm_camera_eeprom_init,
		.eeprom_release = msm_camera_eeprom_release,
		.eeprom_get_data = msm_camera_eeprom_get_data,
	},

	.read_tbl = mt9p017_sunny_eeprom_read_tbl,
	.read_tbl_size = ARRAY_SIZE(mt9p017_sunny_eeprom_read_tbl),
	.data_tbl = mt9p017_sunny_eeprom_data_tbl,
	.data_tbl_size = ARRAY_SIZE(mt9p017_sunny_eeprom_data_tbl),
};
#endif

static int __init msm_sensor_init_module(void)
{
	return i2c_add_driver(&mt9p017_sunny_i2c_driver);
}

static struct v4l2_subdev_core_ops mt9p017_sunny_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops mt9p017_sunny_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops mt9p017_sunny_subdev_ops = {
	.core = &mt9p017_sunny_subdev_core_ops,
	.video  = &mt9p017_sunny_subdev_video_ops,
};

static struct msm_sensor_fn_t mt9p017_sunny_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain = mt9p017_sunny_write_exp_gain,
	.sensor_write_snapshot_exp_gain = mt9p017_sunny_write_pict_exp_gain,
	.sensor_setting = mt9p017_sunny_sensor_setting_otp ,//msm_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = mt9p017_sunny_sensor_power_up,
	.sensor_power_down =mt9p017_sunny_sensor_power_down,
    .sensor_csi_setting = msm_sensor_setting1,
};

static struct msm_sensor_reg_t mt9p017_sunny_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = mt9p017_sunny_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(mt9p017_sunny_start_settings),
	.stop_stream_conf = mt9p017_sunny_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(mt9p017_sunny_stop_settings),
	.group_hold_on_conf = mt9p017_sunny_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(mt9p017_sunny_groupon_settings),
	.group_hold_off_conf = mt9p017_sunny_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(mt9p017_sunny_groupoff_settings),
	.init_settings = &mt9p017_sunny_init_conf[0],
	.init_size = ARRAY_SIZE(mt9p017_sunny_init_conf),
	.mode_settings = &mt9p017_sunny_confs[0],
	.output_settings = &mt9p017_sunny_dimensions[0],
	.num_conf = ARRAY_SIZE(mt9p017_sunny_confs),
};

static struct msm_sensor_ctrl_t mt9p017_sunny_s_ctrl = {
	.msm_sensor_reg = &mt9p017_sunny_regs,
	.sensor_i2c_client = &mt9p017_sunny_sensor_i2c_client,
	.sensor_i2c_addr = 0x6c,
	#if 0 /* not use temply */
	.sensor_eeprom_client = &mt9p017_sunny_eeprom_client,
	#endif
	.sensor_output_reg_addr = &mt9p017_sunny_reg_addr,
	.sensor_id_info = &mt9p017_sunny_id_info,
	.sensor_exp_gain_info = &mt9p017_sunny_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csi_params = &mt9p017_sunny_csi_params_array[0],
	.msm_sensor_mutex = &mt9p017_sunny_mut,
	.sensor_i2c_driver = &mt9p017_sunny_i2c_driver,
	.sensor_v4l2_subdev_info = mt9p017_sunny_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(mt9p017_sunny_subdev_info),
	.sensor_v4l2_subdev_ops = &mt9p017_sunny_subdev_ops,
	.func_tbl = &mt9p017_sunny_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("MT9P017 5m Bayer sensor driver");
MODULE_LICENSE("GPL v2");
