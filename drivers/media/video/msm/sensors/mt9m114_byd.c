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
#define SENSOR_NAME "mt9m114_byd"
#define PLATFORM_DRIVER_NAME "msm_camera_mt9m114_byd"
#define mt9m114_byd_obj mt9m114_byd_##obj

/* Sysctl registers */
#define MT9M114_BYD_COMMAND_REGISTER                0x0080
#define MT9M114_BYD_COMMAND_REGISTER_APPLY_PATCH    (1 << 0)
#define MT9M114_BYD_COMMAND_REGISTER_SET_STATE      (1 << 1)
#define MT9M114_BYD_COMMAND_REGISTER_REFRESH        (1 << 2)
#define MT9M114_BYD_COMMAND_REGISTER_WAIT_FOR_EVENT (1 << 3)
#define MT9M114_BYD_COMMAND_REGISTER_OK             (1 << 15)

DEFINE_MUTEX(mt9m114_byd_mut);
static struct msm_sensor_ctrl_t mt9m114_byd_s_ctrl;

static struct msm_camera_i2c_reg_conf mt9m114_byd_960p_settings[] = {
	{0xdc00, 0x50, MSM_CAMERA_I2C_BYTE_DATA, MSM_CAMERA_I2C_CMD_WRITE},

	{MT9M114_BYD_COMMAND_REGISTER, (MT9M114_BYD_COMMAND_REGISTER_OK |
		MT9M114_BYD_COMMAND_REGISTER_SET_STATE), MSM_CAMERA_I2C_WORD_DATA,
		MSM_CAMERA_I2C_CMD_WRITE},


	{0x098E, 0, MSM_CAMERA_I2C_BYTE_DATA},


	//[1280x960]
	{ 0xC800, 0x0004,},		//cam_sensor_cfg_y_addr_start = 4                        
	{ 0xC802, 0x0004,},		//cam_sensor_cfg_x_addr_start = 4                        
	{ 0xC804, 0x03CB,},		//cam_sensor_cfg_y_addr_end = 971                        
	{ 0xC806, 0x050B,},		//cam_sensor_cfg_x_addr_end = 1291                       
	{ 0xC808, 0x02DC,},		//cam_sensor_cfg_pixclk = 48000000                       
	{ 0xC80A, 0x6C00,},	                                                                 
	{ 0xC80C, 0x0001,},		//cam_sensor_cfg_row_speed = 1                           
	{ 0xC80E, 0x00DB,},		//cam_sensor_cfg_fine_integ_time_min = 219               
	{ 0xC810, 0x05C1,},		//cam_sensor_cfg_fine_integ_time_max = 1473              
	{ 0xC812, 0x03F3,},		//cam_sensor_cfg_frame_length_lines = 1011                                        
	{ 0xC814, 0x0644,},		//cam_sensor_cfg_line_length_pck = 1604                                           
	{ 0xC816, 0x0060,},		//cam_sensor_cfg_fine_correction = 96                                             
	{ 0xC818, 0x03C3,},		//cam_sensor_cfg_cpipe_last_row = 963                                             
	{ 0xC826, 0x0020,},		//cam_sensor_cfg_reg_0_data = 32                                                  
	{ 0xC834, 0x0003,},		//cam_sensor_control_read_mode = 0                                                
	{ 0xC854, 0x0000,},		//cam_crop_window_xoffset = 0                                                     
	{ 0xC856, 0x0000,},		//cam_crop_window_yoffset = 0                                                    9
	{ 0xC858, 0x0500,},		//cam_crop_window_width = 1280                                                   7
	{ 0xC85A, 0x03C0,},		//cam_crop_window_height = 960                                                   1
	{ 0xC85C, 0x03,MSM_CAMERA_I2C_BYTE_DATA},	//cam_crop_cropmode = 3                                                                   
	{ 0xC868, 0x0500,},		//cam_output_width = 1280                                                         
	{ 0xC86A, 0x03C0,},		//cam_output_height = 960                                                         
	{ 0xC878, 0x00,MSM_CAMERA_I2C_BYTE_DATA},	//cam_aet_aemode = 0                                                                      
	{ 0xC88C, 0x1D99,},		//cam_aet_max_frame_rate = 7577                                                   
	{ 0xC88E, 0x0A00,},		//cam_aet_min_frame_rate = 7577                                                   
	{ 0xC914, 0x0000,},		//cam_stat_awb_clip_window_xstart = 0                                             
	{ 0xC916, 0x0000,},		//cam_stat_awb_clip_window_ystart = 0                                             
	{ 0xC918, 0x04FF,},		//cam_stat_awb_clip_window_xend = 1279                                            
	{ 0xC91A, 0x03BF,},		//cam_stat_awb_clip_window_yend = 959                                             
	{ 0xC91C, 0x0000,},		//cam_stat_ae_initial_window_xstart = 0                                           
	{ 0xC91E, 0x0000,},		//cam_stat_ae_initial_window_ystart = 0                                           
	{ 0xC920, 0x00FF,},		//cam_stat_ae_initial_window_xend = 255                                           
	{ 0xC922, 0x00BF,},		//cam_stat_ae_initial_window_yend = 191     

};


static struct msm_camera_i2c_reg_conf mt9m114_byd_recommend_settings[] = {
	{0x301A, 0x0230, MSM_CAMERA_I2C_SET_WORD_MASK},
	{0x098E, 0x1000, },
	/*cam_sysctl_pll_enable = 1*/
	{0xC97E, 0x01, MSM_CAMERA_I2C_BYTE_DATA},
	/*cam_sysctl_pll_divider_m_n = 288*/
	{0xC980, 0x0120,},
	/*cam_sysctl_pll_divider_p = 1792*/
	{0xC982, 0x0700,},


{ 0xC800, 0x0004,},		//cam_sensor_cfg_y_addr_start = 4      
{ 0xC802, 0x0004,},		//cam_sensor_cfg_x_addr_start = 4      
{ 0xC804, 0x03CB,},		//cam_sensor_cfg_y_addr_end = 971      
{ 0xC806, 0x050B,},		//cam_sensor_cfg_x_addr_end = 1291     
{ 0xC808, 0x02DC,},		//cam_sensor_cfg_pixclk = 48000000     
{ 0xC80A, 0x6C00,},		//cam_sensor_cfg_pixclk = 48000000     
{ 0xC80C, 0x0001,},		//cam_sensor_cfg_row_speed = 1         
{ 0xC80E, 0x00DB,},		//cam_sensor_cfg_fine_integ_time_min = 
{ 0xC810, 0x05C1,},		//cam_sensor_cfg_fine_integ_time_max = 
{ 0xC812, 0x03F3,},		//cam_sensor_cfg_frame_length_lines = 1
{ 0xC814, 0x0644,},		//cam_sensor_cfg_line_length_pck = 1604
{ 0xC816, 0x0060,},		//cam_sensor_cfg_fine_correction = 96  
{ 0xC818, 0x03C3,},		//cam_sensor_cfg_cpipe_last_row = 963  
{ 0xC826, 0x0020,},		//cam_sensor_cfg_reg_0_data = 32       
{ 0xC834, 0x0003,},		//cam_sensor_control_read_mode = 0     
{ 0xC854, 0x0000,},		//cam_crop_window_xoffset = 0          
{ 0xC856, 0x0000,},		//cam_crop_window_yoffset = 0          
{ 0xC858, 0x0500,},		//cam_crop_window_width = 1280         
{ 0xC85A, 0x03C0,},		//cam_crop_window_height = 960         
{ 0xC85C, 0x03,MSM_CAMERA_I2C_BYTE_DATA},	  //cam_crop_cropmode = 3                      
{ 0xC868, 0x0500,},		//cam_output_width = 1280              
{ 0xC86A, 0x03C0,},		//cam_output_height = 960              
{ 0xC878, 0x00,MSM_CAMERA_I2C_BYTE_DATA},	  //cam_aet_aemode = 0                         
{ 0xC88C, 0x1D99,},		//cam_aet_max_frame_rate = 7577        
{ 0xC88E, 0x0A00,},		//cam_aet_min_frame_rate = 2560        
{ 0xC914, 0x0000,},		//cam_stat_awb_clip_window_xstart = 0  
{ 0xC916, 0x0000,},		//cam_stat_awb_clip_window_ystart = 0  
{ 0xC918, 0x04FF,},		//cam_stat_awb_clip_window_xend = 1279 
{ 0xC91A, 0x03BF,},		//cam_stat_awb_clip_window_yend = 959  
{ 0xC91C, 0x0000,},		//cam_stat_ae_initial_window_xstart = 0
{ 0xC91E, 0x0000,},		//cam_stat_ae_initial_window_ystart = 0
{ 0xC920, 0x00FF,},		//cam_stat_ae_initial_window_xend = 255
{ 0xC922, 0x00BF,},		//cam_stat_ae_initial_window_yend = 191

{0xDC00, 0x28 ,MSM_CAMERA_I2C_BYTE_DATA},	// SYSMGR_NEXT_STAT
{0x0080, 0x8002,}, 		// COMMAND_REGISTER



	{ 0x316A, 0x8270,}, 	// DAC_TXLO_ROW 
	{ 0x316C, 0x8270,}, 	// DAC_TXLO     
	{ 0x3ED0, 0x2305,}, 	// DAC_LD_4_5   
	{ 0x3ED2, 0x77CF,}, 	// DAC_LD_6_7   
	{ 0x316E, 0x8202,}, 	// DAC_ECL      
	{ 0x3180, 0x87FF,}, 	// DELTA_DK_CONT
	{ 0x30D4, 0x6080,}, 	// COLUMN_CORREC
	{ 0xA802, 0x0008,},                     
	{ 0x3E14, 0xFF39,}, 	// SAMP_COL_PUP2

//MIPI setting for SOC1040
//{ WRITE, { 0x3C5A0002, 0x0009 } },
//{ WRITE, { 0x3C440002, 0x0080 } },      /*MIPI_CUSTOM_SHORT_PKT*/

//Patch 1204
{0x0982, 0x0001,}, 	// ACCESS_CTL_STAT
{0x098A, 0x60BC,},  	// PHYSICAL_ADDRESS_ACCESS
{0xE0BC, 0xC0F1,}, 
{0xE0BE, 0x082A,}, 
{0xE0C0, 0x05A0,}, 
{0xE0C2, 0xD800,}, 
{0xE0C4, 0x71CF,}, 
{0xE0C6, 0xFFFF,}, 
{0xE0C8, 0xC344,}, 
{0xE0CA, 0x77CF,}, 
{0xE0CC, 0xFFFF,}, 
{0xE0CE, 0xC7C0,}, 
{0xE0D0, 0xB104,}, 
{0xE0D2, 0x8F1F,}, 
{0xE0D4, 0x75CF,}, 
{0xE0D6, 0xFFFF,}, 
{0xE0D8, 0xC84C,}, 
{0xE0DA, 0x0811,}, 
{0xE0DC, 0x005E,}, 
{0xE0DE, 0x70CF,}, 
{0xE0E0, 0x0000,}, 
{0xE0E2, 0x500E,}, 
{0xE0E4, 0x7840,}, 
{0xE0E6, 0xF019,}, 
{0xE0E8, 0x0CC6,}, 
{0xE0EA, 0x0340,}, 
{0xE0EC, 0x0E26,}, 
{0xE0EE, 0x0340,}, 
{0xE0F0, 0x95C2,}, 
{0xE0F2, 0x0E21,}, 
{0xE0F4, 0x101E,}, 
{0xE0F6, 0x0E0D,}, 
{0xE0F8, 0x119E,}, 
{0xE0FA, 0x0D56,}, 
{0xE0FC, 0x0340,}, 
{0xE0FE, 0xF008,}, 
{0xE100, 0x2650,}, 
{0xE102, 0x1040,}, 
{0xE104, 0x0AA2,}, 
{0xE106, 0x0360,}, 
{0xE108, 0xB502,}, 
{0xE10A, 0xB5C2,}, 
{0xE10C, 0x0B22,}, 
{0xE10E, 0x0400,}, 
{0xE110, 0x0CCE,}, 
{0xE112, 0x0320,}, 
{0xE114, 0xD800,}, 
{0xE116, 0x70CF,}, 
{0xE118, 0xFFFF,}, 
{0xE11A, 0xC5D4,}, 
{0xE11C, 0x902C,}, 
{0xE11E, 0x72CF,}, 
{0xE120, 0xFFFF,}, 
{0xE122, 0xE218,}, 
{0xE124, 0x9009,}, 
{0xE126, 0xE105,}, 
{0xE128, 0x73CF,}, 
{0xE12A, 0xFF00,}, 
{0xE12C, 0x2FD0,}, 
{0xE12E, 0x7822,}, 
{0xE130, 0x7910,}, 
{0xE132, 0xB202,}, 
{0xE134, 0x1382,}, 
{0xE136, 0x0700,}, 
{0xE138, 0x0815,}, 
{0xE13A, 0x03DE,}, 
{0xE13C, 0x1387,}, 
{0xE13E, 0x0700,}, 
{0xE140, 0x2102,}, 
{0xE142, 0x000A,}, 
{0xE144, 0x212F,}, 
{0xE146, 0x0288,}, 
{0xE148, 0x1A04,}, 
{0xE14A, 0x0284,}, 
{0xE14C, 0x13B9,}, 
{0xE14E, 0x0700,}, 
{0xE150, 0xB8C1,}, 
{0xE152, 0x0815,}, 
{0xE154, 0x0052,}, 
{0xE156, 0xDB00,}, 
{0xE158, 0x230F,}, 
{0xE15A, 0x0003,}, 
{0xE15C, 0x2102,}, 
{0xE15E, 0x00C0,}, 
{0xE160, 0x7910,}, 
{0xE162, 0xB202,}, 
{0xE164, 0x9507,}, 
{0xE166, 0x7822,}, 
{0xE168, 0xE080,}, 
{0xE16A, 0xD900,}, 
{0xE16C, 0x20CA,}, 
{0xE16E, 0x004B,}, 
{0xE170, 0xB805,}, 
{0xE172, 0x9533,}, 
{0xE174, 0x7815,}, 
{0xE176, 0x6038,}, 
{0xE178, 0x0FB2,}, 
{0xE17A, 0x0560,}, 
{0xE17C, 0xB861,}, 
{0xE17E, 0xB711,}, 
{0xE180, 0x0775,}, 
{0xE182, 0x0540,}, 
{0xE184, 0xD900,}, 
{0xE186, 0xF00A,}, 
{0xE188, 0x70CF,}, 
{0xE18A, 0xFFFF,}, 
{0xE18C, 0xE210,}, 
{0xE18E, 0x7835,}, 
{0xE190, 0x8041,}, 
{0xE192, 0x8000,}, 
{0xE194, 0xE102,}, 
{0xE196, 0xA040,}, 
{0xE198, 0x09F1,}, 
{0xE19A, 0x8094,}, 
{0xE19C, 0x7FE0,}, 
{0xE19E, 0xD800,}, 
{0xE1A0, 0xC0F1,}, 
{0xE1A2, 0xC5E1,}, 
{0xE1A4, 0x71CF,}, 
{0xE1A6, 0x0000,}, 
{0xE1A8, 0x45E6,}, 
{0xE1AA, 0x7960,}, 
{0xE1AC, 0x7508,}, 
{0xE1AE, 0x70CF,}, 
{0xE1B0, 0xFFFF,}, 
{0xE1B2, 0xC84C,}, 
{0xE1B4, 0x9002,}, 
{0xE1B6, 0x083D,}, 
{0xE1B8, 0x021E,}, 
{0xE1BA, 0x0D39,}, 
{0xE1BC, 0x10D1,}, 
{0xE1BE, 0x70CF,}, 
{0xE1C0, 0xFF00,}, 
{0xE1C2, 0x3354,}, 
{0xE1C4, 0x9055,}, 
{0xE1C6, 0x71CF,}, 
{0xE1C8, 0xFFFF,}, 
{0xE1CA, 0xC5D4,}, 
{0xE1CC, 0x116C,}, 
{0xE1CE, 0x0103,}, 
{0xE1D0, 0x1170,}, 
{0xE1D2, 0x00C1,}, 
{0xE1D4, 0xE381,}, 
{0xE1D6, 0x22C6,}, 
{0xE1D8, 0x0F81,}, 
{0xE1DA, 0x0000,}, 
{0xE1DC, 0x00FF,}, 
{0xE1DE, 0x22C4,}, 
{0xE1E0, 0x0F82,}, 
{0xE1E2, 0xFFFF,}, 
{0xE1E4, 0x00FF,}, 
{0xE1E6, 0x29C0,}, 
{0xE1E8, 0x0222,}, 
{0xE1EA, 0x7945,}, 
{0xE1EC, 0x7930,}, 
{0xE1EE, 0xB035,}, 
{0xE1F0, 0x0715,}, 
{0xE1F2, 0x0540,}, 
{0xE1F4, 0xD900,}, 
{0xE1F6, 0xF00A,}, 
{0xE1F8, 0x70CF,}, 
{0xE1FA, 0xFFFF,}, 
{0xE1FC, 0xE224,}, 
{0xE1FE, 0x7835,}, 
{0xE200, 0x8041,}, 
{0xE202, 0x8000,}, 
{0xE204, 0xE102,}, 
{0xE206, 0xA040,}, 
{0xE208, 0x09F1,}, 
{0xE20A, 0x8094,}, 
{0xE20C, 0x7FE0,}, 
{0xE20E, 0xD800,}, 
{0xE210, 0xFFFF,}, 
{0xE212, 0xCB40,}, 
{0xE214, 0xFFFF,}, 
{0xE216, 0xE0BC,}, 
{0xE218, 0x0000,}, 
{0xE21A, 0x0000,}, 
{0xE21C, 0x0000,}, 
{0xE21E, 0x0000,}, 
{0xE220, 0x0000,}, 
{0x098E, 0x0000,}, 
{0xE000, 0x1184,}, 
{0xE002, 0x1204,}, 
//{0xE0040004, 0x41030202,}, 
{0xE004, 0x4103,}, 
{0xE006, 0x0202,}, 
{0x0080, 0xFFF0,},  

//  POLL  COMMAND_REGISTER::HOST_COMMAND_0 =>  0x00
// 读Reg= 0x0080， 判断其最低位是否为0， 如果不为0，则delay 5ms，然后继续读， 
// 直到为0或者   50ms以上
{MT9M114_BYD_COMMAND_REGISTER, MT9M114_BYD_COMMAND_REGISTER_SET_STATE, MSM_CAMERA_I2C_UNSET_WORD_MASK, MSM_CAMERA_I2C_CMD_POLL},
//{ DELAY,  { 50 } },
{ 0x0080, 0xFFF1,},	 	// COMMAND_REGISTER

//  POLL  COMMAND_REGISTER::HOST_COMMAND_0 =>  0x00
// 读Reg= 0x0080， 判断其最低位是否为0， 如果不为0，则delay 5ms，然后继续读， 
// 直到为0或者   50ms以上

{MT9M114_BYD_COMMAND_REGISTER, MT9M114_BYD_COMMAND_REGISTER_SET_STATE, MSM_CAMERA_I2C_UNSET_WORD_MASK, MSM_CAMERA_I2C_CMD_POLL},

//{ DELAY,  { 50 } },
{ 0xA804, 0x00BF,},	

	 

{0x098E, 0x495E,},
{0xC95E, 0x0000,},


//[APGA Settings R 90% 2012/01/26 22:37:02]
{0x3640, 0x0310,},  //  P_G1_P0Q0
{0x3642, 0x4F88,},  //  P_G1_P0Q1
{0x3644, 0x1D30,},  //  P_G1_P0Q2
{0x3646, 0x67AA,},  //  P_G1_P0Q3
{0x3648, 0x4A30,},  //  P_G1_P0Q4
{0x364A, 0x00B0,},  //  P_R_P0Q0
{0x364C, 0x3D28,},  //  P_R_P0Q1
{0x364E, 0x34B0,},  //  P_R_P0Q2
{0x3650, 0x632A,},  //  P_R_P0Q3
{0x3652, 0x57D0,},  //  P_R_P0Q4
{0x3654, 0x0110,},  //  P_B_P0Q0
{0x3656, 0x704B,},  //  P_B_P0Q1
{0x3658, 0x5EEF,},  //  P_B_P0Q2
{0x365A, 0x1FCC,},  //  P_B_P0Q3
{0x365C, 0x1C90,},  //  P_B_P0Q4
{0x365E, 0x01F0,},  //  P_G2_P0Q0
{0x3660, 0x992A,},  //  P_G2_P0Q1
{0x3662, 0x2D50,},  //  P_G2_P0Q2
{0x3664, 0x9B28,},  //  P_G2_P0Q3
{0x3666, 0x3D50,},  //  P_G2_P0Q4
{0x3680, 0xB226,},  //  P_G1_P1Q0
{0x3682, 0xD069,},  //  P_G1_P1Q1
{0x3684, 0x012E,},  //  P_G1_P1Q2
{0x3686, 0x508D,},  //  P_G1_P1Q3
{0x3688, 0x184D,},  //  P_G1_P1Q4
{0x368A, 0xDEC9,},  //  P_R_P1Q0
{0x368C, 0x06CB,},  //  P_R_P1Q1
{0x368E, 0x240D,},  //  P_R_P1Q2
{0x3690, 0x4E46,},  //  P_R_P1Q3
{0x3692, 0xF7AA,},  //  P_R_P1Q4
{0x3694, 0x63E9,},  //  P_B_P1Q0
{0x3696, 0xCDEA,},  //  P_B_P1Q1
{0x3698, 0x32AD,},  //  P_B_P1Q2
{0x369A, 0xCC6D,},  //  P_B_P1Q3
{0x369C, 0xF24F,},  //  P_B_P1Q4
{0x369E, 0x538B,},  //  P_G2_P1Q0
{0x36A0, 0x1FEC,},  //  P_G2_P1Q1
{0x36A2, 0x8B0D,},  //  P_G2_P1Q2
{0x36A4, 0x7423,},  //  P_G2_P1Q3
{0x36A6, 0x4B4C,},  //  P_G2_P1Q4
{0x36C0, 0x4610,},  //  P_G1_P2Q0
{0x36C2, 0xB26D,},  //  P_G1_P2Q1
{0x36C4, 0x71F1,},  //  P_G1_P2Q2
{0x36C6, 0x6A90,},  //  P_G1_P2Q3
{0x36C8, 0xBBF0,},  //  P_G1_P2Q4
{0x36CA, 0x3A70,},  //  P_R_P2Q0
{0x36CC, 0xDD83,},  //  P_R_P2Q1
{0x36CE, 0x2752,},  //  P_R_P2Q2
{0x36D0, 0x5F90,},  //  P_R_P2Q3
{0x36D2, 0x96CF,},  //  P_R_P2Q4
{0x36D4, 0x0410,},  //  P_B_P2Q0
{0x36D6, 0x80ED,},  //  P_B_P2Q1
{0x36D8, 0x3251,},  //  P_B_P2Q2
{0x36DA, 0x0C91,},  //  P_B_P2Q3
{0x36DC, 0x33B1,},  //  P_B_P2Q4
{0x36DE, 0x42B0,},  //  P_G2_P2Q0
{0x36E0, 0xA46C,},  //  P_G2_P2Q1
{0x36E2, 0x0412,},  //  P_G2_P2Q2
{0x36E4, 0x17B0,},  //  P_G2_P2Q3
{0x36E6, 0xDE91,},  //  P_G2_P2Q4
{0x3700, 0xA3AD,},  //  P_G1_P3Q0
{0x3702, 0x8DED,},  //  P_G1_P3Q1
{0x3704, 0x96EE,},  //  P_G1_P3Q2
{0x3706, 0x5ECB,},  //  P_G1_P3Q3
{0x3708, 0xB930,},  //  P_G1_P3Q4
{0x370A, 0x6EAA,},  //  P_R_P3Q0
{0x370C, 0x9CCD,},  //  P_R_P3Q1
{0x370E, 0x5430,},  //  P_R_P3Q2
{0x3710, 0xCACB,},  //  P_R_P3Q3
{0x3712, 0xBCB2,},  //  P_R_P3Q4
{0x3714, 0x000E,},  //  P_B_P3Q0
{0x3716, 0xD0EB,},  //  P_B_P3Q1
{0x3718, 0x2BEE,},  //  P_B_P3Q2
{0x371A, 0x244F,},  //  P_B_P3Q3
{0x371C, 0xF210,},  //  P_B_P3Q4
{0x371E, 0xBA0B,},  //  P_G2_P3Q0
{0x3720, 0xB34E,},  //  P_G2_P3Q1
{0x3722, 0xBBF0,},  //  P_G2_P3Q2
{0x3724, 0x438F,},  //  P_G2_P3Q3
{0x3726, 0x19D1,},  //  P_G2_P3Q4
{0x3740, 0x19CF,},  //  P_G1_P4Q0
{0x3742, 0x1E30,},  //  P_G1_P4Q1
{0x3744, 0x2193,},  //  P_G1_P4Q2
{0x3746, 0xC0F3,},  //  P_G1_P4Q3
{0x3748, 0x8C36,},  //  P_G1_P4Q4
{0x374A, 0x6370,},  //  P_R_P4Q0
{0x374C, 0x0630,},  //  P_R_P4Q1
{0x374E, 0x6ED3,},  //  P_R_P4Q2
{0x3750, 0xCFB3,},  //  P_R_P4Q3
{0x3752, 0xD5B6,},  //  P_R_P4Q4
{0x3754, 0x1DD0,},  //  P_B_P4Q0
{0x3756, 0x7310,},  //  P_B_P4Q1
{0x3758, 0x5CB3,},  //  P_B_P4Q2
{0x375A, 0xF973,},  //  P_B_P4Q3
{0x375C, 0x9E16,},  //  P_B_P4Q4
{0x375E, 0x14EF,},  //  P_G2_P4Q0
{0x3760, 0x458F,},  //  P_G2_P4Q1
{0x3762, 0x7BF2,},  //  P_G2_P4Q2
{0x3764, 0x8993,},  //  P_G2_P4Q3
{0x3766, 0xF515,},  //  P_G2_P4Q4
{0x3784, 0x02A0,},  //  CENTER_COLUMN
{0x3782, 0x01EC,},  //  CENTER_ROW
{0x37C0, 0xB08B,},  //  P_GR_Q5
{0x37C2, 0xC929,},  //  P_RD_Q5
{0x37C4, 0xDC6A,},  //  P_BL_Q5
{0x37C6, 0xD12B,},  //  P_GB_Q5


{0x098E, 0x0000,},  //  LOGICAL addressing
{0xC960, 0x0B22,},  //  CAM_PGA_L_CONFIG_COLOUR_TEMP
{0xC962, 0x776C,},  //  CAM_PGA_L_CONFIG_GREEN_RED_Q14
{0xC964, 0x50A8,},  //  CAM_PGA_L_CONFIG_RED_Q14
{0xC966, 0x76C4,},  //  CAM_PGA_L_CONFIG_GREEN_BLUE_Q14
{0xC968, 0x74AC,},  //  CAM_PGA_L_CONFIG_BLUE_Q14
{0xC96A, 0x0F3C,},  //  CAM_PGA_M_CONFIG_COLOUR_TEMP
{0xC96C, 0x7D3E,},  //  CAM_PGA_M_CONFIG_GREEN_RED_Q14
{0xC96E, 0x7F37,},  //  CAM_PGA_M_CONFIG_RED_Q14
{0xC970, 0x7CBC,},  //  CAM_PGA_M_CONFIG_GREEN_BLUE_Q14
{0xC972, 0x7E47,},  //  CAM_PGA_M_CONFIG_BLUE_Q14
{0xC974, 0x1964,},  //  CAM_PGA_R_CONFIG_COLOUR_TEMP
{0xC976, 0x7CDD,},  //  CAM_PGA_R_CONFIG_GREEN_RED_Q14
{0xC978, 0x6D00,},  //  CAM_PGA_R_CONFIG_RED_Q14
{0xC97A, 0x7EF9,},  //  CAM_PGA_R_CONFIG_GREEN_BLUE_Q14
{0xC97C, 0x7964,},  //  CAM_PGA_R_CONFIG_BLUE_Q14
{0xC95E, 0x0003,},  //  CAM_PGA_PGA_CONTROL

	  /*[Tuning_settings]*/                                                                                        
 //[Step5-AWB_CCM]								                                                                               
 //[Color Correction Matrices 06/04/11 15:35:24]                                                                 
{0x098E, 0x0000,},     // LOGICAL_ADDRESS_ACCESS
{0xC892, 0x01D6,},     // CAM_AWB_CCM_L_0
{0xC894, 0xFF04,},     // CAM_AWB_CCM_L_1
{0xC896, 0x0026,},     // CAM_AWB_CCM_L_2
{0xC898, 0xFFAE,},     // CAM_AWB_CCM_L_3
{0xC89A, 0x012F,},     // CAM_AWB_CCM_L_4
{0xC89C, 0x0022,},     // CAM_AWB_CCM_L_5
{0xC89E, 0xFF8E,},     // CAM_AWB_CCM_L_6
{0xC8A0, 0xFF02,},     // CAM_AWB_CCM_L_7
{0xC8A2, 0x0272,},     // CAM_AWB_CCM_L_8
 //{0xC8C8, 0x0067,},  // CAM_AWB_CCM_L_RG_GAIN                                                                  
 //{0xC8CA, 0x0117,},  // CAM_AWB_CCM_L_BG_GAIN                                                                  
 {0xC8A4, 0x01DA,},  // CAM_AWB_CCM_M_0                                                                          
 {0xC8A6, 0xFF07,},  // CAM_AWB_CCM_M_1                                                                          
 {0xC8A8, 0x001F,},  // CAM_AWB_CCM_M_2                                                                          
 {0xC8AA, 0xFFC3,},  // CAM_AWB_CCM_M_3                                                                          
 {0xC8AC, 0x0156,},  // CAM_AWB_CCM_M_4                                                                          
 {0xC8AE, 0xFFE8,},  // CAM_AWB_CCM_M_5                                                                          
 {0xC8B0, 0xFFCD,},  // CAM_AWB_CCM_M_6                                                                          
 {0xC8B2, 0xFF3A,},  // CAM_AWB_CCM_M_7                                                                          
 {0xC8B4, 0x01FA,},  // CAM_AWB_CCM_M_8                                                                          
 //{0xC8CC, 0x0093,},  // CAM_AWB_CCM_M_RG_GAIN                                                                  
 //{0xC8CE, 0x00FE,},  // CAM_AWB_CCM_M_BG_GAIN                                                                  
 {0xC8B6, 0x01D3,},  // CAM_AWB_CCM_R_0                                                                          
 {0xC8B8, 0xFF73,},  // CAM_AWB_CCM_R_1                                                                          
 {0xC8BA, 0xFFB9,},  // CAM_AWB_CCM_R_2                                                                          
 {0xC8BC, 0xFFA4,},  // CAM_AWB_CCM_R_3                                                                          
 {0xC8BE, 0x0170,},  // CAM_AWB_CCM_R_4                                                                          
 {0xC8C0, 0xFFEB,},  // CAM_AWB_CCM_R_5                                                                          
 {0xC8C2, 0xFFEF,},  // CAM_AWB_CCM_R_6                                                                          
 {0xC8C4, 0xFF33,},  // CAM_AWB_CCM_R_7                                                                          
 {0xC8C6, 0x01DE,},  // CAM_AWB_CCM_R_8                                                                          
 {0xC8C8, 0x0067,},  // CAM_AWB_CCM_L_RG_GAIN                                                                
 {0xC8CA, 0x0117,},  // CAM_AWB_CCM_L_BG_GAIN                                                                
 {0xC8CC, 0x0093,},  // CAM_AWB_CCM_M_RG_GAIN                                                                    
 {0xC8CE, 0x00FE,},  // CAM_AWB_CCM_M_BG_GAIN                                                                    
 {0xC8D0, 0x00A6,},  // CAM_AWB_CCM_R_RG_GAIN                                                                    
 {0xC8D2, 0x008C,},  // CAM_AWB_CCM_R_BG_GAIN                                                                    
 {0xC8D4, 0x09C4,},  // CAM_AWB_CCM_L_CTEMP                                                                      
 {0xC8D6, 0x0D67,},  // CAM_AWB_CCM_M_CTEMP                                                                      
 {0xC8D8, 0x1964,},  // CAM_AWB_CCM_R_CTEMP                                                                      
                                                                                                                 
// {0xC8F2, 0x0003,},  // CAM_AWB_AWB_XSCALE                                                                     
// {0xC8F3, 0x0002,},  // CAM_AWB_AWB_YSCALE                                                                     
 {0xC8F2, 0x03 ,MSM_CAMERA_I2C_BYTE_DATA},  // CAM_AWB_AWB_XSCALE                                                
 {0xC8F3, 0x02 ,MSM_CAMERA_I2C_BYTE_DATA},  // CAM_AWB_AWB_YSCALE                              
                                                                                                                 
 {0xC8F4, 0x7E60,},  // CAM_AWB_AWB_WEIGHTS_0                                                                    
 {0xC8F6, 0xAC56,},  // CAM_AWB_AWB_WEIGHTS_1                                                                    
 {0xC8F8, 0x4EC9,},  // CAM_AWB_AWB_WEIGHTS_2                                                                    
 {0xC8FA, 0xF378,},  // CAM_AWB_AWB_WEIGHTS_3                                                                    
 {0xC8FC, 0xC7D8,},  // CAM_AWB_AWB_WEIGHTS_4                                                                    
 {0xC8FE, 0x92D8,},  // CAM_AWB_AWB_WEIGHTS_5                                                                    
 {0xC900, 0x25D9,},  // CAM_AWB_AWB_WEIGHTS_6                                                                    
 {0xC902, 0xB292,},  // CAM_AWB_AWB_WEIGHTS_7                                                                    
 {0xC904, 0x003C,},  // CAM_AWB_AWB_XSHIFT_PRE_ADJ                                                               
 {0xC906, 0x0037,},  // CAM_AWB_AWB_YSHIFT_PRE_ADJ                                                               
                                                                                                                 
                                                                                                                 
 {0xC90C, 0x83	,MSM_CAMERA_I2C_BYTE_DATA}, 	  // CAM_AWB_K_R_L                                                 
 {0xC90D, 0x80	,MSM_CAMERA_I2C_BYTE_DATA}, 	  // CAM_AWB_K_G_L                                                 
 {0xC90E, 0x81	,MSM_CAMERA_I2C_BYTE_DATA}, 	  // CAM_AWB_K_B_L                                                 
 {0xC90F, 0x80	,MSM_CAMERA_I2C_BYTE_DATA}, 	  // 0x88 CAM_AWB_K_R_R                                            
 {0xC910, 0x80	,MSM_CAMERA_I2C_BYTE_DATA}, 	  // CAM_AWB_K_G_R                                                 
 {0xC911, 0x7B	,MSM_CAMERA_I2C_BYTE_DATA}, 	  // CAM_AWB_K_B_R                                                 
                                                                                                                 
 {0xAC06, 0x64	,MSM_CAMERA_I2C_BYTE_DATA}, 	  // CAM_AWB_K_G_R                                                 
 {0xAC08, 0x64	,MSM_CAMERA_I2C_BYTE_DATA}, 	  // CAM_AWB_K_B_R                                                 
                                                                                                                 
 {0x098E, 0x4926,}, 		   // LOGICAL_ADDRESS_ACCESS [CAM_LL_START_BRIGHTNESS]                                   
 {0xC926, 0x0020,}, 		  // CAM_LL_START_BRIGHTNESS                                                             
 {0xC928, 0x009A,}, 		  // CAM_LL_STOP_BRIGHTNESS                                                              
 {0xC946, 0x0070,}, 		  // CAM_LL_START_GAIN_METRIC                                                            
 {0xC948, 0x00f3,}, 		  // CAM_LL_STOP_GAIN_METRIC                                                             
 {0xC952, 0x0020,}, 		  // CAM_LL_START_TARGET_LUMA_BM                                                         
 {0xC954, 0x009A,}, 		  // CAM_LL_STOP_TARGET_LUMA_BM                                                          
{0xC92A, 0x9C, MSM_CAMERA_I2C_BYTE_DATA},       // 0xc0 CAM_LL_START_SATURATION
 {0xC92B, 0x70,MSM_CAMERA_I2C_BYTE_DATA},		 // CAM_LL_END_SATURATION                                            
 {0xC92C, 0x00,MSM_CAMERA_I2C_BYTE_DATA},		  // CAM_LL_START_DESATURATION                                       
 {0xC92D, 0xFF,MSM_CAMERA_I2C_BYTE_DATA},		  // CAM_LL_END_DESATURATION                                         
 {0xC92E, 0x3C	,MSM_CAMERA_I2C_BYTE_DATA}, 	  // CAM_LL_START_DEMOSAIC                                         
 {0xC92F, 0x01	,MSM_CAMERA_I2C_BYTE_DATA}, 	  // CAM_LL_START_AP_GAIN                                          
 {0xC930, 0x06	,MSM_CAMERA_I2C_BYTE_DATA}, 	  // CAM_LL_START_AP_THRESH                                        
 {0xC931, 0x64	,MSM_CAMERA_I2C_BYTE_DATA}, 	  // CAM_LL_STOP_DEMOSAIC                                          
 {0xC932, 0x00	,MSM_CAMERA_I2C_BYTE_DATA}, 	  // CAM_LL_STOP_AP_GAIN                                           
 {0xC933, 0x0C	,MSM_CAMERA_I2C_BYTE_DATA}, 	  // CAM_LL_STOP_AP_THRESH                                         
 {0xC934, 0x3C	,MSM_CAMERA_I2C_BYTE_DATA}, 	  // CAM_LL_START_NR_RED                                           
 {0xC935, 0x3C	,MSM_CAMERA_I2C_BYTE_DATA}, 	  // CAM_LL_START_NR_GREEN                                         
 {0xC936, 0x3C	,MSM_CAMERA_I2C_BYTE_DATA}, 	  // CAM_LL_START_NR_BLUE                                          
 {0xC937, 0x0F	,MSM_CAMERA_I2C_BYTE_DATA}, 	  // CAM_LL_START_NR_THRESH                                        
 {0xC938, 0x64	,MSM_CAMERA_I2C_BYTE_DATA}, 	  // CAM_LL_STOP_NR_RED                                            
 {0xC939, 0x64	,MSM_CAMERA_I2C_BYTE_DATA}, 	  // CAM_LL_STOP_NR_GREEN                                          
 {0xC93A, 0x64	,MSM_CAMERA_I2C_BYTE_DATA}, 	 // CAM_LL_STOP_NR_BLUE                                            
 {0xC93B, 0x32	,MSM_CAMERA_I2C_BYTE_DATA}, 	  // CAM_LL_STOP_NR_THRESH                                         
 {0xC93C, 0x0020,}, 		  // CAM_LL_START_CONTRAST_BM                                                            
 {0xC93E, 0x009A,}, 		  // CAM_LL_STOP_CONTRAST_BM                                                             
 //{0xC940, 0x0103,}, 		 // CAM_LL_GAMMA                                                                       
                                                                                                                 
 {0xC940, 0x00FF,}, 		 // CAM_LL_GAMMA                                                                         
                                                                                                                 
 //{0xBC07,0x02, MSM_CAMERA_I2C_BYTE_DATA},	                                                         
                                                                                                                 
 {0xC942, 0x3C	,MSM_CAMERA_I2C_BYTE_DATA}, 	  // CAM_LL_START_CONTRAST_GRADIENT                                
 {0xC943, 0x30	,MSM_CAMERA_I2C_BYTE_DATA}, 	  // CAM_LL_STOP_CONTRAST_GRADIENT                                 
 {0xC944, 0x4C	,MSM_CAMERA_I2C_BYTE_DATA}, 	  // CAM_LL_START_CONTRAST_LUMA_PERCENTAGE                         
 {0xC945, 0x19	,MSM_CAMERA_I2C_BYTE_DATA}, 	  // CAM_LL_STOP_CONTRAST_LUMA_PERCENTAGE                          
                                                                                                                 
//{0xC94A, 0x0230,}, 		  // CAM_LL_START_FADE_TO_BLACK_LUMA                                                   
 {0xC94A, 0x0000,}, 		  // CAM_LL_START_FADE_TO_BLACK_LUMA		                                                 
 {0xBC02, 0x0007,},                                                                                              
                                                                                                                 
 {0xC94C, 0x0010,}, 		  // CAM_LL_STOP_FADE_TO_BLACK_LUMA                                                      
 {0xC94E, 0x01CD,}, 		 // CAM_LL_CLUSTER_DC_TH_BM                                                              
 {0xC950, 0x05, MSM_CAMERA_I2C_BYTE_DATA}, 	  // CAM_LL_CLUSTER_DC_GATE_PERCENTAGE                             
 {0xC951, 0x40, MSM_CAMERA_I2C_BYTE_DATA}, 	  // CAM_LL_SUMMING_SENSITIVITY_FACTOR                             
 {0xC87B, 0x1B, MSM_CAMERA_I2C_BYTE_DATA}, 	 // CAM_AET_TARGET_AVERAGE_LUMA_DARK                               
 {0xC878, 0x0C, MSM_CAMERA_I2C_BYTE_DATA}, 	  // CAM_AET_AEMODE                                                
 {0xC890, 0x0040,}, 		  // CAM_AET_TARGET_GAIN                                                                 
 {0xC886, 0x0100,}, 		  // CAM_SENSOR_CFG_MAX_ANALOG_GAIN                                                      
 {0xC87C, 0x005A,}, 		  // CAM_AET_BLACK_CLIPPING_TARGET                                                       

 {0xB42A, 0x05	,MSM_CAMERA_I2C_BYTE_DATA}, 	  // CCM_DELTA_GAIN
 {0xA80A, 0x20	,MSM_CAMERA_I2C_BYTE_DATA}, 	  // AE_TRACK_AE_TRACKING_DAMPENING_SPEED
 
//[Step8-Features]

{0xC984, 0x8041,}, 	// CAM_PORT_OUTPUT_CONTROL         
{0xC988, 0x0F00,}, 	// CAM_PORT_MIPI_TIMING_T_HS_ZERO  
{0xC98A, 0x0B07,}, 	// CAM_PORT_MIPI_TIMING_T_HS_EXIT_H
{0xC98C, 0x0D01,}, 	// CAM_PORT_MIPI_TIMING_T_CLK_POST_
{0xC98E, 0x071D,}, 	// CAM_PORT_MIPI_TIMING_T_CLK_TRAIL
{0xC990, 0x0006,}, 	// CAM_PORT_MIPI_TIMING_T_LPX      
{0xC992, 0x0A0C,}, 	// CAM_PORT_MIPI_TIMING_INIT_TIMING			 
{0x098E, 0xC88B,},  // CCM_DELTA_GAIN
{0xC88B, 0x32 ,MSM_CAMERA_I2C_BYTE_DATA},		  // AE_TRACK_AE_TRACKING_DAMPENING_SPEED
			
			 //for gamma
    {0xC87A, 0x49 ,MSM_CAMERA_I2C_BYTE_DATA},	 // CAM_AET_TARGET_AVERAGE_LUMA
    {0x098E, 0xB00C ,}, // LOGICAL_ADDRESS_ACCESS [BLACKLEVEL_MAX_BLACK_LEVEL]
   // {0xB00C, 0x1B ,MSM_CAMERA_I2C_BYTE_DATA},	 // BLACKLEVEL_MAX_BLACK_LEVEL
    {0xB00C, 0x1B ,MSM_CAMERA_I2C_BYTE_DATA},	// BLACKLEVEL_MAX_BLACK_LEVEL
    {0xB004, 0x0004 ,}, // BLACKLEVEL_ALGO
			 
	// Disable the FAED_To_Black
     {0xBC02 , 0x0007,},



};


static struct v4l2_subdev_info mt9m114_byd_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_YUYV8_2X8,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_reg_conf mt9m114_byd_config_change_settings[] = {
	{0xdc00, 0x28, MSM_CAMERA_I2C_BYTE_DATA, MSM_CAMERA_I2C_CMD_WRITE},
	{MT9M114_BYD_COMMAND_REGISTER, MT9M114_BYD_COMMAND_REGISTER_SET_STATE, 
		MSM_CAMERA_I2C_UNSET_WORD_MASK, MSM_CAMERA_I2C_CMD_POLL},

	{MT9M114_BYD_COMMAND_REGISTER, (MT9M114_BYD_COMMAND_REGISTER_OK |
		MT9M114_BYD_COMMAND_REGISTER_SET_STATE), MSM_CAMERA_I2C_WORD_DATA,
		MSM_CAMERA_I2C_CMD_WRITE},
	{MT9M114_BYD_COMMAND_REGISTER, MT9M114_BYD_COMMAND_REGISTER_SET_STATE,
		MSM_CAMERA_I2C_UNSET_WORD_MASK, MSM_CAMERA_I2C_CMD_POLL},

	{0xDC01, 0x31, MSM_CAMERA_I2C_BYTE_DATA},
};

static void mt9m114_byd_stop_stream(struct msm_sensor_ctrl_t *s_ctrl) {}

static struct msm_camera_i2c_conf_array mt9m114_byd_init_conf[] = {
	{mt9m114_byd_recommend_settings,
	ARRAY_SIZE(mt9m114_byd_recommend_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
	{mt9m114_byd_config_change_settings,
	ARRAY_SIZE(mt9m114_byd_config_change_settings),
	0, MSM_CAMERA_I2C_WORD_DATA},
};

static struct msm_camera_i2c_conf_array mt9m114_byd_confs[] = {
	{mt9m114_byd_960p_settings,
	ARRAY_SIZE(mt9m114_byd_960p_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
};


static struct msm_camera_i2c_reg_conf mt9m114_byd_saturation[][1] = {
	{{0xCC12, 0x00},},
	{{0xCC12, 0x1A},},
	{{0xCC12, 0x34},},
	{{0xCC12, 0x4E},},
	{{0xCC12, 0x68},},
	{{0xCC12, 0x80},},
	{{0xCC12, 0x9A},},
	{{0xCC12, 0xB4},},
	{{0xCC12, 0xCE},},
	{{0xCC12, 0xE8},},
	{{0xCC12, 0xFF},},
};

static struct msm_camera_i2c_reg_conf mt9m114_byd_refresh[] = {
	{MT9M114_BYD_COMMAND_REGISTER, (MT9M114_BYD_COMMAND_REGISTER_OK |
		MT9M114_BYD_COMMAND_REGISTER_REFRESH), MSM_CAMERA_I2C_WORD_DATA,
		MSM_CAMERA_I2C_CMD_WRITE},
};

static struct msm_camera_i2c_conf_array mt9m114_byd_saturation_confs[][2] = {
	{{mt9m114_byd_saturation[0],
		ARRAY_SIZE(mt9m114_byd_saturation[0]), 0, MSM_CAMERA_I2C_WORD_DATA},
	{mt9m114_byd_refresh,
		ARRAY_SIZE(mt9m114_byd_refresh), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9m114_byd_saturation[1],
		ARRAY_SIZE(mt9m114_byd_saturation[1]), 0, MSM_CAMERA_I2C_WORD_DATA},
	{mt9m114_byd_refresh,
		ARRAY_SIZE(mt9m114_byd_refresh), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9m114_byd_saturation[2],
		ARRAY_SIZE(mt9m114_byd_saturation[2]), 0, MSM_CAMERA_I2C_WORD_DATA},
	{mt9m114_byd_refresh,
		ARRAY_SIZE(mt9m114_byd_refresh), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9m114_byd_saturation[3],
		ARRAY_SIZE(mt9m114_byd_saturation[3]), 0, MSM_CAMERA_I2C_WORD_DATA},
	{mt9m114_byd_refresh,
		ARRAY_SIZE(mt9m114_byd_refresh), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9m114_byd_saturation[4],
		ARRAY_SIZE(mt9m114_byd_saturation[4]), 0, MSM_CAMERA_I2C_WORD_DATA},
	{mt9m114_byd_refresh,
		ARRAY_SIZE(mt9m114_byd_refresh), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9m114_byd_saturation[5],
		ARRAY_SIZE(mt9m114_byd_saturation[5]), 0, MSM_CAMERA_I2C_WORD_DATA},
	{mt9m114_byd_refresh,
		ARRAY_SIZE(mt9m114_byd_refresh), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9m114_byd_saturation[6],
		ARRAY_SIZE(mt9m114_byd_saturation[6]), 0, MSM_CAMERA_I2C_WORD_DATA},
	{mt9m114_byd_refresh,
		ARRAY_SIZE(mt9m114_byd_refresh), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9m114_byd_saturation[7],
		ARRAY_SIZE(mt9m114_byd_saturation[7]), 0, MSM_CAMERA_I2C_WORD_DATA},
	{mt9m114_byd_refresh,
		ARRAY_SIZE(mt9m114_byd_refresh), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9m114_byd_saturation[8],
		ARRAY_SIZE(mt9m114_byd_saturation[8]), 0, MSM_CAMERA_I2C_WORD_DATA},
	{mt9m114_byd_refresh,
		ARRAY_SIZE(mt9m114_byd_refresh), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9m114_byd_saturation[9],
		ARRAY_SIZE(mt9m114_byd_saturation[9]), 0, MSM_CAMERA_I2C_WORD_DATA},
	{mt9m114_byd_refresh,
		ARRAY_SIZE(mt9m114_byd_refresh), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9m114_byd_saturation[10],
		ARRAY_SIZE(mt9m114_byd_saturation[10]),
		0, MSM_CAMERA_I2C_WORD_DATA},
	{mt9m114_byd_refresh,
		ARRAY_SIZE(mt9m114_byd_refresh), 0, MSM_CAMERA_I2C_WORD_DATA},},
};

static int mt9m114_byd_saturation_enum_map[] = {
	MSM_V4L2_SATURATION_L0,
	MSM_V4L2_SATURATION_L1,
	MSM_V4L2_SATURATION_L2,
	MSM_V4L2_SATURATION_L3,
	MSM_V4L2_SATURATION_L4,
	MSM_V4L2_SATURATION_L5,
	MSM_V4L2_SATURATION_L6,
	MSM_V4L2_SATURATION_L7,
	MSM_V4L2_SATURATION_L8,
	MSM_V4L2_SATURATION_L9,
	MSM_V4L2_SATURATION_L10,
};

static struct msm_camera_i2c_enum_conf_array mt9m114_byd_saturation_enum_confs = {
	.conf = &mt9m114_byd_saturation_confs[0][0],
	.conf_enum = mt9m114_byd_saturation_enum_map,
	.num_enum = ARRAY_SIZE(mt9m114_byd_saturation_enum_map),
	.num_index = ARRAY_SIZE(mt9m114_byd_saturation_confs),
	.num_conf = ARRAY_SIZE(mt9m114_byd_saturation_confs[0]),
	.data_type = MSM_CAMERA_I2C_WORD_DATA,
};

struct msm_sensor_v4l2_ctrl_info_t mt9m114_byd_v4l2_ctrl_info[] = {
	{
		.ctrl_id = V4L2_CID_SATURATION,
		.min = MSM_V4L2_SATURATION_L0,
		.max = MSM_V4L2_SATURATION_L10,
		.step = 1,
		.enum_cfg_settings = &mt9m114_byd_saturation_enum_confs,
		.s_v4l2_ctrl = msm_sensor_s_ctrl_by_enum,
	},
};

static struct msm_sensor_output_info_t mt9m114_byd_dimensions[] = {
	{
		.x_output = 0x500,
		.y_output = 0x3C0,
		.line_length_pclk = 0x500,
		.frame_length_lines = 0x3C0,
		.vt_pixel_clk = 48000000,
		.op_pixel_clk = 128000000,
		.binning_factor = 1,
	},
};

static struct msm_camera_csid_vc_cfg mt9m114_byd_cid_cfg[] = {
	{0, CSI_YUV422_8, CSI_DECODE_8BIT},
	{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params mt9m114_byd_csi_params = {
	.csid_params = {
		.lane_assign = 0xe4,
		.lane_cnt = 1,
		.lut_params = {
			.num_cid = 2,
			.vc_cfg = mt9m114_byd_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt = 1,
		.settle_cnt = 0x14,
	},
};

static struct msm_camera_csi2_params *mt9m114_byd_csi_params_array[] = {
	&mt9m114_byd_csi_params,
	&mt9m114_byd_csi_params,
};

static struct msm_sensor_output_reg_addr_t mt9m114_byd_reg_addr = {
	.x_output = 0xC868,
	.y_output = 0xC86A,
	.line_length_pclk = 0xC868,
	.frame_length_lines = 0xC86A,
};

static struct msm_sensor_id_info_t mt9m114_byd_id_info = {
	.sensor_id_reg_addr = 0x0,
	.sensor_id = 0x2481,
};

static const struct i2c_device_id mt9m114_byd_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&mt9m114_byd_s_ctrl},
	{ }
};

static struct i2c_driver mt9m114_byd_i2c_driver = {
	.id_table = mt9m114_byd_i2c_id,
	.probe  = msm_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client mt9m114_byd_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

int32_t mt9m114_byd_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
    int rc; 
	int reset_gpio;

	msm_sensor_expand_power_up(s_ctrl);
	
	
    reset_gpio = get_gpio_num_by_name("SCAM_RST");
	if(reset_gpio < 0)
	{
		printk(KERN_ERR"%s get_gpio_num_by_name fail\n",__func__);
		return reset_gpio;
	}
	rc = gpio_request(reset_gpio,"mt9m114_byd");
	if (rc) 
	{
		gpio_free(reset_gpio);
		rc = gpio_request(reset_gpio,"mt9m114_byd");
		if(rc)
		{
		    printk("%s gpio_request(%d) again fail \n",__func__,reset_gpio);
			return rc;
		}
		printk("%s gpio_request(%d) again success\n",__func__,reset_gpio);
	} 
	gpio_direction_output(reset_gpio, 1);
    mdelay(2);        
    gpio_direction_output(reset_gpio, 0);
    mdelay(5);
	gpio_direction_output(reset_gpio, 1);
    mdelay(60);

	return rc;
}

int32_t mt9m114_byd_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	int reset_gpio;

    reset_gpio = get_gpio_num_by_name("SCAM_RST");
	if(reset_gpio < 0)
	{
		printk(KERN_ERR"%s get_gpio_num_by_name fail\n",__func__);
		return reset_gpio;
	}
    gpio_direction_output(reset_gpio, 0);
    gpio_free(reset_gpio);
    
    msm_sensor_expand_power_down(s_ctrl);
    return 0;
}

int mt9m114_byd_sensor_set_effect (struct msm_sensor_ctrl_t *s_ctrl, char effect)
{
	int rc = 0;
	
	CDBG("%s,  effect=%d\n",__func__,effect);
	switch (effect) {
	case CAMERA_EFFECT_OFF: 
        msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
            0x098E, 0xC874, MSM_CAMERA_I2C_WORD_DATA);

        msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
            0xC874, 0x00, MSM_CAMERA_I2C_BYTE_DATA);

        msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
            0xDC00, 0x28, MSM_CAMERA_I2C_BYTE_DATA);

        msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
            0x0080, 0x8004, MSM_CAMERA_I2C_WORD_DATA);
        
		break;
		
	case CAMERA_EFFECT_MONO: 
        msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
            0x098E, 0xC874, MSM_CAMERA_I2C_WORD_DATA);

        msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
            0xC874, 0x01, MSM_CAMERA_I2C_BYTE_DATA);

        msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
            0xDC00, 0x28, MSM_CAMERA_I2C_BYTE_DATA);

        msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
            0x0080, 0x8004, MSM_CAMERA_I2C_WORD_DATA);
        
		break;

	case CAMERA_EFFECT_NEGATIVE: 
        msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
            0x098E, 0xC874, MSM_CAMERA_I2C_WORD_DATA);

        msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
            0xC874, 0x03, MSM_CAMERA_I2C_BYTE_DATA);

        msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
            0xDC00, 0x28, MSM_CAMERA_I2C_BYTE_DATA);

        msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
            0x0080, 0x8004, MSM_CAMERA_I2C_WORD_DATA);
        
		break;

	case CAMERA_EFFECT_SOLARIZE: 
        msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
            0x098E, 0xC874, MSM_CAMERA_I2C_WORD_DATA);

        msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
            0xC874, 0x04, MSM_CAMERA_I2C_BYTE_DATA);

        msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
            0xDC00, 0x28, MSM_CAMERA_I2C_BYTE_DATA);

        msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
            0x0080, 0x8004, MSM_CAMERA_I2C_WORD_DATA);
        

		break;

	case CAMERA_EFFECT_SEPIA: 

        msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
            0x098E, 0xC874, MSM_CAMERA_I2C_WORD_DATA);

        msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
            0xC874, 0x02, MSM_CAMERA_I2C_BYTE_DATA);

        msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
            0xDC00, 0x28, MSM_CAMERA_I2C_BYTE_DATA);

        msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
            0x0080, 0x8004, MSM_CAMERA_I2C_WORD_DATA);
        

		break;
	default:
        CDBG("%s,  effect=%d not support yet \n",__func__,effect);
        break;
	}
    return rc;
}

int mt9m114_byd_sensor_set_whitebalance (struct msm_sensor_ctrl_t * s_ctrl,char wb)
{
     long rc = 0;
     
     CDBG("%s,  wb=%d\n",__func__,wb);

     switch (wb) {
     case CAMERA_WB_AUTO: {
         //[AWB]
         msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
             0x098E, 0x0000, MSM_CAMERA_I2C_WORD_DATA);
         
         msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
             0xC909, 0x03, MSM_CAMERA_I2C_BYTE_DATA);

     }
         break;
 
     case CAMERA_WB_INCANDESCENT: {
         //[Incandescent] [Alight MWB]
         msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
             0x098E, 0x0000, MSM_CAMERA_I2C_WORD_DATA);
         
         msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
             0xC909, 0x01, MSM_CAMERA_I2C_BYTE_DATA);

         msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
             0xC8F0, 0x0AF0, MSM_CAMERA_I2C_WORD_DATA);

     }
         break;
 
     case CAMERA_WB_CLOUDY_DAYLIGHT: {
         //[Cloudy] U30/TL84 MWB]
         msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
             0x098E, 0x0000, MSM_CAMERA_I2C_WORD_DATA);
         
         msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
             0xC909, 0x01, MSM_CAMERA_I2C_BYTE_DATA);

         msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
             0xC8F0, 0x1B58, MSM_CAMERA_I2C_WORD_DATA);

     }
         break;
 
     case CAMERA_WB_DAYLIGHT: {
         //[DayLight] [D65 MWB]
         msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
             0x098E, 0x0000, MSM_CAMERA_I2C_WORD_DATA);
         
         msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
             0xC909, 0x01, MSM_CAMERA_I2C_BYTE_DATA);

/* update camera daylight awb parameters to diff from cloudy awb effect start*/
         msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
             0xC8F0, 0x1464, MSM_CAMERA_I2C_WORD_DATA);  
/* update camera daylight awb parameters to diff from cloudy awb effect end*/

     }
         break;
         case CAMERA_WB_FLUORESCENT: {
         //[Flourescent] [CWF MWB]
         msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
             0x098E, 0x0000, MSM_CAMERA_I2C_WORD_DATA);
         
         msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
             0xC909, 0x01, MSM_CAMERA_I2C_BYTE_DATA);

/* update camera fluorescent awb parameters start */
         msm_camera_i2c_write( s_ctrl->sensor_i2c_client,
             0xC8F0, 0x0C3D, MSM_CAMERA_I2C_WORD_DATA);  
/* update camera fluorescent awb parameters end */

     }
         break;
 
     case CAMERA_WB_TWILIGHT: {
         CDBG("%s,  wb=%d not support yet \n",__func__,wb);
     }
         break;
 
     case CAMERA_WB_SHADE: {
         CDBG("%s,  wb=%d not support yet \n",__func__,wb);
     }
         break;
         
     default: {
         CDBG("%s,  wb=%d not support yet \n",__func__,wb);
         }
     break;
 }
 
     return rc;
 }

static int __init mt9m114_byd_sunny_init_module(void)
{
	return i2c_add_driver(&mt9m114_byd_i2c_driver);
}

static struct v4l2_subdev_core_ops mt9m114_byd_subdev_core_ops = {
	.s_ctrl = msm_sensor_v4l2_s_ctrl,
	.queryctrl = msm_sensor_v4l2_query_ctrl,
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops mt9m114_byd_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops mt9m114_byd_subdev_ops = {
	.core = &mt9m114_byd_subdev_core_ops,
	.video  = &mt9m114_byd_subdev_video_ops,
};

static struct msm_sensor_fn_t mt9m114_byd_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = mt9m114_byd_stop_stream,
	.sensor_setting = msm_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = mt9m114_byd_sensor_power_up,
	.sensor_power_down = mt9m114_byd_sensor_power_down,
	.sensor_set_whitebalance_yuv = mt9m114_byd_sensor_set_whitebalance,
    .sensor_set_effect_yuv =  mt9m114_byd_sensor_set_effect,
    	.sensor_csi_setting = msm_sensor_setting1,
};

static struct msm_sensor_reg_t mt9m114_byd_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = mt9m114_byd_config_change_settings,
	.start_stream_conf_size = ARRAY_SIZE(mt9m114_byd_config_change_settings),
	.init_settings = &mt9m114_byd_init_conf[0],
	.init_size = ARRAY_SIZE(mt9m114_byd_init_conf),
	.mode_settings = &mt9m114_byd_confs[0],
	.output_settings = &mt9m114_byd_dimensions[0],
	.num_conf = ARRAY_SIZE(mt9m114_byd_confs),
};

static struct msm_sensor_ctrl_t mt9m114_byd_s_ctrl = {
	.msm_sensor_reg = &mt9m114_byd_regs,
	.msm_sensor_v4l2_ctrl_info = mt9m114_byd_v4l2_ctrl_info,
	.num_v4l2_ctrl = ARRAY_SIZE(mt9m114_byd_v4l2_ctrl_info),
	.sensor_i2c_client = &mt9m114_byd_sensor_i2c_client,
	.sensor_i2c_addr = 0x90,
	.sensor_output_reg_addr = &mt9m114_byd_reg_addr,
	.sensor_id_info = &mt9m114_byd_id_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csi_params = &mt9m114_byd_csi_params_array[0],
	.msm_sensor_mutex = &mt9m114_byd_mut,
	.sensor_i2c_driver = &mt9m114_byd_i2c_driver,
	.sensor_v4l2_subdev_info = mt9m114_byd_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(mt9m114_byd_subdev_info),
	.sensor_v4l2_subdev_ops = &mt9m114_byd_subdev_ops,
	.func_tbl = &mt9m114_byd_func_tbl,
};

module_init(mt9m114_byd_sunny_init_module);
MODULE_DESCRIPTION("Aptina 1.26MP YUV sensor driver");
MODULE_LICENSE("GPL v2");
