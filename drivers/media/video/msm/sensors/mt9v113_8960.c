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

#include "msm_camera_i2c.h"
#include "msm_sensor.h"
#include <hsad/config_interface.h>
#define SENSOR_NAME "mt9v113_8960"
#define PLATFORM_DRIVER_NAME "msm_camera_mt9v113_8960"
#define mt9v113_8960_obj mt9v113_8960_##obj


DEFINE_MUTEX(mt9v113_8960_mut);
static struct msm_sensor_ctrl_t mt9v113_8960_s_ctrl;



static struct msm_camera_i2c_reg_conf mt9v113_8960_start_settings[] = {
	{ 0x3400, 0x7A2C },
};

static struct msm_camera_i2c_reg_conf mt9v113_8960_stop_settings[] = {
	{ 0x3400, 0x7A26 },
};

static struct msm_camera_i2c_reg_conf_delay mt9v113_8960_vga_settings[] = {
// move to the recommand setting already, just for hook
};
static struct msm_camera_i2c_reg_conf_delay mt9v113_8960_recommend_settings[] = 

{
    { 0x0018, 0x4028, 0 },
    { 0x0000, 0x0000, 100 },//delay 100
    { 0x001A, 0x0011, 0 },
    { 0x001A, 0x0010, 0 },
    { 0x0018, 0x4028, 0 },
    { 0x0000, 0x0000, 100 },//delay 100
    { 0x098C, 0x02F0, 0 },
    { 0x0990, 0x0000, 0 },
    { 0x098C, 0x02F2, 0 },
    { 0x0990, 0x0210, 0 },
    { 0x098C, 0x02F4, 0 },
    { 0x0990, 0x001A, 0 },
    { 0x098C, 0x2145, 0 },
    { 0x0990, 0x02F4, 0 },
    { 0x098C, 0xA134, 0 },
    { 0x0990, 0x0001, 0 },
    { 0x31E0, 0x0001, 0 },
    { 0x001A, 0x0010, 0 },
    { 0x3400, 0x7A28, 0 },
    { 0x321C, 0x8003, 0 },
    { 0x001E, 0x0777, 0 },
    { 0x0016, 0x42DF, 0 },
    { 0x0014, 0xB04B, 0 },
    { 0x0014, 0xB049, 0 },
    { 0x0010, 0x0631, 0 },
    { 0x0012, 0x0000, 0 },
    { 0x0014, 0x244B, 0 },
    { 0x0014, 0x304B, 0 },
    { 0x0000, 0x0000, 100 },//delay 100
    { 0x0014, 0xB04A, 0 },
    { 0x098C, 0xAB1F, 0 },
    { 0x0990, 0x00C7, 0 },
    { 0x098C, 0xAB31, 0 },
    { 0x0990, 0x001E, 0 },
    { 0x098C, 0x274F, 0 },
    { 0x0990, 0x0004, 0 },
    { 0x098C, 0x2741, 0 },
    { 0x0990, 0x0004, 0 },
    { 0x098C, 0xAB20, 0 },
    { 0x0990, 0x0054, 0 },
    { 0x098C, 0xAB21, 0 },
    { 0x0990, 0x0046, 0 },
    { 0x098C, 0xAB22, 0 },
    { 0x0990, 0x0002, 0 },
    { 0x098C, 0xAB24, 0 },
    { 0x0990, 0x0005, 0 },
    { 0x098C, 0x2B28, 0 },
    { 0x0990, 0x170C, 0 },
    { 0x098C, 0x2B2A, 0 },
    { 0x0990, 0x3E80, 0 },
    { 0x3210, 0x09B0, 0 },
    { 0x364E, 0x0350, 0 },
    { 0x3650, 0x22ED, 0 },
    { 0x3652, 0x0513, 0 },
    { 0x3654, 0x6C70, 0 },
    { 0x3656, 0x5015, 0 },
    { 0x3658, 0x0130, 0 },
    { 0x365A, 0x444D, 0 },
    { 0x365C, 0x18D3, 0 },
    { 0x365E, 0x5FB1, 0 },
    { 0x3660, 0x6415, 0 },
    { 0x3662, 0x00D0, 0 },
    { 0x3664, 0x014C, 0 },
    { 0x3666, 0x7BB2, 0 },
    { 0x3668, 0x31B1, 0 },
    { 0x366A, 0x46D5, 0 },
    { 0x366C, 0x0130, 0 },
    { 0x366E, 0x338D, 0 },
    { 0x3670, 0x0593, 0 },
    { 0x3672, 0x13D1, 0 },
    { 0x3674, 0x4875, 0 },
    { 0x3676, 0x992E, 0 },
    { 0x3678, 0x910E, 0 },
    { 0x367A, 0xAF92, 0 },
    { 0x367C, 0x1732, 0 },
    { 0x367E, 0x7BD3, 0 },
    { 0x3680, 0x98EE, 0 },
    { 0x3682, 0xEF4D, 0 },
    { 0x3684, 0x8872, 0 },
    { 0x3686, 0x0352, 0 },
    { 0x3688, 0x2792, 0 },
    { 0x368A, 0xDB6D, 0 },
    { 0x368C, 0xF52D, 0 },
    { 0x368E, 0xA532, 0 },
    { 0x3690, 0x0213, 0 },
    { 0x3692, 0x10D5, 0 },
    { 0x3694, 0x8BCE, 0 },
    { 0x3696, 0xFC2D, 0 },
    { 0x3698, 0xA532, 0 },
    { 0x369A, 0x67F1, 0 },
    { 0x369C, 0x1034, 0 },
    { 0x369E, 0x1113, 0 },
    { 0x36A0, 0x2EF3, 0 },
    { 0x36A2, 0x39F7, 0 },
    { 0x36A4, 0xB097, 0 },
    { 0x36A6, 0x81BA, 0 },
    { 0x36A8, 0x2CF3, 0 },
    { 0x36AA, 0x1373, 0 },
    { 0x36AC, 0x4457, 0 },
    { 0x36AE, 0xFAF6, 0 },
    { 0x36B0, 0xEC19, 0 },
    { 0x36B2, 0x0E73, 0 },
    { 0x36B4, 0x0873, 0 },
    { 0x36B6, 0x34F7, 0 },
    { 0x36B8, 0x9EB7, 0 },
    { 0x36BA, 0x9B9A, 0 },
    { 0x36BC, 0x0E33, 0 },
    { 0x36BE, 0x2013, 0 },
    { 0x36C0, 0x3C37, 0 },
    { 0x36C2, 0xA0D7, 0 },
    { 0x36C4, 0x935A, 0 },
    { 0x36C6, 0xCD11, 0 },
    { 0x36C8, 0x0353, 0 },
    { 0x36CA, 0x2516, 0 },
    { 0x36CC, 0x8437, 0 },
    { 0x36CE, 0xA01A, 0 },
    { 0x36D0, 0xFCF1, 0 },
    { 0x36D2, 0xAD91, 0 },
    { 0x36D4, 0x29D4, 0 },
    { 0x36D6, 0x0AB6, 0 },
    { 0x36D8, 0x9436, 0 },
    { 0x36DA, 0xA872, 0 },
    { 0x36DC, 0xD00F, 0 },
    { 0x36DE, 0x2B36, 0 },
    { 0x36E0, 0x0714, 0 },
    { 0x36E2, 0xFEB9, 0 },
    { 0x36E4, 0xD211, 0 },
    { 0x36E6, 0x22F1, 0 },
    { 0x36E8, 0x5BD5, 0 },
    { 0x36EA, 0x98D3, 0 },
    { 0x36EC, 0xF4D9, 0 },
    { 0x36EE, 0x78B5, 0 },
    { 0x36F0, 0xAA57, 0 },
    { 0x36F2, 0xF65A, 0 },
    { 0x36F4, 0x52DB, 0 },
    { 0x36F6, 0x4CDE, 0 },
    { 0x36F8, 0x7475, 0 },
    { 0x36FA, 0xD936, 0 },
    { 0x36FC, 0xEDFA, 0 },
    { 0x36FE, 0x7DDA, 0 },
    { 0x3700, 0x46DE, 0 },
    { 0x3702, 0x4775, 0 },
    { 0x3704, 0xE5F6, 0 },
    { 0x3706, 0xF9DA, 0 },
    { 0x3708, 0x281B, 0 },
    { 0x370A, 0x4C1E, 0 },
    { 0x370C, 0x0396, 0 },
    { 0x370E, 0xF016, 0 },
    { 0x3710, 0x85DB, 0 },
    { 0x3712, 0x239B, 0 },
    { 0x3714, 0x6B9E, 0 },
    { 0x3644, 0x0154, 0 },
    { 0x3642, 0x00DC, 0 },
    { 0x3210, 0x09B8, 0 },
    { 0x098C, 0xA24F, 0 },
    { 0x0990, 0x0038, 0 },
    { 0x098C, 0xAB37, 0 },
    { 0x0990, 0x0003, 0 },    
    { 0x098C, 0x2306, 0 },
    { 0x0990, 0x0755, 0 },
    { 0x098C, 0x231C, 0 },
    { 0x0990, 0x001B, 0 },
    { 0x098C, 0x2308, 0 },
    { 0x0990, 0xF9B2, 0 },
    { 0x098C, 0x231E, 0 },
    { 0x0990, 0xFFEA, 0 },
    { 0x098C, 0x230A, 0 },
    { 0x0990, 0x0067, 0 },
    { 0x098C, 0x2320, 0 },
    { 0x0990, 0x000F, 0 },
    { 0x098C, 0x230C, 0 },
    { 0x0990, 0xFDE2, 0 },
    { 0x098C, 0x2322, 0 },
    { 0x0990, 0x00AA, 0 },
    { 0x098C, 0x230E, 0 },
    { 0x0990, 0x05F7, 0 },
    { 0x098C, 0x2324, 0 },
    { 0x0990, 0xFF89, 0 },
    { 0x098C, 0x2310, 0 },
    { 0x0990, 0xFDC7, 0 },
    { 0x098C, 0x2326, 0 },
    { 0x0990, 0xFFDC, 0 },
    { 0x098C, 0x2312, 0 },
    { 0x0990, 0xFDC4, 0 },
    { 0x098C, 0x2328, 0 },
    { 0x0990, 0x018B, 0 },
    { 0x098C, 0x2314, 0 },
    { 0x0990, 0xFACE, 0 },
    { 0x098C, 0x232A, 0 },
    { 0x0990, 0x0271, 0 },
    { 0x098C, 0x2316, 0 },
    { 0x0990, 0x08F8, 0 },
    { 0x098C, 0x232C, 0 },
    { 0x0990, 0xFC10, 0 },
    { 0x098C, 0x2318, 0 },
    { 0x0990, 0x001C, 0 },
    { 0x098C, 0x231A, 0 },
    { 0x0990, 0x0039, 0 },    
    { 0x098C, 0x232E, 0 },
    { 0x0990, 0x0001, 0 },
    { 0x098C, 0x2330, 0 },
    { 0x0990, 0xFFEF, 0 },
    { 0x098C, 0xA366, 0 },    // MCU_ADDRESS [AWB_KR_L]
    { 0x0990, 0x0080, 0 },    // MCU_DATA_0            
    { 0x098C, 0xA367, 0 },    // MCU_ADDRESS [AWB_KG_L]
    { 0x0990, 0x0080, 0 },       // MCU_DATA_0            
    { 0x098C, 0xA368, 0 },    // MCU_ADDRESS [AWB_KB_L]
    { 0x0990, 0x0080, 0 },    // MCU_DATA_0            
    { 0x098C, 0xA369, 0 },    // MCU_ADDRESS [AWB_KR_R]
    { 0x0990, 0x0080, 0 },    // MCU_DATA_0            
    { 0x098C, 0xA36A, 0 },    // MCU_ADDRESS [AWB_KG_R]
    { 0x0990, 0x0080, 0 },       // MCU_DATA_0            
    { 0x098C, 0xA36B, 0 },    // MCU_ADDRESS [AWB_KB_R]
    { 0x0990, 0x0080, 0 },    // MCU_DATA_0            
    { 0x098C, 0xA348, 0 },
    { 0x0990, 0x0008, 0 },
    { 0x098C, 0xA349, 0 },
    { 0x0990, 0x0002, 0 },
    { 0x098C, 0xA34A, 0 },
    { 0x0990, 0x0090, 0 },
    { 0x098C, 0xA34B, 0 },
    { 0x0990, 0x00FF, 0 },
    { 0x098C, 0xA34C, 0 },
    { 0x0990, 0x0075, 0 },
    { 0x098C, 0xA34D, 0 },
    { 0x0990, 0x00EF, 0 },
    { 0x098C, 0xA351, 0 },
    { 0x0990, 0x0000, 0 },
    { 0x098C, 0xA352, 0 },
    { 0x0990, 0x007F, 0 },
    { 0x098C, 0xA354, 0 },
    { 0x0990, 0x0043, 0 },
    { 0x098C, 0xA355, 0 },
    { 0x0990, 0x0001, 0 },
    { 0x098C, 0xA35D, 0 },
    { 0x0990, 0x0078, 0 },
    { 0x098C, 0xA35E, 0 },
    { 0x0990, 0x0086, 0 },
    { 0x098C, 0xA35F, 0 },
    { 0x0990, 0x007E, 0 },
    { 0x098C, 0xA360, 0 },
    { 0x0990, 0x0082, 0 },
    { 0x098C, 0x2361, 0 },
    { 0x0990, 0x0040, 0 },
    { 0x098C, 0xA363, 0 },
    { 0x0990, 0x00D2, 0 },
    { 0x098C, 0xA364, 0 },
    { 0x0990, 0x00F6, 0 },
    { 0x098C, 0xA302, 0 },
    { 0x0990, 0x0000, 0 },
    { 0x098C, 0xA303, 0 },
    { 0x0990, 0x00EF, 0 },
    { 0x098C, 0xAB20, 0 },
    { 0x0990, 0x0024, 0 },
    { 0x098C, 0x222D, 0 },
    { 0x0990, 0x0088, 0 },
    { 0x098C, 0xA408, 0 },
    { 0x0990, 0x0020, 0 },
    { 0x098C, 0xA409, 0 },
    { 0x0990, 0x0023, 0 },
    { 0x098C, 0xA40A, 0 },
    { 0x0990, 0x0027, 0 },
    { 0x098C, 0xA40B, 0 },
    { 0x0990, 0x002A, 0 },
    { 0x098C, 0x2411, 0 },
    { 0x0990, 0x0088, 0 },
    { 0x098C, 0x2413, 0 },
    { 0x0990, 0x00A4, 0 },
    { 0x098C, 0x2415, 0 },
    { 0x0990, 0x0088, 0 },
    { 0x098C, 0x2417, 0 },
    { 0x0990, 0x00A4, 0 },
    { 0x098C, 0xA404, 0 },
    { 0x0990, 0x0010, 0 },
    { 0x098C, 0xA40D, 0 },
    { 0x0990, 0x0002, 0 },
    { 0x098C, 0xA40E, 0 },
    { 0x0990, 0x0003, 0 },
    { 0x098C, 0x2739, 0 },
    { 0x0990, 0x0000, 0 },
    { 0x098C, 0x273B, 0 },
    { 0x0990, 0x027F, 0 },
    { 0x098C, 0x273D, 0 },
    { 0x0990, 0x0000, 0 },
    { 0x098C, 0x273F, 0 },
    { 0x0990, 0x01DF, 0 },
    { 0x098C, 0x2703, 0 },
    { 0x0990, 0x0280, 0 },
    { 0x098C, 0x2705, 0 },
    { 0x0990, 0x01E0, 0 },
    { 0x98C, 0x2717  , 0 },    //Read Mode (A)
    { 0x990, 0x0025  , 0 },    //      = 39
    { 0x98C, 0x272D  , 0 },    //Read Mode (B)
    { 0x990, 0x0025  , 0 },    //      = 39
    { 0x098C, 0xA103, 0 },
    { 0x0990, 0x0006, 0 },
    { 0x0000, 0x0000, 100 },
    { 0x098C, 0xA103, 0 },
    { 0x0990, 0x0005, 0 },
    { 0x0000, 0x0000, 100 },
    { 0x3400, 0x7A28, 0 },
};
static struct v4l2_subdev_info mt9v113_8960_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_YUYV8_2X8,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_conf_array mt9v113_8960_init_conf[] = {
{
    &mt9v113_8960_recommend_settings[0],
	ARRAY_SIZE(mt9v113_8960_recommend_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
};

static struct msm_camera_i2c_conf_array mt9v113_8960_confs[] = {
	{mt9v113_8960_vga_settings,
	ARRAY_SIZE(mt9v113_8960_vga_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
};

static struct msm_sensor_output_info_t mt9v113_8960_dimensions[] = {
	{
		.x_output = 0x280,
		.y_output = 0x1E0,
		.line_length_pclk = 0x034A,//0x034A
		.frame_length_lines = 0x022A,//0x046F
		.vt_pixel_clk = 28000000,//448M
		.op_pixel_clk = 28000000,//28000000,
		.binning_factor = 0,
	},
};

static struct msm_camera_csid_vc_cfg mt9v113_8960_cid_cfg[] = {
	{0, CSI_YUV422_8, CSI_DECODE_8BIT},
	{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params mt9v113_8960_csi_params = {
	.csid_params = {
		.lane_assign = 0xe4,
		.lane_cnt = 1,
		.lut_params = {
			.num_cid = 2,
			.vc_cfg = mt9v113_8960_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt = 1,
		.settle_cnt = 0x14,
	},
};

static struct msm_camera_csi2_params *mt9v113_8960_csi_params_array[] = {
	&mt9v113_8960_csi_params,
	&mt9v113_8960_csi_params,
};


static struct msm_sensor_id_info_t mt9v113_8960_id_info = { //OK
	.sensor_id_reg_addr = 0x0000,
	.sensor_id = 0x2280,//0x2481,ok
};

static const struct i2c_device_id mt9v113_8960_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&mt9v113_8960_s_ctrl},
	{ }
};

static struct i2c_driver mt9v113_8960_i2c_driver = {
	.id_table = mt9v113_8960_i2c_id,
	.probe  = msm_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client mt9v113_8960_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

int32_t mt9v113_8960_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
    int rc; 
	int reset_gpio;
	int pwd_gpio;

	msm_sensor_expand_power_up(s_ctrl);

	reset_gpio = get_gpio_num_by_name("SCAM_RST");
	pwd_gpio = get_gpio_num_by_name("SCAM_SHDN");
	if(reset_gpio < 0)
	{
		printk(KERN_ERR"%s get_gpio_num_by_name fail\n",__func__);
		return reset_gpio;
	}
	if(pwd_gpio < 0)
	{
		printk(KERN_ERR"%s get_gpio_num_by_name fail\n",__func__);
		return pwd_gpio;
	}
	
    mdelay(5);

	rc = gpio_request(pwd_gpio,"mt9v113_8960");
    if (rc) 
    {
        gpio_free(pwd_gpio);
	    rc = gpio_request(pwd_gpio,"mt9v113_8960");
	    if(rc)
	    {
	        printk("%s gpio_request(%d) again fail \n",__func__,pwd_gpio);
	        return rc;
	    }
	    printk("%s gpio_request(%d) again success\n",__func__,pwd_gpio);
    }
    gpio_direction_output(pwd_gpio, 0);
	mdelay(10);
	
    rc = gpio_request(reset_gpio,"mt9v113_8960");
    if (rc) 
    {
        gpio_free(reset_gpio);
	    rc = gpio_request(reset_gpio,"mt9v113_8960");
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
    mdelay(10);

    return rc;
}

int32_t mt9v113_8960_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	int reset_gpio;
	int pwd_gpio;

    msm_sensor_expand_power_down(s_ctrl);

	reset_gpio = get_gpio_num_by_name("SCAM_RST");
	pwd_gpio = get_gpio_num_by_name("SCAM_SHDN");
    if(reset_gpio < 0)
	{
		printk(KERN_ERR"%s get_gpio_num_by_name fail\n",__func__);
		return reset_gpio;
	}
	if(pwd_gpio < 0)
	{
		printk(KERN_ERR"%s get_gpio_num_by_name fail\n",__func__);
		return pwd_gpio;
	}
	
    gpio_direction_output(reset_gpio, 0);
    mdelay(1);
    gpio_direction_output(pwd_gpio, 0);
    mdelay(1);
    gpio_free(reset_gpio);     
    gpio_free(pwd_gpio);

    return 0;
}

 int mt9v113_8960_sensor_set_effect (struct msm_sensor_ctrl_t *s_ctrl, char effect)
{
    return 0;
}


 int mt9v113_8960_sensor_set_whitebalance (struct msm_sensor_ctrl_t * s_ctrl,char wb)
 {
    return 0;
 }

static int __init mt9v113_sunny_init_module(void)
{
    printk("%s,\n",__func__);

	return i2c_add_driver(&mt9v113_8960_i2c_driver);
}

static struct v4l2_subdev_core_ops mt9v113_8960_subdev_core_ops = {
	.s_ctrl = msm_sensor_v4l2_s_ctrl,
	.queryctrl = msm_sensor_v4l2_query_ctrl,
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
}	;
static struct v4l2_subdev_video_ops mt9v113_8960_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops mt9v113_8960_subdev_ops = {
	.core = &mt9v113_8960_subdev_core_ops,
	.video  = &mt9v113_8960_subdev_video_ops,
};

static struct msm_sensor_fn_t mt9v113_8960_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_setting = msm_sensor_setting_delay,//msm_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = mt9v113_8960_sensor_power_up,
	.sensor_power_down = mt9v113_8960_sensor_power_down,
    .sensor_set_whitebalance_yuv = mt9v113_8960_sensor_set_whitebalance,
    .sensor_set_effect_yuv =  mt9v113_8960_sensor_set_effect,
};

static struct msm_sensor_reg_t mt9v113_8960_regs = {
    .default_data_type = MSM_CAMERA_I2C_WORD_DATA,
    .start_stream_conf = mt9v113_8960_start_settings,
    .start_stream_conf_size = ARRAY_SIZE(mt9v113_8960_start_settings),
    .stop_stream_conf = mt9v113_8960_stop_settings,
    .stop_stream_conf_size = ARRAY_SIZE(mt9v113_8960_stop_settings),

    
	.init_settings = &mt9v113_8960_init_conf[0],
	.init_size = ARRAY_SIZE(mt9v113_8960_init_conf),
	.mode_settings = &mt9v113_8960_confs[0],
	.output_settings = &mt9v113_8960_dimensions[0],
	.num_conf = ARRAY_SIZE(mt9v113_8960_confs),
};

static struct msm_sensor_ctrl_t mt9v113_8960_s_ctrl = {
	.msm_sensor_reg = &mt9v113_8960_regs,
	.sensor_i2c_client = &mt9v113_8960_sensor_i2c_client,
	.sensor_i2c_addr = 0x7A,
	//.sensor_output_reg_addr = &mt9v113_8960_reg_addr,
	.sensor_id_info = &mt9v113_8960_id_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csi_params = &mt9v113_8960_csi_params_array[0],
	.msm_sensor_mutex = &mt9v113_8960_mut,
	.sensor_i2c_driver = &mt9v113_8960_i2c_driver,
	.sensor_v4l2_subdev_info = mt9v113_8960_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(mt9v113_8960_subdev_info),
	.sensor_v4l2_subdev_ops = &mt9v113_8960_subdev_ops,
	.func_tbl = &mt9v113_8960_func_tbl,
};

module_init(mt9v113_sunny_init_module);
MODULE_DESCRIPTION("Aptina 0.3M YUV sensor driver");
MODULE_LICENSE("GPL v2");

