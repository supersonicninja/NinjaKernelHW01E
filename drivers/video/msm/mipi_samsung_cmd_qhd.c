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
#include <hsad/config_interface.h>
/*add for lcd power down timing*/
#include <linux/gpio.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include "msm_fb.h"
#include "mipi_dsi.h"

/* wenjuan 00182148 20111205 begin >*/
#include <linux/msm_mdp.h>
#include <linux/lcd_tuning.h>
/* wenjuan 00182148 20111205 end >*/
#include <linux/msm_mdp.h>

/* add backlight */
#include "mdp4.h"

/* optmize lcd self adapt function,delete 3 lines */
/*add for lcd power down timing*/
#define PM8921_GPIO_BASE		NR_GPIO_IRQS
#define PM8921_GPIO_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_GPIO_BASE)
static int lcd_on_off = 1;
static struct msm_panel_info pinfo;
static int ch_used[3];
static struct dsi_buf samsung_tx_buf;
static struct dsi_buf samsung_rx_buf;
static struct msm_panel_common_pdata *mipi_samsung_pdata;
/* add for disable disorder screen when lcd on */
static struct msm_fb_data_type *samsung_mfd = NULL;
static struct workqueue_struct *samsung_disp_on_wq;
static struct work_struct disp_on_work;
/*add for lcd power down timing*/
static bool panel_power_on = false;
/*w00182148 add ACL 20120210 begin*/
int lcd_acl_state = 0;
/*w00182148 add ACL 20120210 end*/
static struct mutex power_lock;
static int reset_gpio;
static struct regulator *reg_vddio, *reg_vci;
static bool new_materal = false;
static struct mipi_dsi_phy_ctrl dsi_cmd_mode_phy_db = {
	/* 600*1024, RGB888, 3 Lane 55 fps video mode */
    /* regulator */
	{0x03, 0x0a, 0x04, 0x00, 0x20},
	/* timing */
	{0xa9, 0x8a, 0x18, 0x00, 0x91, 0x91, 0x1b, 0x8c,
	0x1a, 0x03, 0x04, 0xa0},
    /* phy ctrl */
	{0x5f, 0x00, 0x00, 0x10},
    /* strength */
	{0xff, 0x00, 0x06, 0x00},
	/* pll control */
	{0x0, 0x71, 0x1, 0x1a, 0x00, 0x50, 0x48, 0x63,
	0x41, 0x0f, 0x07,
	0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01 },
};

static struct mdp_csc_cfg samsung_mdp_csc_cfg =
{
    (0),
    //w00182148 add for mdp tuning parameter begin 20120326//
    {
        0x0224, 0x1fd0, 0x1ff7,
        0x1fe8, 0x020d, 0x1ff7,
        0x1fe8, 0x1fd0, 0x0234,
    },
    {
        0x0, 0x0, 0x0,
    },
    {
        0x0001, 0x0001, 0x0001,
    },
    {
        0, 0xff, 0, 0xff, 0, 0xff,
    },
    {
        0, 0xff, 0, 0xff, 0, 0xff,
    },
    //w00182148 add for mdp tuning parameter end 20120326//
};
/*add soft reset for disable disorder screen when wake up*/
static char soft_reset[2] = {0x01, 0x00};
static char etc_condition_set1_0[3] = {0xf0, 0x5a, 0x5a};
static char etc_condition_set1_1[3] = {0xf1, 0x5a, 0x5a};
static char etc_condition_set1_2[3] = {0xfc, 0x5a, 0x5a};

static char gamma_set_300[26] = {0xfa, 0x02, 0x10, 0x10, 0x10, 0xd1, 0x34, 0xd0, 0xd1, 0xba,
             0xdc, 0xe0, 0xd9, 0xe2, 0xc2, 0xc0, 0xbf, 0xd4, 0xd5, 0xd0,
             0x00, 0x73, 0x00, 0x59, 0x00, 0x82};
static char gamma_set_300_new_materal[26] = {0xfa, 0x02, 0x38, 0x00, 0x56, 0xc9, 0xdd, 0x94, 0xc2, 0xd9,
             0xac, 0xd3, 0xe6, 0xce, 0xae, 0xc5, 0x9d, 0xc6, 0xd4, 0xb9,
             0x00, 0x8a, 0x00, 0x82, 0x00, 0x9f};
static char gamma_update[2] = {0xfa, 0x03};
static char panel_condition_set[14] = {0xf8, 0x27, 0x27, 0x08, 0x08, 0x4e, 0xaa, 0x5e, 0x8a,
               0x10, 0x3f, 0x10, 0x10, 0x00};
static char etc_condition_set2_0[4] = {0xf6, 0x00, 0x84, 0x09};
static char etc_condition_set2_1[2] = {0xb0, 0x09};
static char etc_condition_set2_2[2] = {0xd5, 0x64};
static char etc_condition_set2_3[2] = {0xb0, 0x0b};
static char etc_condition_set2_4[4] = {0xd5, 0xa4, 0x7e, 0x20};
static char etc_condition_set2_5[2] = {0xf7, 0x03};
static char etc_condition_set2_6[2] = {0xb0, 0x02};
static char etc_condition_set2_7[2] = {0xb3, 0xc3};
//add for sloving font edge blur by wenjuan 00182148 20120405 begin//
static char etc_condition_set2_17[2] = {0xb3, 0x6c};
//add for sloving font edge blur by wenjuan 00182148 20120405 end//
static char etc_condition_set2_8[2] = {0xb0, 0x08};
static char etc_condition_set2_9[2] = {0xfd, 0xf8};
static char etc_condition_set2_10[2] = {0xb0, 0x04};
static char etc_condition_set2_11[2] = {0xf2, 0x4d};
static char etc_condition_set2_12[2] = {0xb0, 0x05};
static char etc_condition_set2_13[2] = {0xfd, 0x1f};
static char etc_condition_set2_14[4] = {0xb1, 0x01, 0x00, 0x16};
static char etc_condition_set2_15[5] = {0xb2, 0x06, 0x06, 0x06, 0x06};
static char etc_condition_set2_15_new_materal[5] = {0xb2, 0x08, 0x08, 0x08, 0x08};
static char etc_condition_set2_16[2] = {0x11,0x00};//change by wenjuan 00182148 20111206//
static char mem_winodw_set1_0[5] = {0x2a, 0x00, 0x00, 0x02, 0x57};
static char mem_winodw_set1_1[5] = {0x2b, 0x00, 0x00, 0x03, 0xff};

static char mem_winodw_set2_0[2] = {0x35, 0x00};
static char mem_winodw_set2_1[5] = {0x2a, 0x00, 0x1e, 0x02, 0x39};
static char mem_winodw_set2_2[5] = {0x2b, 0x00, 0x00, 0x03, 0xbf};
static char mad_ctl[2] = {0x36, 0x00};
static char mem_winodw_set2_3[2] = {0xd1, 0x8a};
static char acl_off[2] = {0xc0, 0x00};
static char acl_on[2] = {0xc0, 0x01};
static char acl_setting[29] = {0xc1, 0x47,  0x53,  0x13,  0x53,
						   0x00, 0x00,  0x01,   0xdf,  0x00,
						   0x00, 0x03,  0x1f,   0x00,  0x00,
						   0x00, 0x00,  0x00,   0x01,  0x02, 
						   0x03, 0x07,  0x0e,   0x14,  0x1c,
						   0x24, 0x2d,  0x2d,   0x00};
//static char acl_on[2] = {0xc0, 0x00};
/*w00182148 add for ACL 20120207 begin */
//static char exit_sleep[2] = {0x11, 0x00};
static char display_on[2] = {0x29, 0x00};
static char display_off[2] = {0x28, 0x00};
static char enter_sleep[2] = {0x10, 0x00};
static char get_pwr_state[2]  = {0x0a,0x00};
/* add backlight */
/* gamma config for backlight */
static char bl_data[][26] = {

           {0xfa, 0x02, 0x10, 0x10, 0x10, 0xd1, 0x34, 0xd0, 0xd1, 0xba,
             0xdc, 0xe0, 0xd9, 0xe2, 0xc2, 0xc0, 0xbf, 0xd4, 0xd5, 0xd0,
             0x00, 0x73, 0x00, 0x59, 0x00, 0x82}, /* 70 */
           {0xfa, 0x02, 0x10, 0x10, 0x10, 0xd7, 0x39, 0xd6, 0xd6, 0xbf,
             0xdd, 0xe1, 0xda, 0xe2, 0xc0, 0xbf, 0xbd, 0xd3, 0xd5, 0xcf,
             0x00, 0x78, 0x00, 0x5d, 0x00, 0x88}, /* 80 */
           {0xfa, 0x02, 0x10, 0x10, 0x10, 0xd7, 0x39, 0xd5, 0xd5, 0xbf,
             0xdc, 0xdf, 0xda, 0xe0, 0xc1, 0xc0, 0xbd, 0xd2, 0xd4, 0xcf,
             0x00, 0x7c, 0x00, 0x60, 0x00, 0x8c}, /* 90 */
           {0xfa, 0x02, 0x10, 0x10, 0x10, 0xdd, 0x3a, 0xe3, 0xd7, 0xc5,
             0xdd, 0xdf, 0xda, 0xdf, 0xc0, 0xbf, 0xbc, 0xd0, 0xd3, 0xcd,
             0x00, 0x81, 0x00, 0x64, 0x00, 0x92}, /* 100*/
           {0xfa, 0x02, 0x10, 0x10, 0x10, 0xe1, 0x43, 0xe2, 0xd6, 0xc5,
             0xdc, 0xde, 0xda, 0xdf, 0xbf, 0xbf, 0xbb, 0xd0, 0xd3, 0xcd,
             0x00, 0x85, 0x00, 0x67, 0x00, 0x96}, /* 110*/
           {0xfa, 0x02, 0x10, 0x10, 0x10, 0xe5, 0x48, 0xe4, 0xd5, 0xc5,
             0xdb, 0xde, 0xda, 0xdd, 0xbe, 0xbf, 0xbb, 0xd0, 0xd2, 0xcc,
             0x00, 0x88, 0x00, 0x6a, 0x00, 0x9a}, /* 120*/
           {0xfa, 0x02, 0x10, 0x10, 0x10, 0xe6, 0x4d, 0xe3, 0xd5, 0xc5,
             0xda, 0xdd, 0xda, 0xdd, 0xbe, 0xbe, 0xba, 0xce, 0xd1, 0xca,
             0x00, 0x8c, 0x00, 0x6d, 0x00, 0x9f}, /* 130*/
           {0xfa, 0x02, 0x10, 0x10, 0x10, 0xe8, 0x51, 0xe4, 0xd6, 0xc9,
             0xdb, 0xdc, 0xd9, 0xdc, 0xbe, 0xbf, 0xba, 0xcd, 0xd0, 0xc9,
             0x00, 0x90, 0x00, 0x70, 0x00, 0xa3}, /* 140*/
           {0xfa, 0x02, 0x10, 0x10, 0x10, 0xea, 0x57, 0xe9, 0xd6, 0xc9,
             0xda, 0xdd, 0xda, 0xdc, 0xbc, 0xbe, 0xb8, 0xce, 0xd0, 0xca,
             0x00, 0x92, 0x00, 0x72, 0x00, 0xa6}, /* 150*/
           {0xfa, 0x02, 0x10, 0x10, 0x10, 0xf0, 0x61, 0xee, 0xd5, 0xc9,
             0xd9, 0xdd, 0xda, 0xdb, 0xbc, 0xbe, 0xb9, 0xcc, 0xcf, 0xc8,
             0x00, 0x96, 0x00, 0x75, 0x00, 0xaa}, /* 160*/
           {0xfa, 0x02, 0x10, 0x10, 0x10, 0xf1, 0x81, 0xee, 0xd4, 0xc9,
             0xd9, 0xdd, 0xdb, 0xdb, 0xbb, 0xbd, 0xb7, 0xcc, 0xce, 0xc8,
             0x00, 0x99, 0x00, 0x78, 0x00, 0xae}, /* 170*/
           {0xfa, 0x02, 0x10, 0x10, 0x10, 0xf0, 0x8c, 0xed, 0xd5, 0xca,
             0xd8, 0xdc, 0xdb, 0xdc, 0xbb, 0xbd, 0xb7, 0xcb, 0xcd, 0xc5,
             0x00, 0x9c, 0x00, 0x7a, 0x00, 0xb2}, /* 180*/
           {0xfa, 0x02, 0x10, 0x10, 0x10, 0xed, 0x90, 0xec, 0xd4, 0xc9,
             0xd7, 0xdd, 0xdb, 0xdb, 0xba, 0xbd, 0xb7, 0xca, 0xcd, 0xc5,
             0x00, 0x9f, 0x00, 0x7c, 0x00, 0xb5}, /* 190*/
           {0xfa, 0x02, 0x10, 0x10, 0x10, 0xed, 0x8f, 0xed, 0xd2, 0xc7,
             0xd5, 0xdc, 0xda, 0xda, 0xba, 0xbb, 0xb5, 0xca, 0xcc, 0xc4,
             0x00, 0xa0, 0x00, 0x7f, 0x00, 0xbb}, /* 200*/
           {0xfa, 0x02, 0x10, 0x10, 0x10, 0xef, 0x96, 0xef, 0xd1, 0xc7,
             0xd3, 0xdb, 0xd9, 0xd9, 0xb9, 0xbb, 0xb4, 0xca, 0xcc, 0xc6,
             0x00, 0xa3, 0x00, 0x81, 0x00, 0xbd}, /* 210*/
           {0xfa, 0x02, 0x10, 0x10, 0x10, 0xed, 0x99, 0xee, 0xd3, 0xc8,
             0xd3, 0xdb, 0xd9, 0xd9, 0xb8, 0xbb, 0xb4, 0xc9, 0xcc, 0xc4,
             0x00, 0xa6, 0x00, 0x83, 0x00, 0xc1}, /* 220*/
           {0xfa, 0x02, 0x10, 0x10, 0x10, 0xee, 0xa3, 0xef, 0xd2, 0xc9,
             0xd3, 0xdb, 0xd9, 0xd9, 0xb9, 0xba, 0xb3, 0xc8, 0xcc, 0xc4,
             0x00, 0xa8, 0x00, 0x85, 0x00, 0xc4}, /* 230*/
           {0xfa, 0x02, 0x10, 0x10, 0x10, 0xed, 0xa4, 0xee, 0xd2, 0xc9,
             0xd3, 0xdc, 0xd9, 0xd9, 0xb7, 0xba, 0xb3, 0xc8, 0xcb, 0xc2,
             0x00, 0xab, 0x00, 0x87, 0x00, 0xc8}, /* 240*/
           {0xfa, 0x02, 0x10, 0x10, 0x10, 0xed, 0xa8, 0xef, 0xd2, 0xc8,
             0xd2, 0xda, 0xd9, 0xd8, 0xb7, 0xb9, 0xb2, 0xc8, 0xcb, 0xc2,
             0x00, 0xad, 0x00, 0x89, 0x00, 0xcb}, /* 250*/
           {0xfa, 0x02, 0x10, 0x10, 0x10, 0xeb, 0xb2, 0xef, 0xd2, 0xc9,
             0xd3, 0xdb, 0xda, 0xd9, 0xb6, 0xb9, 0xba, 0xc7, 0xca, 0xc1,
             0x00, 0xb0, 0x00, 0x8b, 0x00, 0xce}, /* 260*/
           {0xfa, 0x02, 0x10, 0x10, 0x10, 0xec, 0xb0, 0xef, 0xd1, 0xc9,
             0xd2, 0xdb, 0xda, 0xd9, 0xb7, 0xb9, 0xb1, 0xc6, 0xc9, 0xc0,
             0x00, 0xb2, 0x00, 0x8d, 0x00, 0xd1}, /* 270*/
           {0xfa, 0x02, 0x10, 0x10, 0x10, 0xed, 0xb6, 0xf0, 0xd1, 0xc9,
             0xd2, 0xdb, 0xda, 0xd9, 0xb6, 0xb8, 0xaf, 0xc7, 0xca, 0xc2,
             0x00, 0xb4, 0x00, 0x8f, 0x00, 0xd3}, /* 280*/
           {0xfa, 0x02, 0x10, 0x10, 0x10, 0xec, 0xb7, 0xee, 0xd0, 0xc8,
             0xd1, 0xdb, 0xda, 0xd9, 0xb6, 0xb8, 0xb0, 0xc5, 0xc9, 0xc1,
             0x00, 0xb7, 0x00, 0x91, 0x00, 0xd5}, /* 290*/
           {0xfa, 0x02, 0x10, 0x10, 0x10, 0xec, 0xb7, 0xef, 0xd1, 0xca,
             0xd1, 0xd8, 0xda, 0xd8, 0xb5, 0xb8, 0xb0, 0xc5, 0xc8, 0xbf,
             0x00, 0xb9, 0x00, 0x93, 0x00, 0xd9} /* 300 */
};
static char bl_data_new_materal[][26] = {
// w00182148:new lcd panel parameter with the change of lcd materal begin 20120120
	     {0xfa, 0x02, 0x00, 0x10, 0x48, 0xEB, 0xBC, 0xA1, 0xDC, 0xCD,
                 0xA3, 0xE4, 0xE5, 0xC3, 0xCB, 0xC9, 0xB0, 0xDA, 0xD6, 0xC2,
                 0x00, 0x65, 0x00, 0x5F, 0x00, 0x71}, /* 30 */
            {0xfa, 0x02, 0x10, 0x10, 0x49, 0xE8, 0xC6, 0x9D, 0xD7, 0xCF,
                 0xA3, 0xE0, 0xE6, 0xC8, 0xC3, 0xC6, 0xAB, 0xD5, 0xD5, 0xC1,
                 0x00, 0x6E, 0x00, 0x67, 0x00, 0x7B}, /* 40 */
            {0xfa, 0x02, 0x24, 0x00, 0x4D, 0xDA, 0xDC, 0x92, 0xCF, 0xD7,
                 0x9F, 0xDA, 0xE9, 0xCC, 0xBB, 0xC8, 0xA5, 0xCD, 0xD8, 0xBF,
                 0x00, 0x76, 0x00, 0x6E, 0x00, 0x83}, /* 50 */
           {0xfa, 0x02, 0x2E, 0x01, 0x4D, 0xD4, 0xDE, 0x93, 0xCA, 0xDB,
                 0xA5, 0xD6, 0xE6, 0xCC, 0xB4, 0xC7, 0xA4, 0xCB, 0xD5, 0xBF,
                 0x00, 0x7D, 0x00, 0x76, 0x00, 0x8B}, /* 60*/
           {0xfa, 0x02, 0x38, 0x00, 0x56, 0xc9, 0xdd, 0x94, 0xc2, 0xd9,
             0xac, 0xd3, 0xe6, 0xce, 0xae, 0xc5, 0x9d, 0xc6, 0xd4, 0xb9,
             0x00, 0x8a, 0x00, 0x82, 0x00, 0x9f}, /* 70 */
           {0xfa, 0x02, 0x3c, 0x10, 0x57, 0xc6, 0xd3, 0x90, 0xc0, 0xd7, 
             0xb1, 0xd0, 0xe2, 0xca, 0xab, 0xbf, 0x9b, 0xc4, 0xcf, 0xb8, 
             0x00, 0x90, 0x00, 0x88, 0x00, 0xa6}, /* 80 */
           {0xfa, 0x02, 0x43, 0x00, 0x57, 0xbd, 0xe1, 0x91, 0xbb, 0xdc,
             0xb3, 0xcd, 0xe3, 0xca, 0xa7, 0xc4, 0x9b, 0xc0, 0xd1, 0xb5,
             0x00, 0x95, 0x00, 0x8d, 0x00, 0xad}, /* 90 */
           {0xfa, 0x02, 0x43, 0x00, 0x58, 0xbd, 0xe2, 0x90, 0xbc, 0xde,
             0xb6, 0xcc, 0xe2, 0xc8, 0xa6, 0xc2, 0x99, 0xc0, 0xd1, 0xb4,
             0x00, 0x9a, 0x00, 0x92, 0x00, 0xb3}, /* 100*/
           {0xfa, 0x02, 0x47, 0x00, 0x57, 0xb9, 0xe3, 0x93, 0xba, 0xde,
             0xb9, 0xc9, 0xe2, 0xc9, 0xa4, 0xc1, 0x98, 0xbc, 0xd0, 0xb3,
             0x00, 0x9f, 0x00, 0x96, 0x00, 0xb9}, /* 110*/
           {0xfa, 0x02, 0x48, 0x00, 0x57, 0xb7, 0xe5, 0x95, 0xb9, 0xde,
             0xba, 0xca, 0xe2, 0xc8, 0xa2, 0xc0, 0x97, 0xbe, 0xce, 0xb5,
             0x00, 0xa1, 0x00, 0x9b, 0x00, 0xbc}, /* 120*/
           {0xfa, 0x02, 0x4a, 0x19, 0x58, 0xb6, 0xd7, 0x95, 0xb7, 0xd8,
             0xba, 0xc8, 0xdb, 0xc6, 0x9f, 0xb7, 0x96, 0xba, 0xc9, 0xb4,
             0x00, 0xaa, 0x00, 0xa0, 0x00, 0xc3}, /* 130*/
           {0xfa, 0x02, 0x4a, 0x1d, 0x57, 0xb5, 0xd4, 0x98, 0xb6, 0xd7,
             0xbb, 0xc8, 0xd9, 0xc6, 0xa0, 0xb5, 0x96, 0xb9, 0xc7, 0xb2,
             0x00, 0xac, 0x00, 0xa3, 0x00, 0xc8}, /* 140*/
           {0xfa, 0x02, 0x4c, 0x1f, 0x57, 0xb3, 0xd5, 0x9c, 0xb4, 0xd6,
             0xbb, 0xc7, 0xd8, 0xc6, 0x9e, 0xb4, 0x96, 0xb8, 0xc5, 0xaf,
             0x00, 0xb0, 0x00, 0xa7, 0x00, 0xce}, /* 150*/
           {0xfa, 0x02, 0x4c, 0x26, 0x58, 0xb5, 0xd0, 0x99, 0xb5, 0xd3,
             0xbb, 0xc7, 0xd6, 0xc5, 0x9c, 0xb0, 0x94, 0xb9, 0xc3, 0xb0,
             0x00, 0xb4, 0x00, 0xab, 0x00, 0xd2}, /* 160*/
           {0xfa, 0x02, 0x4d, 0x2a, 0x58, 0xb3, 0xce, 0x9c, 0xb4, 0xd2,
             0xbb, 0xc6, 0xd4, 0xc4, 0x9b, 0xad, 0x93, 0xb9, 0xc1, 0xb1,
             0x00, 0xb7, 0x00, 0xaf, 0x00, 0xd6}, /* 170*/
           {0xfa, 0x02, 0x4f, 0x28, 0x57, 0xb1, 0xd0, 0x9d, 0xb2, 0xd3,
             0xbb, 0xc5, 0xd5, 0xc5, 0x9b, 0xad, 0x94, 0xb5, 0xc1, 0xad,
             0x00, 0xbb, 0x00, 0xb2, 0x00, 0xdc}, /* 180*/
           {0xfa, 0x02, 0x50, 0x2e, 0x58, 0xb0, 0xcd, 0x9d, 0xb2, 0xd0,
             0xbb, 0xc4, 0xd2, 0xc3, 0x9a, 0xa9, 0x92, 0xb4, 0xc0, 0xae,
             0x00, 0xbf, 0x00, 0xb6, 0x00, 0xe0}, /* 190*/
           {0xfa, 0x02, 0x4f, 0x31, 0x57, 0xb2, 0xcb, 0xa0, 0xb3, 0xce,
             0xbb, 0xc4, 0xd1, 0xc4, 0x9b, 0xa9, 0x92, 0xb3, 0xbc, 0xad,
             0x00, 0xc3, 0x00, 0xba, 0x00, 0xe5}, /* 200*/
           {0xfa, 0x02, 0x4f, 0x34, 0x58, 0xb2, 0xc9, 0x9f, 0xb3, 0xcc,
             0xba, 0xc4, 0xcf, 0xc3, 0x9a, 0xa7, 0x91, 0xb3, 0xbb, 0xac,
             0x00, 0xc6, 0x00, 0xbd, 0x00, 0xe9}, /* 210*/
           {0xfa, 0x02, 0x4f, 0x38, 0x57, 0xb3, 0xc6, 0xa3, 0xb4, 0xca,
             0xbb, 0xc4, 0xce, 0xc3, 0x99, 0xa4, 0x92, 0xb3, 0xb9, 0xab,
             0x00, 0xc9, 0x00, 0xc0, 0x00, 0xed}, /* 220*/
           {0xfa, 0x02, 0x51, 0x38, 0x58, 0xb0, 0xc8, 0xa3, 0xb1, 0xc9,
             0xb9, 0xc3, 0xce, 0xc3, 0x98, 0xa4, 0x90, 0xb1, 0xb8, 0xab,
             0x00, 0xcd, 0x00, 0xc4, 0x00, 0xf1}, /* 230*/
           {0xfa, 0x02, 0x52, 0x39, 0x58, 0xaf, 0xc7, 0xa3, 0xb1, 0xc8,
             0xba, 0xc3, 0xcd, 0xc3, 0x97, 0xa3, 0x8f, 0xb1, 0xb7, 0xab,
             0x00, 0xcf, 0x00, 0xc7, 0x00, 0xf4}, /* 240*/
           {0xfa, 0x02, 0x53, 0x3a, 0x57, 0xae, 0xc7, 0xa7, 0xb1, 0xc8,
             0xba, 0xc2, 0xcd, 0xc2, 0x95, 0xa1, 0x90, 0xb0, 0xb7, 0xa9,
             0x00, 0xd3, 0x00, 0xca, 0x00, 0xfa}, /* 250*/
           {0xfa, 0x02, 0x52, 0x3c, 0x58, 0xb0, 0xc6, 0xa6, 0xb1, 0xc7,
             0xb9, 0xc2, 0xcb, 0xc1, 0x96, 0xa0, 0x8f, 0xb0, 0xb6, 0xa9,
             0x00, 0xd6, 0x00, 0xcd, 0x00, 0xfd}, /* 260*/
           {0xfa, 0x02, 0x53, 0x3d, 0x58, 0xae, 0xc4, 0xa6, 0xb1, 0xc5,
             0xb9, 0xc2, 0xcb, 0xc1, 0x95, 0xa0, 0x8e, 0xae, 0xb4, 0xa8,
             0x00, 0xd9, 0x00, 0xd0, 0x01, 0x01}, /* 270*/
           {0xfa, 0x02, 0x55, 0x3e, 0x58, 0xac, 0xc4, 0xa7, 0xaf, 0xc5,
             0xb8, 0xc1, 0xcb, 0xc1, 0x94, 0x9f, 0x8e, 0xac, 0xb2, 0xa7,
             0x00, 0xdd, 0x00, 0xd4, 0x01, 0x06}, /* 280*/
           {0xfa, 0x02, 0x53, 0x41, 0x5a, 0xb0, 0xc4, 0xa4, 0xb0, 0xc2,
             0xb6, 0xc2, 0xca, 0xc1, 0x94, 0x9c, 0x8c, 0xae, 0xb2, 0xa5,
             0x00, 0xdf, 0x00, 0xd6, 0x01, 0x0a}, /* 290*/
           {0xfa, 0x02, 0x51, 0x39, 0x55, 0xb0, 0xc7, 0xa0, 0xb0, 0xc5,
             0xb8, 0xc2, 0xcb, 0xc1, 0x94, 0xa0, 0x8f, 0xad, 0xb3, 0xa6,
             0x00, 0xe0, 0x00, 0xd7, 0x01, 0x08} /* 300*/
// w00182148:new lcd panel parameter with the change of lcd materal end 20120120
};
/*w00182148 add ACL 20120210 begin*/
static struct dsi_cmd_desc acl_on_cmds[] =
{
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(acl_on),acl_on},
};
static struct dsi_cmd_desc acl_off_cmds[] =
{
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(acl_off),acl_off},
};
/*w00182148 add ACL 20120210 end*/

/*add soft reset for disable disorder screen when wake up*/
/*modify wrong dtype*/
static struct dsi_cmd_desc samsung_display_on_cmds[] = 
{
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(soft_reset),soft_reset},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(etc_condition_set1_0),etc_condition_set1_0},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(etc_condition_set1_1),etc_condition_set1_1},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(etc_condition_set1_2),etc_condition_set1_2},

    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_300),gamma_set_300},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(gamma_update),gamma_update},
    
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(panel_condition_set),panel_condition_set},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(etc_condition_set2_0),etc_condition_set2_0},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(etc_condition_set2_1),etc_condition_set2_1},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(etc_condition_set2_2),etc_condition_set2_2},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(etc_condition_set2_3),etc_condition_set2_3},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(etc_condition_set2_4),etc_condition_set2_4},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(etc_condition_set2_5),etc_condition_set2_5},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(etc_condition_set2_6),etc_condition_set2_6},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(etc_condition_set2_7),etc_condition_set2_7},
    //add for sloving font edge blur by wenjuan 00182148 20120405 begin//
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(etc_condition_set2_17),etc_condition_set2_17},
	//add for sloving font edge blur by wenjuan 00182148 20120405 end//
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(etc_condition_set2_8),etc_condition_set2_8},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(etc_condition_set2_9),etc_condition_set2_9},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(etc_condition_set2_10),etc_condition_set2_10},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(etc_condition_set2_11),etc_condition_set2_11},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(etc_condition_set2_12),etc_condition_set2_12},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(etc_condition_set2_13),etc_condition_set2_13},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(etc_condition_set2_14),etc_condition_set2_14},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(etc_condition_set2_15),etc_condition_set2_15},
    {DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(etc_condition_set2_16),etc_condition_set2_16},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(mem_winodw_set1_0),mem_winodw_set1_0},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(mem_winodw_set1_1),mem_winodw_set1_1},
    
    {DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(mem_winodw_set2_0),mem_winodw_set2_0},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(mem_winodw_set2_1),mem_winodw_set2_1},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(mem_winodw_set2_2),mem_winodw_set2_2},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(mem_winodw_set2_3),mem_winodw_set2_3},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(mad_ctl),mad_ctl},
  //  {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(acl_on),acl_on},
     {DTYPE_DCS_LWRITE,1,0,0,0,sizeof(acl_setting),acl_setting},
//    {DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on),display_on},
};
static struct dsi_cmd_desc samsung_new_materal_display_on_cmds[] = 
{
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(soft_reset),soft_reset},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(etc_condition_set1_0),etc_condition_set1_0},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(etc_condition_set1_1),etc_condition_set1_1},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(etc_condition_set1_2),etc_condition_set1_2},

    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_300_new_materal),gamma_set_300_new_materal},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(gamma_update),gamma_update},
    
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(panel_condition_set),panel_condition_set},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(etc_condition_set2_0),etc_condition_set2_0},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(etc_condition_set2_1),etc_condition_set2_1},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(etc_condition_set2_2),etc_condition_set2_2},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(etc_condition_set2_3),etc_condition_set2_3},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(etc_condition_set2_4),etc_condition_set2_4},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(etc_condition_set2_5),etc_condition_set2_5},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(etc_condition_set2_6),etc_condition_set2_6},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(etc_condition_set2_7),etc_condition_set2_7},
     //add for sloving font edge blur by wenjuan 00182148 20120405 begin//
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(etc_condition_set2_17),etc_condition_set2_17},
	//add for sloving font edge blur by wenjuan 00182148 20120405 end//
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(etc_condition_set2_8),etc_condition_set2_8},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(etc_condition_set2_9),etc_condition_set2_9},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(etc_condition_set2_10),etc_condition_set2_10},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(etc_condition_set2_11),etc_condition_set2_11},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(etc_condition_set2_12),etc_condition_set2_12},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(etc_condition_set2_13),etc_condition_set2_13},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(etc_condition_set2_14),etc_condition_set2_14},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(etc_condition_set2_15_new_materal),etc_condition_set2_15_new_materal},
    {DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(etc_condition_set2_16),etc_condition_set2_16},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(mem_winodw_set1_0),mem_winodw_set1_0},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(mem_winodw_set1_1),mem_winodw_set1_1},
    
    {DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(mem_winodw_set2_0),mem_winodw_set2_0},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(mem_winodw_set2_1),mem_winodw_set2_1},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(mem_winodw_set2_2),mem_winodw_set2_2},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(mem_winodw_set2_3),mem_winodw_set2_3},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(mad_ctl),mad_ctl},
   // {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(acl_on),acl_on},
   {DTYPE_DCS_LWRITE,1,0,0,0,sizeof(acl_setting),acl_setting},
//    {DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on),display_on},
};

static struct dsi_cmd_desc samsung_display_off_cmds[] = 
{
    {DTYPE_DCS_READ, 1, 0, 1, 0, sizeof(display_off),display_off},
    {DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(enter_sleep),enter_sleep},
};
static struct dsi_cmd_desc mipi_samsung_get_pwr_state_cmd[] =
{
    {DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(get_pwr_state),get_pwr_state},
};
/*add for lcd power down timing*/
extern struct platform_device msm_mipi_dsi1_device;

static int mipi_samsung_panel_power(int on)
{
	int rc;

    mutex_lock(&power_lock);
	printk("%s: state : %d enter \n", __func__, on);
    
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
	printk("%s: state : %d out \n", __func__, on);
    mutex_unlock(&power_lock);
    
	return 0;
}

/*w00182148 add for ACL  20120207 begin */

int S6E39A0_set_ACL(struct lcd_tuning_dev *ltd, enum amoled_acl acl)
{
    struct dsi_cmd_desc disp_on_cmds[1];
    struct msm_fb_data_type *mfd = samsung_mfd;

    switch( acl)
    {
        case ACL_ON:

           // printk("########S6E39A0_set_ACL ON \n");

	     disp_on_cmds[0].dtype = DTYPE_DCS_WRITE1;
            disp_on_cmds[0].last = 1;
            disp_on_cmds[0].vc = 0;
            disp_on_cmds[0].ack = 0;
            disp_on_cmds[0].wait = 0;
            disp_on_cmds[0].dlen = sizeof(acl_on);
            disp_on_cmds[0].payload = acl_on;

			lcd_acl_state = 1;
            break ;

        case ACL_OFF:
	     disp_on_cmds[0].dtype = DTYPE_DCS_WRITE1;
            disp_on_cmds[0].last = 1;
            disp_on_cmds[0].vc = 0;
            disp_on_cmds[0].ack = 0;
            disp_on_cmds[0].wait = 0;
            disp_on_cmds[0].dlen = sizeof(acl_off);
            disp_on_cmds[0].payload = acl_off;

	    // printk("########S6E39A0_set_ACL OFF \n");
			lcd_acl_state = 0;
            break ;

    }
	mutex_lock(&mfd->dma->ov_mutex);
	mdp4_dsi_cmd_dma_busy_wait(mfd);
    mdp4_dsi_blt_dmap_busy_wait(mfd);
    mipi_dsi_mdp_busy_wait(mfd);
    mipi_dsi_cmds_tx(mfd, &samsung_tx_buf, disp_on_cmds, ARRAY_SIZE(disp_on_cmds));
    mutex_unlock(&mfd->dma->ov_mutex);

	return 0;
}
/*w00182148 add for ACL  20120207 end */
//w0018248 add for color temperature ajust begin 20120417//
#define COLOR_DEPTH	256
static int S6E39A0_set_lut(u8 mdp_gc)
{
	int top_index, i;
	u8 *r;
	u8 *g;
	u8 *b;
	r = kmalloc(COLOR_DEPTH * sizeof(u8), GFP_KERNEL);
	g = kmalloc(COLOR_DEPTH * sizeof(u8), GFP_KERNEL);
	b = kmalloc(COLOR_DEPTH * sizeof(u8), GFP_KERNEL);

	top_index = (mdp_gc - 127)*10/127;

	if(top_index <= 0)
	{
		for(i = 0;i < COLOR_DEPTH;i++)
		{
			*(r+i) = i;
			*(g+i) = i;
			*(b+i) = i * (256 + top_index*3)/256;
		}
	}
	if(top_index > 0)
	{
		for(i = 0;i < COLOR_DEPTH;i++)
		{
			*(r+i) = i * (256 - top_index*3)/256;
			*(g+i) = i * (256 - top_index*3)/256;
			*(b+i) = i;
		}
	}

	mdp_pipe_ctrl(MDP_CMD_BLOCK, MDP_BLOCK_POWER_ON, FALSE);
	for(i = 0;i < COLOR_DEPTH;i++)
	{
#ifdef CONFIG_FB_MSM_MDP40
		MDP_OUTP(MDP_BASE + 0x94800 +
#else
		MDP_OUTP(MDP_BASE + 0x93800 +
#endif
				i*4,
				((g[i] & 0xff) |
				 ((b[i] & 0xff) << 8) |
				 ((r[i] & 0xff) << 16)));
        wmb();
	}
	MDP_OUTP(MDP_BASE + 0x90070, 0x17);

	mdp_pipe_ctrl(MDP_CMD_BLOCK, MDP_BLOCK_POWER_OFF, FALSE);

	kfree(r);
	kfree(g);
	kfree(b);

	return 0;
}
//w0018248 add for color temperature ajust begin 20120417//
static int mipi_samsung_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
    
	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
	 if(!samsung_mfd)
	{
        samsung_mfd = mfd;
	}

	//mutex_lock(&mfd->dma->ov_mutex);
/*add for lcd power down timing*/
        mipi_samsung_panel_power(1);
	mipi_dsi_cmds_rx(mfd, &samsung_tx_buf, &samsung_rx_buf,
		mipi_samsung_get_pwr_state_cmd, 1);
       printk("%s:mipi_samsung_lcd_on enter data=%x\n", __func__,samsung_rx_buf.data[0]);

    if(samsung_rx_buf.data[0] & 0x04)//Display is on return
		 return 0;


	printk("%s:mipi_samsung_lcd_on enter\n", __func__);
    if(new_materal)
        mipi_dsi_cmds_tx(mfd, &samsung_tx_buf, samsung_new_materal_display_on_cmds,
        		ARRAY_SIZE(samsung_new_materal_display_on_cmds));
    else
        mipi_dsi_cmds_tx(mfd, &samsung_tx_buf, samsung_display_on_cmds,
                ARRAY_SIZE(samsung_display_on_cmds));
	/*w00182148 add ACL 20120210 begin*/
	if(lcd_acl_state)
		{
		mipi_dsi_cmds_tx(mfd, &samsung_tx_buf, acl_on_cmds,ARRAY_SIZE(acl_on_cmds));
		}
	else
		{
		mipi_dsi_cmds_tx(mfd, &samsung_tx_buf, acl_off_cmds,ARRAY_SIZE(acl_off_cmds));
		}
	/*w00182148 add ACL 20120210 end*/
	printk("%s:mipi_samsung_lcd_on out\n", __func__);
		
	//mutex_unlock(&mfd->dma->ov_mutex);
//w0018248 add for color temperature ajust begin 20120417//
S6E39A0_set_lut(179);
//w0018248 add for color temperature ajust end 20120417//


	queue_work(samsung_disp_on_wq, &disp_on_work);
    lcd_on_off = 1;
	return 0;
}

static int mipi_samsung_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	//mutex_lock(&mfd->dma->ov_mutex);
	printk("%s:mipi_samsung_lcd_off enter\n", __func__);
	mipi_dsi_cmds_tx(mfd, &samsung_tx_buf, samsung_display_off_cmds,
			ARRAY_SIZE(samsung_display_off_cmds));
	printk("%s:mipi_samsung_lcd_off out\n", __func__);
    
	//mutex_unlock(&mfd->dma->ov_mutex);

	return 0;
}

static void mipi_sumsung_lcd_backlight(struct msm_fb_data_type *mfd)
{
    int num_bl_steps,level,index,range;
    struct dsi_cmd_desc bl_cmds[2];

    struct dsi_cmd_desc display_cmd;
    level = mfd->bl_level;
    if(level==0)
    {
        display_cmd.dtype = DTYPE_DCS_WRITE;
        display_cmd.last = 1;
        display_cmd.vc = 0;
        display_cmd.ack = 0;
        display_cmd.wait = 0;
        display_cmd.dlen = sizeof(display_off);
        display_cmd.payload = display_off;
        mutex_lock(&mfd->dma->ov_mutex);
        if (ST_DSI_SUSPEND == mdp4_overlay_dsi_state_get()) {
            mutex_unlock(&mfd->dma->ov_mutex);
            return;
        }
        mdp4_dsi_cmd_dma_busy_wait(mfd);
        mdp4_dsi_blt_dmap_busy_wait(mfd);
        mipi_dsi_mdp_busy_wait(mfd);
        mipi_dsi_cmds_tx(mfd, &samsung_tx_buf, &display_cmd, 1);
        lcd_on_off = 0;
        mutex_unlock(&mfd->dma->ov_mutex);
    }
    else
    {
        if(lcd_on_off == 0)
        {
            display_cmd.dtype = DTYPE_DCS_WRITE;
            display_cmd.last = 1;
            display_cmd.vc = 0;
            display_cmd.ack = 0;
            display_cmd.wait = 0;
            display_cmd.dlen = sizeof(display_on);
            display_cmd.payload = display_on;
            mutex_lock(&mfd->dma->ov_mutex);
            if (ST_DSI_SUSPEND == mdp4_overlay_dsi_state_get()) {
                mutex_unlock(&mfd->dma->ov_mutex);
                return;
            }
            mdp4_dsi_cmd_dma_busy_wait(mfd);
            mdp4_dsi_blt_dmap_busy_wait(mfd);
            mipi_dsi_mdp_busy_wait(mfd);
            mipi_dsi_cmds_tx(mfd, &samsung_tx_buf, &display_cmd, 1);
            lcd_on_off = 1;
            mutex_unlock(&mfd->dma->ov_mutex);
        }
	if(new_materal)
		num_bl_steps = ARRAY_SIZE(bl_data_new_materal);
	else
        num_bl_steps = ARRAY_SIZE(bl_data);

        range = (301 / num_bl_steps) + 1;
        index = level / range;
        if (index >= num_bl_steps)
            index = num_bl_steps - 1;

        bl_cmds[0].dtype = DTYPE_DCS_LWRITE;
        bl_cmds[0].last = 1;
        bl_cmds[0].vc = 0;
        bl_cmds[0].ack = 0;
        bl_cmds[0].wait = 0;
        if(new_materal)
        {
            bl_cmds[0].dlen = sizeof(bl_data_new_materal[index]);
            bl_cmds[0].payload = bl_data_new_materal[index];
        }
        else
        {
            bl_cmds[0].dlen = sizeof(bl_data[index]);
            bl_cmds[0].payload = bl_data[index];
        }

        bl_cmds[1].dtype = DTYPE_DCS_WRITE1;
        bl_cmds[1].last = 1;
        bl_cmds[1].vc = 0;
        bl_cmds[1].ack = 0;
        bl_cmds[1].wait = 0;
        bl_cmds[1].dlen = sizeof(gamma_update);
        bl_cmds[1].payload = gamma_update;
        mutex_lock(&mfd->dma->ov_mutex);
        if (ST_DSI_SUSPEND == mdp4_overlay_dsi_state_get()) {
            mutex_unlock(&mfd->dma->ov_mutex);
            return;
        }
        mdp4_dsi_cmd_dma_busy_wait(mfd);
        mdp4_dsi_blt_dmap_busy_wait(mfd);
        mipi_dsi_mdp_busy_wait(mfd);
        mipi_dsi_cmds_tx(mfd, &samsung_tx_buf, bl_cmds, ARRAY_SIZE(bl_cmds));
        mutex_unlock(&mfd->dma->ov_mutex);
    }

}
static void disp_on_work_func(struct work_struct *work)
{
    struct dsi_cmd_desc disp_on_cmds[1];
    struct msm_fb_data_type *mfd = samsung_mfd;

    if(!mfd)
    {
        printk("[%s]warning:mfd is NULL pointer\n",__func__);
        return;
    }
    disp_on_cmds[0].dtype = DTYPE_DCS_WRITE;
    disp_on_cmds[0].last = 1;
    disp_on_cmds[0].vc = 0;
    disp_on_cmds[0].ack = 0;
    disp_on_cmds[0].wait = 0;
    disp_on_cmds[0].dlen = sizeof(display_on);
    disp_on_cmds[0].payload = display_on;

    mfd->pan_waiting = TRUE;
    wait_for_completion_killable(&mfd->pan_comp);

    mutex_lock(&mfd->dma->ov_mutex);
    mdp4_dsi_cmd_dma_busy_wait(mfd);
    mdp4_dsi_blt_dmap_busy_wait(mfd);
    mipi_dsi_mdp_busy_wait(mfd);
    mipi_dsi_cmds_tx(mfd, &samsung_tx_buf, disp_on_cmds, ARRAY_SIZE(disp_on_cmds));
    mutex_unlock(&mfd->dma->ov_mutex);
}

/*wenjuan 00182148 20111228 begin */
#ifdef CONFIG_FB_DYNAMIC_GAMMA

struct gamma_set_desc {
	int wait;
	int dlen;
	char *payload;
};

static char samsung_s6e39a_gamma19_set[26] = {0xfa, 0x02, 0x10, 0x10, 0x10, 0xd1, 0x34, 0xd0, 0xd1, 0xba,
             0xdc, 0xe0, 0xd9, 0xe2, 0xc2, 0xc0, 0xbf, 0xd4, 0xd5, 0xd0,0x00, 0x73, 0x00, 0x59, 0x00, 0x82};
//gamma 1.9 for new material w0018218 20120131 begin
static char samsung_s6e39a_gamma19_set_new_materal[26] = {0xfa, 0x02, 0x4f, 0x31, 0x57, 0xb5, 
                    0xcf, 0xa5, 0xbc, 0xd1, 0xc2, 0xc8, 0xd2, 0xc6, 0xa1, 0xad, 0x9a, 
                    0xbc, 0xc5, 0xb5, 0x00, 0xc3, 0x00, 0xba, 0x00, 0xe5};
//gamma 1.9 for new material w0018218 20120131 end

static struct dsi_cmd_desc s6e39a_gamma19_set[] = 
{
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(samsung_s6e39a_gamma19_set),samsung_s6e39a_gamma19_set},
};
static struct dsi_cmd_desc s6e39a_gamma19_set_new_materal[] = 
{
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(samsung_s6e39a_gamma19_set_new_materal),samsung_s6e39a_gamma19_set_new_materal},
};

//gamma2.2
/*there is 2.2 GAMA initialization sequence */
static char samsung_s6e39a_gamma22_set[26] = {0xfa, 0x02, 0x10, 0x10, 0x10, 0xec, 
                    0xb7, 0xef, 0xd1, 0xca, 0xd1, 0xdb, 0xda, 0xd8, 0xb5, 0xb8, 0xb0, 0xc5, 0xc8, 0xbf, 0x00, 0xb9, 0x00, 0x93, 0x00, 0xd9};

static struct dsi_cmd_desc s6e39a_gamma22_set[] = 
{
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(samsung_s6e39a_gamma22_set),samsung_s6e39a_gamma22_set},
};


//gamma update,only after gamma update ,gamma set can be function.
static char samsung_s6e39a_gamma_update[2] = {0xfa, 0x03};

static struct dsi_cmd_desc s6e39a_gamma_update[] = 
{
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(samsung_s6e39a_gamma_update),samsung_s6e39a_gamma_update},
};




int process_lcd_table(struct msm_fb_data_type *mfd,enum lcd_gamma gamma_mode)
{
	mutex_lock(&mfd->dma->ov_mutex);
    if (mdp4_overlay_dsi_state_get() == ST_DSI_SUSPEND) {
		mutex_unlock(&mfd->dma->ov_mutex);
		return 1;
    }
	mdp4_dsi_cmd_dma_busy_wait(mfd);
    mdp4_dsi_blt_dmap_busy_wait(mfd);
    mipi_dsi_mdp_busy_wait(mfd);
    if (gamma_mode == 2)
    	{
        	if(new_materal)
                mipi_dsi_cmds_tx(mfd, &samsung_tx_buf, s6e39a_gamma19_set_new_materal, ARRAY_SIZE(s6e39a_gamma19_set_new_materal));
            else
            	mipi_dsi_cmds_tx(mfd, &samsung_tx_buf, s6e39a_gamma19_set, ARRAY_SIZE(s6e39a_gamma19_set));
    	}
    
	if (gamma_mode == 1)
		{
		mipi_dsi_cmds_tx(mfd, &samsung_tx_buf, s6e39a_gamma22_set, ARRAY_SIZE(s6e39a_gamma22_set));
		}
    mipi_dsi_cmds_tx(mfd, &samsung_tx_buf, s6e39a_gamma_update, ARRAY_SIZE(s6e39a_gamma_update));
	mutex_unlock(&mfd->dma->ov_mutex);
    
    return 0;
}


/* add the function  to set different gama by different mode */
int S6E39A0_set_dynamic_gamma(struct lcd_tuning_dev *ltd, enum lcd_gamma gamma)
{
    int ret = 0;
		
    switch(gamma)
    {
        case GAMMA22:
            printk("########mipi_samsung_dynamic_gamma 2.2 setting \n");
			mipi_sumsung_lcd_backlight(samsung_mfd);
            break ;
        case GAMMA19:
            ret = process_lcd_table(samsung_mfd,gamma);
			printk("########mipi_samsung_dynamic_gamma 1.9 setting \n");
            if(ret)
            break ;
        default:
            ret= -1;
            break;
    }
    printk("%s: change gamma mode to %d\n",__func__,gamma);
	return ret;
}
#endif
/* wenjuan 00182148 20111228 end >*/

/* wenjuan 00182148 20111228 begin*/
static struct lcd_tuning_ops sp_tuning_ops = 
{
	/*w00182148 add for ACL  2012/01/13 begin */
	.set_acl = S6E39A0_set_ACL,
	/*w00182148 add for ACL  2012/01/13 end */
    #ifdef CONFIG_FB_DYNAMIC_GAMMA
    .set_gamma = S6E39A0_set_dynamic_gamma,
    #endif    
};
/*wenjuan 00182148 20111228 end */
/*add for lcd power down timing*/
static int __devinit mipi_samsung_lcd_probe(struct platform_device *pdev)
{
    int rc;
    /* wenjuan 00182148 20111228 begin*/
	struct lcd_tuning_dev *ltd;
	struct lcd_properities lcd_props;
	int r;
    /*wenjuan 00182148 20111228 end */
	
	if (pdev->id == 0) {
		mipi_samsung_pdata = pdev->dev.platform_data;
		return 0;
	}
	printk("%s:mipi_samsung_lcd_probe\n", __func__);

	 /*w00182148 add for ACL 20120207 begin */
	 lcd_props.default_aclsetting = ACL_OFF;
	 /*w00182148 add for ACL 20120207 end */

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
    rc = regulator_set_voltage(reg_vci, 3000000, 3000000);
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
	
    /*  wenjuan 00182148 20111228 begin*/
    lcd_props.type = OLED;
    lcd_props.default_gamma = GAMMA22;
    ltd = lcd_tuning_dev_register(&lcd_props, &sp_tuning_ops, NULL);
    if (IS_ERR(ltd)) 
	{
    	r = PTR_ERR(ltd);
    }
	 /*  wenjuan 00182148 20111228 begin*/
	return 0;
}

static void mipi_samsung_lcd_shutdown(struct platform_device *pdev)
{
    mipi_samsung_panel_power(0);
}

static struct platform_driver this_driver = {
	.probe  = mipi_samsung_lcd_probe,
    .shutdown  = mipi_samsung_lcd_shutdown,
	.driver = {
		.name   = "mipi_samsung_qhd",
	},
};

static struct msm_fb_panel_data samsung_panel_data = {
	.on		= mipi_samsung_lcd_on,
	.off		= mipi_samsung_lcd_off,
	.set_backlight  = mipi_sumsung_lcd_backlight,
};

int mipi_samsung_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_samsung_qhd", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	samsung_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &samsung_panel_data,
		sizeof(samsung_panel_data));
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

extern struct mdp_csc_cfg csc_matrix[3];

/* add lcd id self adapt */
static int __init mipi_video_samsung_qhd_init(void)
{
	int ret;

/* optmize lcd self adapt function */
	if(0 != lcd_detect_panel("samsung_amoled_mipi_cmd_qhd"))
	{
	    if(0 == lcd_detect_panel("samsung_amoled_mipi_cmd_qhd_new_materal"))
            new_materal = true;
        else
            return 0;
        
	}

    memcpy(&csc_matrix[2],&samsung_mdp_csc_cfg,sizeof(struct mdp_csc_cfg));
	printk("%s:mipi_video_samsung_qhd_pt_init\n", __func__);
	pinfo.xres = 540;
	pinfo.yres = 960; 

	pinfo.mipi.xres_pad = 0;
	pinfo.mipi.yres_pad = 0;
	pinfo.type = MIPI_CMD_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.lcdc.h_back_porch = 64;
	pinfo.lcdc.h_front_porch = 64;
	pinfo.lcdc.h_pulse_width = 16;
	pinfo.lcdc.v_back_porch = 8;
	pinfo.lcdc.v_front_porch = 4;
	pinfo.lcdc.v_pulse_width = 1;
	pinfo.lcdc.border_clr = 0;	/* blk */
	pinfo.lcdc.underflow_clr = 0xff;	/* blue */
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 301;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
/* merge patch from qualcomm to solve samsung freeze issue */
	pinfo.lcd.v_back_porch = 2;
	pinfo.lcd.v_front_porch = 77;
	pinfo.lcd.v_pulse_width = 1;

	/*over*/
	pinfo.lcd.vsync_enable = TRUE;
	pinfo.lcd.hw_vsync_mode = TRUE;
	pinfo.lcd.refx100 = 6000;
	pinfo.clk_rate = 460000000;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.t_clk_post = 0x19;
	pinfo.mipi.t_clk_pre = 0x2e;
	pinfo.mipi.stream = 0; /* dma_p */
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_NONE;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.frame_rate = 60;
	pinfo.mipi.dsi_phy_db = &dsi_cmd_mode_phy_db;
	pinfo.mipi.data_lane0=TRUE;
	pinfo.mipi.data_lane1=TRUE;
	pinfo.mipi.te_sel = 1; /* TE from vsycn gpio */
	pinfo.mipi.interleave_max = 1;
	pinfo.mipi.insert_dcs_cmd = TRUE;
	pinfo.mipi.wr_mem_continue = 0x3c;
	pinfo.mipi.wr_mem_start = 0x2c;
	pinfo.mipi.tx_eot_append = TRUE;

	/*test*/
	pinfo.mipi.mode=DSI_CMD_MODE;
	pinfo.mipi.dst_format=DSI_CMD_DST_FORMAT_RGB888;
	pinfo.mipi.rgb_swap=DSI_RGB_SWAP_RGB;

	ret = mipi_samsung_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_WVGA_PT);

	if (ret)
	{
		printk(KERN_ERR "%s: failed to register device!\n", __func__);
		goto device_reg_failed;
	}
    mipi_dsi_buf_alloc(&samsung_tx_buf, DSI_BUF_SIZE);
    mipi_dsi_buf_alloc(&samsung_rx_buf, DSI_BUF_SIZE);
    
    ret = platform_driver_register(&this_driver);
	if (ret)
    {   
		printk(KERN_ERR "%s: failed to register driver!\n", __func__);
        goto driver_reg_failed;
    }

	samsung_disp_on_wq = create_singlethread_workqueue("disp_on_wq");
    if(!samsung_disp_on_wq)
    {
		ret = -ENOMEM;
		goto err_create_workqueue_failed;
    }
	INIT_WORK(&disp_on_work, disp_on_work_func);
    
/*add for lcd power down timing*/
    reset_gpio = PM8921_GPIO_PM_TO_SYS(43);
    ret = gpio_request(reset_gpio, "disp_rst_n");
    if (ret) {
        printk(KERN_ERR "request gpio 43 failed, ret=%d\n", ret);
        goto reset_gpio_request_failed;
    }
    mutex_init(&power_lock);
    return 0;

reset_gpio_request_failed:
    destroy_workqueue(samsung_disp_on_wq);
err_create_workqueue_failed:
    platform_driver_unregister(&this_driver);
driver_reg_failed:
    kfree(samsung_tx_buf.data);
    memset(&samsung_tx_buf,0,sizeof(struct dsi_buf));
    kfree(samsung_rx_buf.data);
    memset(&samsung_rx_buf,0,sizeof(struct dsi_buf));
device_reg_failed:
	return ret;
}

module_init(mipi_video_samsung_qhd_init);
