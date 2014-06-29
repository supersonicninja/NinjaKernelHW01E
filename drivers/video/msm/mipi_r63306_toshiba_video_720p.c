/* Copyright (c) 2009-2011, Code Aurora Forum. All rights reserved.
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
#include <linux/gpio.h>
#include <hsad/config_interface.h>
#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mdp4.h"
#include <linux/pwm.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/msm_mdp.h>
/*fangchao 00159369 20120223 begin */
#include <linux/lcd_tuning.h>
/*fangchao 00159369 20120223 end */
//#define MIPI_TOSHIBA_720P_PWM_PMIC
#define MIPI_TOSHIBA_720P_PWM_FREQ_HZ 300
#define MIPI_TOSHIBA_720P_PWM_PERIOD_USEC (USEC_PER_SEC / MIPI_TOSHIBA_720P_PWM_FREQ_HZ)
#define MIPI_TOSHIBA_720P_PWM_LEVEL 100
#define MIPI_TOSHIBA_720P_PWM_DUTY_LEVEL \
	(MIPI_TOSHIBA_720P_PWM_PERIOD_USEC / MIPI_TOSHIBA_720P_PWM_LEVEL)
/* modify for 8960 t0 */
#define LCD_BIAS_EN 1
#define SIZE_PER_BACKLIGHT_LEVEL 16
/* optmize lcd self adapt function,delete 3 lines */
/* add chip version r63308 support */
#define CHIP_VER_R63308 0x08
#define CHIP_VER_R63306 0x06
/*add for lcd power down timing*/
#define PM8921_GPIO_BASE		NR_GPIO_IRQS
#define PM8921_GPIO_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_GPIO_BASE)

static struct msm_panel_info pinfo;
static int ch_used[3];
static struct dsi_buf toshiba_tx_buf;
static struct dsi_buf toshiba_rx_buf;
static struct pwm_device *bl_lpm;
static struct mipi_dsi_panel_platform_data *mipi_toshiba_pdata;
static struct msm_fb_data_type *mfd_toshiba;
/*add for lcd power down timing*/
static bool panel_power_on = false;
static struct mutex power_lock;
static int reset_gpio;
static struct regulator *reg_vddio, *reg_vci;
static unsigned int backlight_value[16] = {
	0 ,8 ,12,15,18 ,21 ,24 ,27,
        30,52,70,85,100,140,190,255
};
/*fangchao 00159369 20120223 begin */
#ifdef CONFIG_FB_DYNAMIC_GAMMA

struct gamma_set_desc {
	int wait;
	int dlen;
	char *payload;
};
static char r63306_gamma22_setA_positive[14] = {0xc9,0x0f,0x10,0x1a,0x25,0x28,0x25,0x32,   
                   0x3e,0x38,0x3b,0x4d,0x36,0x33};
static char r63306_gamma22_setA_negative[14] = {0xca,0x30,0x2f,0x45,0x3a,0x37,0x3a,0x2d,
                   0x21,0x27,0x24,0x12,0x09,0x0c};
static char r63306_gamma22_setB_positive[14] = {0xcb,0x0f,0x10,0x1a,0x25,0x28,0x25,0x32,
                   0x3e,0x38,0x3b,0x4d,0x36,0x33};
static char r63306_gamma22_setB_negative[14] = {0xcc,0x30,0x2f,0x45,0x3a,0x37,0x3a,0x2d,
                   0x21,0x27,0x24,0x12,0x09,0x0c};
static char r63306_gamma22_setC_positive[14] = {0xcd,0x0f,0x10,0x1a,0x25,0x28,0x25,0x32,
                   0x3e,0x38,0x3b,0x4d,0x36,0x33};
static char r63306_gamma22_setC_negative[14] = {0xce,0x30,0x2f,0x45,0x3a,0x37,0x3a,0x2d,
                   0x21,0x27,0x24,0x12,0x09,0x0c};

static char r63306_gamma25_setA_positive[14] = {0xc9, 0x0f, 0x14, 0x21, 0x2e, 0x32, 0x2e, 0x3a, 0x45, 0x3f, 0x42, 0x52, 0x39, 0x33};
static char r63306_gamma25_setA_negative[14] = {0xca, 0x30, 0x2b, 0x3e, 0x31, 0x2d, 0x31, 0x25, 0x1a, 0x20, 0x1d, 0x0d, 0x06, 0x0c};
static char r63306_gamma25_setB_positive[14] = {0xcb, 0x0f, 0x14, 0x21, 0x2e, 0x32, 0x2e, 0x3a, 0x45, 0x3f, 0x42, 0x52, 0x39, 0x33};
static char r63306_gamma25_setB_negative[14] = {0xcc, 0x30, 0x2b, 0x3e, 0x31, 0x2d, 0x31, 0x25, 0x1a, 0x20, 0x1d, 0x0d, 0x06, 0x0c};
static char r63306_gamma25_setC_positive[14] = {0xcd, 0x0f, 0x14, 0x21, 0x2e, 0x32, 0x2e, 0x3a, 0x45, 0x3f, 0x42, 0x52, 0x39, 0x33};
static char r63306_gamma25_setC_negative[14] = {0xce, 0x30, 0x2b, 0x3e, 0x31, 0x2d, 0x31, 0x25, 0x1a, 0x20, 0x1d, 0x0d, 0x06, 0x0c};

                   
static struct dsi_cmd_desc toshiba_720p_r63308_dynamic_gamma_22[] = {
        {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_gamma22_setA_positive), r63306_gamma22_setA_positive},
        {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_gamma22_setA_negative), r63306_gamma22_setA_negative},
        {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_gamma22_setB_positive), r63306_gamma22_setB_positive},
        {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_gamma22_setB_negative), r63306_gamma22_setB_negative},
        {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_gamma22_setC_positive), r63306_gamma22_setC_positive},
        {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_gamma22_setC_negative), r63306_gamma22_setC_negative}
        };

static struct dsi_cmd_desc toshiba_720p_r63308_dynamic_gamma_25[] = {
        {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_gamma25_setA_positive), r63306_gamma25_setA_positive},
        {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_gamma25_setA_negative), r63306_gamma25_setA_negative},
        {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_gamma25_setB_positive), r63306_gamma25_setB_positive},
        {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_gamma25_setB_negative), r63306_gamma25_setB_negative},
        {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_gamma25_setC_positive), r63306_gamma25_setC_positive},
        {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_gamma25_setC_negative), r63306_gamma25_setC_negative}
        };        
#endif
/*fangchao 00159369 20120223 end */

static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
	/* bit_clk 402.17Mhz, 720*1280, RGB888, 4 Lane 60 fps video mode */
    /* regulator */
	{0x03, 0x0a, 0x04, 0x00, 0x20},
	/* timing */
	{0x66, 0x26, 0x1a, 0x00, 0x32, 0x91, 0x1e, 0x2b,
	0x1c, 0x03, 0x04, 0xa0},
    /* phy ctrl */
	{0x5f, 0x00, 0x00, 0x10},
    /* strength */
	{0xff, 0x00, 0x06, 0x00},
	/* pll control */
	{0x09, 0x76, 0x30, 0xc7, 0x00, 0x20, 0x0c, 0x62,
	0x41, 0x0f, 0x03,
	0x00, 0x1a, 0x00, 0x00, 0x02, 0x00, 0x20, 0x00, 0x02 },
};

static struct mdp_csc_cfg toshiba_720p_r63306_mdp_csc_cfg =
{
    (0),
    //w00182148 add for mdp tuning parameter begin 20120326//
    {
        0x022f, 0x1fc7, 0x1ff5,
        0x1fe3, 0x0213, 0x1ff5,
        0x1fe4, 0x1fc7, 0x0241,
    },
    {
        0x0, 0x0, 0x0,
    },
    {
        0x0009, 0x0009, 0x0009,
    },
    {
        0, 0xff, 0, 0xff, 0, 0xff,
    },
    {
        0, 0xff, 0, 0xff, 0, 0xff,
    },
    //w00182148 add for mdp tuning parameter end 20120326//
};

static char exit_sleep[2] = {0x11, 0x00};
static char display_on[2] = {0x29, 0x00};
//static char display_off[2] = {0x28, 0x00};
static char enter_sleep[2] = {0x10, 0x00};
/* add chip version r63308 support */
static char r63306_mcap_start[2] = {0xb0, 0x00};
static char r63306_acr[2] = {0xb2, 0x00};
static char r63306_clk_if[2] = {0xb3, 0x0c};
static char r63306_pix_format[2] = {0xb4, 0x02};
static char r63306_pwm_control[4] = {0xb9, 0x01, 0x00, 0x75};
static char r63306_cabc_off[2] = {0xbb, 0x08};
static char r63306_cabc_user_parameter[7] = {0xbe, 0x05, 0x00};
static char r63306_panel_drive_setting[6] = {0xc0, 0x40, 0x02, 0x7f, 0xc8, 0x08};
static char r63306_display_h_timing[16] = {0xc1, 0x00, 0xa8, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x9c, 0x08, 0x24, 0x0b, 0x00, 0x00, 0x00, 0x00};
static char r63306_source_output[6] = {0xc2, 0x00, 0x00, 0x0b, 0x00, 0x00};
static char r63306_gate_ic_control[2] = {0xc3, 0x04};
static char r63306_ltps_if_control_0[4] = {0xc4, 0x4d, 0x83, 0x00};
static char r63306_source_output_mode[11] = {0xc6, 0x13, 0x00, 0x08, 0x71, 0x00, 0x00, 
                    0x00, 0x00, 0x00, 0x00};
static char r63306_ltps_if_control_1[2] = {0xc7, 0x22};
static char r63306_gamma_control[5] = {0xc8, 0x07, 0x00, 0x07, 0x00};
static char r63306_gamma_setA_positive[14] = {0xc9, 0x43, 0x25, 0x3b, 0x39, 0x31, 0x23, 0x27,
                    0x2c, 0x26, 0x29, 0x40, 0x2d, 0x74};
static char r63306_gamma_setA_negative[14] = {0xca, 0x7c, 0x1a, 0x24, 0x26, 0x2e, 0x3c, 0x38,
                    0x33, 0x39, 0x36, 0x1f, 0x12, 0x4b};
static char r63306_gamma_setB_positive[14] = {0xcb, 0x43, 0x25, 0x3b, 0x39, 0x31, 0x23, 0x27,
                    0x2c, 0x26, 0x29, 0x40, 0x2d, 0x74};
static char r63306_gamma_setB_negative[14] = {0xcc, 0x7c, 0x1a, 0x24, 0x26, 0x2e, 0x3c, 0x38,
                    0x33, 0x39, 0x36, 0x1f, 0x12, 0x4b};
static char r63306_gamma_setC_positive[14] = {0xcd, 0x43, 0x25, 0x3b, 0x39, 0x31, 0x23, 0x27,
                    0x2c, 0x26, 0x29, 0x40, 0x2d, 0x74};
static char r63306_gamma_setC_negative[14] = {0xce, 0x7c, 0x1a, 0x24, 0x26, 0x2e, 0x3c, 0x38,
                    0x33, 0x39, 0x36, 0x1f, 0x12, 0x4b};
static char r63306_power_setting1[4] = {0xd0, 0x69, 0x65, 0x01};
static char r63306_power_setting2[3] = {0xd1, 0x77, 0xd4};
static char r63306_power_setting_internal[2] = {0xd3, 0x33};
static char r63306_vpnlvl[3] = {0xd5, 0x0c, 0x0c};
static char r63306_vcomdc_0[7] = {0xd8, 0x34, 0x64, 0x23, 0x25, 0x62, 0x32};
static char r63306_vcomdc_1[12] = {0xde, 0x01, 0x00, 0x31, 0x46, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00};
static char r63306_nvm_load_control[2] = {0xe2, 0x00};

static char r63308_mcap_start[2] = {0xb0, 0x00};
static char r63308_cabc[2] = {0xbb, 0x0c};
static char r63308_nvm_load_control[2] = {0xe2, 0x01};
static char r63308_color_enhance[12] = {0xbd, 0x60, 0x98, 0x60, 0xC0, 0x90, 0xD0, 0xB0, 0x90, 0x20, 0x00, 0x80};
//w0182148 add for cabc begin 20120306//
static char r63308_cabc_UI_TEST1[15] = {0xb7,0x18,0x00,0x18,0x18,0x0c,0x10,0x5c,0x10,0xac,0x10,0x0c,0x10,0x00,0x10};
static char r63308_cabc_UI_TEST2[14] = {0xb8,0xf8,0xda,0x6d,0xfb,0xff,0xff,0xcf,0x1f,0xc0,0xd3,0xe3,0xf1,0xff};
static char r63308_cabc_UI_USEPARAMETER[7] = {0xbe,0x00,0x00,0x2,0x2,0x4,0x4};

static char r63308_cabc_VIDEO_TEST1[15] = {0xb7,0x18,0x00,0x18,0x18,0x0c,0x13,0x5c,0x13,0xac,0x13,0x0c,0x13,0x00,0x10};
static char r63308_cabc_VIDEO_TEST2[14] = {0xb8,0xf8,0xda,0x6d,0xfb,0xff,0xff,0xcf,0x1f,0x67,0x89,0xaf,0xd6,0xff};
static char r63308_cabc_VIDEO_USEPARAMETER[7] = {0xbe,0x00,0x00,0x00,0x18,0x4,0x40};

static struct dsi_cmd_desc resa_r63308_cabc_UI_SET[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63308_cabc_UI_TEST1), r63308_cabc_UI_TEST1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63308_cabc_UI_TEST2), r63308_cabc_UI_TEST2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63308_cabc_UI_USEPARAMETER), r63308_cabc_UI_USEPARAMETER},
};

static struct dsi_cmd_desc resa_r63308_cabc_VIDEO_SET[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63308_cabc_VIDEO_TEST1), r63308_cabc_VIDEO_TEST1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63308_cabc_VIDEO_TEST2), r63308_cabc_VIDEO_TEST2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63308_cabc_VIDEO_USEPARAMETER), r63308_cabc_VIDEO_USEPARAMETER}
};
//w0182148 add for cabc end 20120306//

/* set lcd backlight from r63306 ledpwmout */
#ifndef MIPI_TOSHIBA_720P_PWM_PMIC
static struct dsi_cmd_desc toshiba_720p_backlight_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_cabc_user_parameter), r63306_cabc_user_parameter},
};
#endif

/*exit sleep cmd should be first according to new spec*/
static struct dsi_cmd_desc toshiba_720p_r63306_display_on_cmds[] = {
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(r63306_mcap_start), r63306_mcap_start},
    {DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(r63306_acr), r63306_acr},
    {DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(r63306_clk_if), r63306_clk_if},
    {DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(r63306_pix_format), r63306_pix_format},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_pwm_control), r63306_pwm_control},
    {DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(r63306_cabc_off), r63306_cabc_off},
  //  {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cabc_user_parameter), cabc_user_parameter},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_panel_drive_setting), r63306_panel_drive_setting},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_display_h_timing), r63306_display_h_timing},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_source_output), r63306_source_output},
    {DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(r63306_gate_ic_control), r63306_gate_ic_control},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_ltps_if_control_0), r63306_ltps_if_control_0},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_source_output_mode), r63306_source_output_mode},
    {DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(r63306_ltps_if_control_1), r63306_ltps_if_control_1},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_gamma_control), r63306_gamma_control},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_gamma_setA_positive), r63306_gamma_setA_positive},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_gamma_setA_negative), r63306_gamma_setA_negative},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_gamma_setB_positive), r63306_gamma_setB_positive},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_gamma_setB_negative), r63306_gamma_setB_negative},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_gamma_setC_positive), r63306_gamma_setC_positive},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_gamma_setC_negative), r63306_gamma_setC_negative},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_power_setting1), r63306_power_setting1},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_power_setting2), r63306_power_setting2},
    {DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(r63306_power_setting_internal), r63306_power_setting_internal},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_vpnlvl), r63306_vpnlvl},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_vcomdc_0), r63306_vcomdc_0},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63306_vcomdc_1), r63306_vcomdc_1},
    {DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(r63306_nvm_load_control), r63306_nvm_load_control},
    {DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(exit_sleep), exit_sleep},
    {DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(display_on), display_on}
};

static struct dsi_cmd_desc toshiba_720p_r63308_display_on_cmds[] = {
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(r63308_mcap_start), r63308_mcap_start},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(r63308_cabc), r63308_cabc},
    {DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(r63308_nvm_load_control), r63308_nvm_load_control},
    {DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
    {DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(r63308_cabc), r63308_cabc},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63308_color_enhance), r63308_color_enhance},
};
static struct dsi_cmd_desc toshiba_720p_display_off_cmds[] = {
	//{DTYPE_DCS_WRITE, 1, 0, 0, 50, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(enter_sleep), enter_sleep}
};

static char get_pwr_state[2] = {0x0a, 0x00}; /* DTYPE_DCS_READ */
static char get_chip_ver[2] = {0xbf, 0x00};
static char enter_dstb[2] = {0xb1, 0x01};

static struct dsi_cmd_desc mipi_toshiba_720p_get_pwr_state_cmd = {
	DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(get_pwr_state), get_pwr_state};
static struct dsi_cmd_desc mipi_toshiba_720p_chip_ver_cmd = {
    DTYPE_GEN_READ, 1, 0, 1, 5, sizeof(get_chip_ver), get_chip_ver};
static struct dsi_cmd_desc toshiba_720p_enter_dstb_cmd[] = {
    {DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(enter_dstb), enter_dstb}};

static void mipi_toshiba_720p_get_pwr_state(struct msm_fb_data_type *mfd)
{
	struct dsi_buf *rp, *tp;
	struct dsi_cmd_desc *cmd;

	tp = &toshiba_tx_buf;
	rp = &toshiba_rx_buf;
	cmd = &mipi_toshiba_720p_get_pwr_state_cmd;
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 1);
	pr_debug("%s: power mode reg:0x%02x\n", __func__, *(char *)rp->data);
}

static u8 mipi_toshiba_720p_get_chip_ver(struct msm_fb_data_type *mfd)
{
    struct dsi_buf *rp, *tp;
    struct dsi_cmd_desc *cmd;

    tp = &toshiba_tx_buf;
    rp = &toshiba_rx_buf;
    cmd = &mipi_toshiba_720p_chip_ver_cmd;
    mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 5);
    return rp->data[3];
}

static void mipi_toshiba_720p_enter_dstb(struct msm_fb_data_type *mfd)
{
	mipi_dsi_cmds_tx(mfd, &toshiba_tx_buf, toshiba_720p_enter_dstb_cmd,
			ARRAY_SIZE(toshiba_720p_enter_dstb_cmd));
}
extern struct platform_device msm_mipi_dsi1_device;

static int mipi_toshiba_720p_panel_power(int on)
{
	int rc;

    mutex_lock(&power_lock);
	pr_debug("%s: state : %d enter \n", __func__, on);
    
	if (on&&(panel_power_on == false)) {
		rc = regulator_set_optimum_mode(reg_vddio, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode reg_vddio failed, rc=%d\n", rc);
            mutex_unlock(&power_lock);
			return -EINVAL;
		}
		rc = regulator_set_optimum_mode(reg_vci, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode reg_vci failed, rc=%d\n", rc);
            mutex_unlock(&power_lock);
			return -EINVAL;
		}
		rc = regulator_enable(reg_vddio);
		if (rc) {
			pr_err("enable reg_vddio failed, rc=%d\n", rc);
            mutex_unlock(&power_lock);
			return -ENODEV;
		}
		rc = regulator_enable(reg_vci);
		if (rc) {
			pr_err("enable reg_vci failed, rc=%d\n", rc);
            mutex_unlock(&power_lock);
			return -ENODEV;
		}
		msleep(30);
		gpio_set_value_cansleep(reset_gpio, 1);
        panel_power_on = true;
	} else if(!on&&(panel_power_on == true))
	{
		gpio_set_value_cansleep(reset_gpio, 0);
        msleep(120);
		rc = regulator_disable(reg_vddio);
		if (rc) {
			pr_err("disable reg_vddio failed, rc=%d\n", rc);
            mutex_unlock(&power_lock);
			return -ENODEV;
		}
		rc = regulator_disable(reg_vci);
		if (rc) {
			pr_err("disable reg_vci failed, rc=%d\n", rc);
            mutex_unlock(&power_lock);
			return -ENODEV;
		}
		rc = regulator_set_optimum_mode(reg_vddio, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode reg_vddio failed, rc=%d\n", rc);
            mutex_unlock(&power_lock);
			return -EINVAL;
		}
		rc = regulator_set_optimum_mode(reg_vci, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode reg_vci failed, rc=%d\n", rc);
            mutex_unlock(&power_lock);
			return -EINVAL;
		}
        panel_power_on = false;
	}
	pr_debug("%s: state : %d out \n", __func__, on);
    mutex_unlock(&power_lock);
    
	return 0;
}

/*use dstb mode when sleep*/
/* modify for 8960 t0 */
//w0182148 add for cabc begin 20120306//
int set_cabc(struct lcd_tuning_dev *ltc, enum tft_cabc cabc)
{
    struct msm_fb_data_type *mfd;
    int ret = 0;
    mfd = mfd_toshiba;
    switch(cabc)
    {
        case CABC_UI:
            //donot touch backlight begin//
            r63308_cabc_UI_USEPARAMETER[1] = r63306_cabc_user_parameter[1];
            r63308_cabc_UI_USEPARAMETER[2] = r63306_cabc_user_parameter[2];
            //donot touch backlight end//
            mutex_lock(&mfd->dma->ov_mutex);
            if (mdp4_overlay_dsi_state_get() == ST_DSI_SUSPEND) {
                mutex_unlock(&mfd->dma->ov_mutex);
                return 0;
            }
            mipi_set_tx_power_mode(0);
            mipi_dsi_cmds_tx(mfd, &toshiba_tx_buf, resa_r63308_cabc_UI_SET,
            ARRAY_SIZE(resa_r63308_cabc_UI_SET));
            mutex_unlock(&mfd->dma->ov_mutex);
            break ;
        case CABC_VID:
            //donot touch backlight begin//
            r63308_cabc_VIDEO_USEPARAMETER[1] = r63306_cabc_user_parameter[1];
            r63308_cabc_VIDEO_USEPARAMETER[2] = r63306_cabc_user_parameter[2];
            //donot touch backlight end//
            mutex_lock(&mfd->dma->ov_mutex);
            if (mdp4_overlay_dsi_state_get() == ST_DSI_SUSPEND) {
                mutex_unlock(&mfd->dma->ov_mutex);
                return 0;
            }
            mipi_set_tx_power_mode(0);
            mipi_dsi_cmds_tx(mfd, &toshiba_tx_buf, resa_r63308_cabc_VIDEO_SET,
            ARRAY_SIZE(resa_r63308_cabc_VIDEO_SET));
            mutex_unlock(&mfd->dma->ov_mutex);
            break;
        default:
            ret= 0;
            break;
    }
    return ret;
}
//w0182148 add for cabc end 20120306//
static int mipi_toshiba_720p_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
    u8 chip_id;
	mfd = platform_get_drvdata(pdev);

	if(!mfd_toshiba)
		mfd_toshiba = mfd;

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

    mutex_lock(&power_lock);
    if(unlikely(!panel_power_on))
    {
        mutex_unlock(&power_lock);
        mipi_toshiba_720p_panel_power(1);
/* BEGIN: Added by w00182148, 2012/03/20 for cabc */

//Default is UI mode.
//donot touch backlight begin//
	r63308_cabc_UI_USEPARAMETER[1] = r63306_cabc_user_parameter[1];
	r63308_cabc_UI_USEPARAMETER[2] = r63306_cabc_user_parameter[2];
//donot touch backlight END//
    mipi_set_tx_power_mode(0);
				mipi_dsi_cmds_tx(mfd, &toshiba_tx_buf, resa_r63308_cabc_UI_SET,
                ARRAY_SIZE(resa_r63308_cabc_UI_SET));
 /* END: Added by w00182148, 2012/03/20 for cabc */
        return 0;
    }
    mutex_unlock(&power_lock);
	pr_debug("mipi_toshiba_lcd_on enter\n");
    
    gpio_set_value_cansleep(reset_gpio,1);
    msleep(1);
    gpio_set_value_cansleep(reset_gpio,0);
    msleep(5);
    gpio_set_value_cansleep(reset_gpio,1);
    msleep(10);
	/* enable vsp and vsn */    
	gpio_direction_output(LCD_BIAS_EN, 1);
	msleep(10);
/* delete 3 lines  */
    mipi_set_tx_power_mode(0);
    chip_id = mipi_toshiba_720p_get_chip_ver(mfd);
    if((chip_id!=CHIP_VER_R63308)&&(chip_id!=CHIP_VER_R63306))
    {
        pr_info("get chip_id error,use default chip ver:r63308\n");
        chip_id = CHIP_VER_R63308;
    }
    if(chip_id == CHIP_VER_R63308)
    {
    	mipi_dsi_cmds_tx(mfd, &toshiba_tx_buf, toshiba_720p_r63308_display_on_cmds,
    			ARRAY_SIZE(toshiba_720p_r63308_display_on_cmds));
    }
    else
    {
    	mipi_dsi_cmds_tx(mfd, &toshiba_tx_buf, toshiba_720p_r63306_display_on_cmds,
    			ARRAY_SIZE(toshiba_720p_r63306_display_on_cmds));
    }

	/* BEGIN: Added by w00182148, 2012/03/20 for cabc */

		//Default is UI mode.
		//donot touch backlight begin//
		r63308_cabc_UI_USEPARAMETER[1] = r63306_cabc_user_parameter[1];
		r63308_cabc_UI_USEPARAMETER[2] = r63306_cabc_user_parameter[2];
        //donot touch backlight end//

		mipi_dsi_cmds_tx(mfd, &toshiba_tx_buf, resa_r63308_cabc_UI_SET,
                ARRAY_SIZE(resa_r63308_cabc_UI_SET));

	/* END:   Added by w00182148, 2012/03/20 for cabc */

    mipi_toshiba_720p_get_pwr_state(mfd);
	pr_debug("mipi_toshiba_lcd_on out\n");

	return 0;
}

/*use dstb mode when sleep*/
/* modify for 8960 t0 */
static int mipi_toshiba_720p_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct fb_info *fbi;
	mfd = platform_get_drvdata(pdev);
    
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
    fbi = mfd->fbi;
    
	memset(fbi->screen_base, 0x0, fbi->fix.smem_len);
	pr_debug("mipi_toshiba_lcd_off enter\n");
        mipi_set_tx_power_mode(0);
	mipi_dsi_cmds_tx(mfd, &toshiba_tx_buf, toshiba_720p_display_off_cmds,
			ARRAY_SIZE(toshiba_720p_display_off_cmds));

	/* disable vsp and vsn */
	gpio_direction_output(LCD_BIAS_EN, 0);
	msleep(10);

    mipi_toshiba_720p_get_pwr_state(mfd);

    mipi_toshiba_720p_enter_dstb(mfd);
	pr_debug("mipi_toshiba_lcd_off out\n");
	return 0;
}

/* fangchao 00159369 20120223 begin*/
#ifdef CONFIG_FB_DYNAMIC_GAMMA
int R63306_set_dynamic_gamma(struct lcd_tuning_dev *ltd, enum lcd_gamma gamma)
{
    int ret = 0;
	struct msm_fb_data_type *mfd;
	mfd = mfd_toshiba;
    switch(gamma)
    {
        case GAMMA25:
        mutex_lock(&mfd->dma->ov_mutex);
        if (mdp4_overlay_dsi_state_get() == ST_DSI_SUSPEND) {
            mutex_unlock(&mfd->dma->ov_mutex);
            printk("########error : writing lcd reg failed \n");
            return 1;
        }
        //mdp4_dsi_cmd_dma_busy_wait(mfd);
        //mdp4_dsi_blt_dmap_busy_wait(mfd);
        //mipi_dsi_mdp_busy_wait(mfd);
         mipi_set_tx_power_mode(0);
        mipi_dsi_cmds_tx(mfd, &toshiba_tx_buf, toshiba_720p_r63308_dynamic_gamma_25,
        ARRAY_SIZE(toshiba_720p_r63308_dynamic_gamma_25));
        printk("########mipi_toshiba_dynamic_gamma 2.5 setting \n");
        mutex_unlock(&mfd->dma->ov_mutex);
        break ;
        case GAMMA22:

        mutex_lock(&mfd->dma->ov_mutex);
        if (mdp4_overlay_dsi_state_get() == ST_DSI_SUSPEND) {
            mutex_unlock(&mfd->dma->ov_mutex);
            printk("########error : writing lcd reg failed \n");
            return 1;
        }
        //mdp4_dsi_cmd_dma_busy_wait(mfd);
        //mdp4_dsi_blt_dmap_busy_wait(mfd);
        //mipi_dsi_mdp_busy_wait(mfd);
         mipi_set_tx_power_mode(0);
        mipi_dsi_cmds_tx(mfd, &toshiba_tx_buf, toshiba_720p_r63308_dynamic_gamma_22,
        ARRAY_SIZE(toshiba_720p_r63308_dynamic_gamma_22));
        mutex_unlock(&mfd->dma->ov_mutex);
        printk("########mipi_toshiba_dynamic_gamma 2.2 setting \n");
            break ;
        default:
            ret= -1;
            break;
    }
    printk("%s: change gamma mode to %d\n",__func__,gamma);
	return ret;
}
#endif

/* w0182148 add for cabc begin 20120306*/
static struct lcd_tuning_ops sp_tuning_ops = 
{
    #ifdef CONFIG_FB_DYNAMIC_GAMMA
    .set_gamma = R63306_set_dynamic_gamma,
    #endif    
    #ifdef CONFIG_FB_cabc
    .set_cabc = set_cabc,
    #endif
};
/*fangchao 00159369 20120223 end */
/* w0182148 add for cabc end 20120306*/

/*add for lcd power down sequence*/
static int __devinit mipi_toshiba_720p_lcd_probe(struct platform_device *pdev)
{
    int rc;
    /*<fangchao 00159369 20120223 begin*/
	struct lcd_tuning_dev *ltd;
	struct lcd_properities lcd_props;
    /*fangchao 00159369 20120223 end>*/

	if (pdev->id == 0) {
		mipi_toshiba_pdata = pdev->dev.platform_data;
		return 0;
	}

	if (mipi_toshiba_pdata != NULL)
		bl_lpm = pwm_request(mipi_toshiba_pdata->gpio[0],
			"backlight");

	if (bl_lpm == NULL || IS_ERR(bl_lpm)) {
		pr_err("%s pwm_request() failed\n", __func__);
		bl_lpm = NULL;
	}
	pr_debug("bl_lpm = %p lpm = %d\n", bl_lpm,
		mipi_toshiba_pdata->gpio[0]);

    reg_vci = regulator_get(&msm_mipi_dsi1_device.dev,
            "dsi_vdc");
    if (IS_ERR(reg_vci)) {
        pr_err("could not get reg_vci, rc = %ld\n",
            PTR_ERR(reg_vci));
        return -ENODEV;
    }
    reg_vddio = regulator_get(&msm_mipi_dsi1_device.dev,
            "dsi_vddio");
    if (IS_ERR(reg_vddio)) {
        pr_err("could not get reg_vddio, rc = %ld\n",
            PTR_ERR(reg_vddio));
        regulator_put(reg_vci);
        return -ENODEV;
    }
    rc = regulator_set_voltage(reg_vci, 2850000, 2850000);
    if (rc) {
        pr_err("set_voltage reg_vci failed, rc=%d\n", rc);
        regulator_put(reg_vci);
        regulator_put(reg_vddio);
        return -EINVAL;
    }
    rc = regulator_set_voltage(reg_vddio, 1800000, 1800000);
    if (rc) {
        pr_err("set_voltage reg_vddio failed, rc=%d\n", rc);
        regulator_put(reg_vci);
        regulator_put(reg_vddio);
        return -EINVAL;
    }

	msm_fb_add_device(pdev);
	/* BEGIN: Added by w00182148, 2012/03/07   PN:cabc*/
    lcd_props.cabc_mode = CABC_UI;
	/* end: Added by w00182148, 2012/03/07   PN:cabc*/
	/*  wenjuan 00182148 20111228 begin*/
    lcd_props.type = TFT;
    lcd_props.default_gamma = GAMMA25;
    ltd = lcd_tuning_dev_register(&lcd_props, &sp_tuning_ops, NULL);
	 /*  wenjuan 00182148 20111228 begin*/
	return 0;
}

static void  mipi_toshiba_720p_lcd_shutdown(struct platform_device *pdev)
{
    gpio_direction_output(LCD_BIAS_EN, 0);
    msleep(15);
    mipi_toshiba_720p_panel_power(0);
}

static void mipi_toshiba_720p_set_backlight(struct msm_fb_data_type *mfd)
{
	printk("%s: back light level %d\n", __func__, mfd->bl_level);
/* set lcd backlight from r63306 ledpwmout */
#ifdef MIPI_TOSHIBA_720P_PWM_PMIC
	int ret;
	printk("%s: back light level %d\n", __func__, mfd->bl_level);
    	mfd->bl_level = mfd->bl_level*16;
	if (bl_lpm) {
		ret = pwm_config(bl_lpm, MIPI_TOSHIBA_720P_PWM_DUTY_LEVEL *
			mfd->bl_level, MIPI_TOSHIBA_720P_PWM_PERIOD_USEC);
		if (ret) {
			pr_err("pwm_config on lpm failed %d\n", ret);
			return;
		}
		if (mfd->bl_level) {
			ret = pwm_enable(bl_lpm);
			if (ret)
				pr_err("pwm enable/disable on lpm failed"
					"for bl %d\n",	mfd->bl_level);
		} else {
			pwm_disable(bl_lpm);
		}
	}
/* set lcd backlight from r63306 ledpwmout */	
#else
    mutex_lock(&mfd->dma->ov_mutex);
    if (mdp4_overlay_dsi_state_get() == ST_DSI_SUSPEND) {
		mutex_unlock(&mfd->dma->ov_mutex);
		return;
    }
    mdp4_dsi_cmd_dma_busy_wait(mfd);
    mdp4_dsi_blt_dmap_busy_wait(mfd);
    mipi_dsi_mdp_busy_wait(mfd);
    r63306_cabc_user_parameter[1] = backlight_value[mfd->bl_level] >> 4;
    r63306_cabc_user_parameter[2] = backlight_value[mfd->bl_level] & 0x0f;
    mipi_set_tx_power_mode(0);
    mipi_dsi_cmds_tx(mfd, &toshiba_tx_buf, toshiba_720p_backlight_cmds,
			ARRAY_SIZE(toshiba_720p_backlight_cmds));
    mutex_unlock(&mfd->dma->ov_mutex);
#endif
}

static struct platform_driver this_driver = {
	.probe  = mipi_toshiba_720p_lcd_probe,
    .shutdown  = mipi_toshiba_720p_lcd_shutdown,
	.driver = {
		.name   = "mipi_toshiba_720p",
	},
};

static struct msm_fb_panel_data toshiba_720p_panel_data = {
	.on		= mipi_toshiba_720p_lcd_on,
	.off		= mipi_toshiba_720p_lcd_off,
	.set_backlight  = mipi_toshiba_720p_set_backlight,
};

static int mipi_toshiba_720p_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_toshiba_720p", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	toshiba_720p_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &toshiba_720p_panel_data,
		sizeof(toshiba_720p_panel_data));
	if (ret) {
		pr_err("%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		pr_err("%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

extern struct mdp_csc_cfg csc_matrix[3];

/* add lcd id self adapt */
/* modify for 8960 t0 */
static int __init mipi_video_toshiba_720p_init(void)
{
	int ret;

/* optmize lcd self adapt function */
    if(0 != lcd_detect_panel("toshiba_lcd_mipi_video_720p"))
    {
        return 0;
    }
	pr_info("%s:mipi_video_toshiba_video_pt_init\n", __func__);
    memcpy(&csc_matrix[2],&toshiba_720p_r63306_mdp_csc_cfg,sizeof(struct mdp_csc_cfg));

	pinfo.xres = 720;
	pinfo.yres = 1280;
	pinfo.width= 55;
	pinfo.height= 98;
	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.lcdc.h_back_porch = 64;
	pinfo.lcdc.h_front_porch = 64;
	pinfo.lcdc.h_pulse_width = 16;
	pinfo.lcdc.v_back_porch = 3;
	pinfo.lcdc.v_front_porch = 9;
	pinfo.lcdc.v_pulse_width = 4;
	pinfo.lcdc.border_clr = 0;	/* blk */
	pinfo.lcdc.underflow_clr = 0xff;	/* blue */
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 15;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;

	pinfo.clk_rate = 405000000;

	pinfo.mipi.mode = DSI_VIDEO_MODE;
	pinfo.mipi.pulse_mode_hsa_he = TRUE;
	pinfo.mipi.hfp_power_stop = FALSE;
	pinfo.mipi.hbp_power_stop = FALSE;
	pinfo.mipi.hsa_power_stop = FALSE;
	pinfo.mipi.eof_bllp_power_stop = TRUE;
	pinfo.mipi.bllp_power_stop = TRUE;
	pinfo.mipi.traffic_mode = DSI_NON_BURST_SYNCH_EVENT;
	pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.data_lane2 = TRUE;
	pinfo.mipi.data_lane3 = TRUE;
	pinfo.mipi.t_clk_post = 0x05;
	pinfo.mipi.t_clk_pre = 0x15;
	pinfo.mipi.stream = 0; /* dma_p */
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.frame_rate = 60;
	pinfo.mipi.dsi_phy_db = &dsi_video_mode_phy_db;

	ret = mipi_toshiba_720p_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_WVGA_PT);
	if (ret)
    {   
		pr_err("%s: failed to register device!\n", __func__);
        goto device_reg_failed;
    }

	mipi_dsi_buf_alloc(&toshiba_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&toshiba_rx_buf, DSI_BUF_SIZE);

    ret = platform_driver_register(&this_driver);
	if (ret)
    {   
		pr_err("%s: failed to register driver!\n", __func__);
        goto driver_reg_failed;
    }
    
    ret = gpio_request(LCD_BIAS_EN, "lcd_bias_en");
    if (ret) {
        pr_err("request LCD_BIAS_EN 42 failed, rc=%d\n", ret);
        goto driver_reg_failed;
    }

    reset_gpio = PM8921_GPIO_PM_TO_SYS(43);
    ret = gpio_request(reset_gpio, "disp_rst_n");
    if (ret) {
        pr_err("request gpio 43 failed, ret=%d\n", ret);
        goto reset_gpio_request_failed;
    }
    mutex_init(&power_lock);
    return 0;

reset_gpio_request_failed:
    gpio_free(LCD_BIAS_EN);
driver_reg_failed:
    kfree(toshiba_tx_buf.data);
    memset(&toshiba_tx_buf,0,sizeof(struct dsi_buf));
    kfree(toshiba_rx_buf.data);
    memset(&toshiba_rx_buf,0,sizeof(struct dsi_buf));
device_reg_failed:
	return ret;
}

module_init(mipi_video_toshiba_720p_init);
