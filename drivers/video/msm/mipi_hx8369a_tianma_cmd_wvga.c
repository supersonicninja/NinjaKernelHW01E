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
#include <linux/gpio.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mdp4.h"
#include <linux/pwm.h>

//#define MIPI_hx8369_TIANMA_PWM_PMIC
#define MIPI_hx8369_TIANMA_PWM_FREQ_HZ 300
#define MIPI_hx8369_TIANMA_PWM_PERIOD_USEC (USEC_PER_SEC / MIPI_hx8369_TIANMA_PWM_FREQ_HZ)
#define MIPI_hx8369_TIANMA_PWM_LEVEL 100
#define MIPI_hx8369_TIANMA_PWM_DUTY_LEVEL \
	(MIPI_hx8369_TIANMA_PWM_PERIOD_USEC / MIPI_hx8369_TIANMA_PWM_LEVEL)
/*add for lcd power down timing*/
#define PM8921_GPIO_BASE		NR_GPIO_IRQS
#define PM8921_GPIO_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_GPIO_BASE)

static struct msm_panel_info pinfo;
static int ch_used[3];
static struct dsi_buf hx8369_tianma_tx_buf;
static struct dsi_buf hx8369_tianma_rx_buf;
static struct mipi_dsi_panel_platform_data *mipi_hx8369_tianma_pdata;
static struct msm_fb_data_type *hx8369_tianma_mfd = NULL;
static struct workqueue_struct *hx8369_tianma_disp_on_wq;
static struct work_struct disp_on_work;
static struct pwm_device *bl_lpm;
static bool panel_power_on = false;
static struct mutex power_lock;
static int reset_gpio;
static struct regulator *reg_vddio, *reg_vci;

static struct mipi_dsi_phy_ctrl dsi_cmd_mode_phy_db = {
	/* regulator */
	{0x03, 0x0a, 0x04, 0x00, 0x20},
	/* timing */
	{0x66, 0x26, 0x12, 0x00, 0x14, 0x89, 0x1e, 0x8a,
		0x14, 0x03, 0x04, 0xa0},
	/* phy ctrl */
	{0x5f, 0x00, 0x00, 0x10},
	/* strength */
	{0xff, 0x00, 0x06, 0x00},
	/* pll control */
	{0x0, 0x1d, 0x1, 0x1a, 0x00, 0x50, 0x48, 0x63,
		0x41, 0x0f, 0x07,
		0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01 },
};
static char hx8369_softreset[1]={0x01};
static char hx8369_setextc[4] = {0xb9, 0xff, 0x83,0x69};
static char hx8369_setpower[20] = {0xb1, 0x01, 0x00,0x34,
	0x0a, 0x00, 0x0f, 0x0f,
	0x25, 0x2d, 0x3f, 0x3f,
	0x01, 0x0b, 0x01, 0xe6,
	0xe6, 0xe6, 0xe6, 0xe6};
static char hx8369_set_disp[16] = {0xb2, 0x00, 0x20,0x05,
	0x05, 0x70, 0x00, 0xff,
	0x00, 0x00, 0x00, 0x00,
	0x03, 0x03, 0x00, 0x01,};
static char hx8369_set_cyc[6] = {0xb4, 0x02, 0x18, 0x78,
	0x06, 0x02};
static char hx8369_set_vcom[3] = {0xb6, 0x42, 0x42};
static char hx8369_set_osc[3] = {0xb0, 0x01 ,0x09};
static char hx8369_set_tear_on[2] = {0x35, 0x00};

static char hx8369_set_scan_line[3] = {0x44,0x01,0x90};
static char hx8369_set_address_mode[2] = {0x36, 0xd0};
static char hx8369_set_gip[27] = {0xd5, 0x00, 0x01, 0x03,
	0x27, 0x01, 0x04, 0x00,
	0x67, 0x11, 0x13, 0x00,
	0x00, 0x60, 0x24, 0x71,
	0x35, 0x00, 0x00, 0x73,
	0x15, 0x62, 0x04, 0x07,
	0x0f, 0x04, 0x04};
static char hx8369_set_gamma[35] = {0xe0, 0x0a, 0x18, 0x1e,
	0x39, 0x3f, 0x3f, 0x2f,
	0x4d, 0x08, 0x0d, 0x0f,
	0x13, 0x16, 0x13, 0x14,
	0x14, 0x14, 0x0a, 0x18,
	0x1e, 0x39, 0x3f, 0x3f,
	0x2f, 0x4d, 0x08, 0x0d,
	0x0f, 0x13, 0x16,0x13,
	0x14, 0x14, 0x14};
static char hx8369_set_pixel_formate[2] = {0x3a, 0x77};
static char hx8369_set_mipi[14] = {0xba, 0x00, 0xa0, 0xc6,
	0x00, 0x0a, 0x00, 0x10,
	0x30, 0x6f, 0x02, 0x11,
	0x18, 0x40};
static char hx8369_enter_sleep[1] = {0x10};
static char hx8369_exit_sleep[1] = {0x11};
static char hx8369_display_on[1] = {0x29};
static char hx8369_memory_start[1] = {0x2c};
static char hx8369_set_brightness[2]={0x51, 0x80};
static char hx8369_ctl_display[2]={0x53, 0x24};
static char hx8369_adaptive_brightness_ctl[2] ={0x55, 0x00};
static char hx8369_sel_pwmclk[8]={0xc9, 0x3e, 0x00, 0x00,
	0x01, 0x0f, 0x02, 0x1e};
static struct dsi_cmd_desc hx8369_tianma_display_on_cmds[] =
{
	{DTYPE_DCS_WRITE,  1, 0, 0, 10, sizeof(hx8369_softreset),hx8369_softreset},
	{DTYPE_GEN_LWRITE,  1, 0, 0, 0, sizeof(hx8369_setextc),hx8369_setextc},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8369_setpower),hx8369_setpower},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8369_set_disp),hx8369_set_disp},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8369_set_cyc),hx8369_set_cyc},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8369_set_vcom),hx8369_set_vcom},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8369_set_osc),hx8369_set_osc},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8369_set_tear_on),hx8369_set_tear_on},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_set_scan_line),hx8369_set_scan_line},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8369_set_address_mode),hx8369_set_address_mode},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8369_set_gip),hx8369_set_gip},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8369_set_gamma),hx8369_set_gamma},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(hx8369_set_pixel_formate),hx8369_set_pixel_formate},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8369_set_mipi),hx8369_set_mipi},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(hx8369_exit_sleep),hx8369_exit_sleep},
	//{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(hx8369_display_on),hx8369_display_on},
	{DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(hx8369_memory_start),hx8369_memory_start},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8369_set_brightness), hx8369_set_brightness},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8369_ctl_display), hx8369_ctl_display},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8369_adaptive_brightness_ctl), hx8369_adaptive_brightness_ctl},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8369_sel_pwmclk), hx8369_sel_pwmclk},

};

static struct dsi_cmd_desc hx8369_tianma_display_off_cmds[] =
{
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(hx8369_enter_sleep),hx8369_enter_sleep},
};
#ifndef MIPI_hx8369_TIANMA_PWM_PMIC
static struct dsi_cmd_desc hx8369_tianma_backlight_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8369_set_brightness), hx8369_set_brightness},
};
#endif
extern struct platform_device msm_mipi_dsi1_device;

static int mipi_hx8369_tianma_panel_power(int on)
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
		msleep(30);
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

static int mipi_hx8369_tianma_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
    mipi_hx8369_tianma_panel_power(1);
	//printk("%s:mipi_hx8369_tianma_lcd_on enter\n", __func__);
	mipi_dsi_cmds_tx(mfd, &hx8369_tianma_tx_buf, hx8369_tianma_display_on_cmds,
			ARRAY_SIZE(hx8369_tianma_display_on_cmds));
	//printk("%s:mipi_hx8369_tianma_lcd_on out\n", __func__);

	if(!hx8369_tianma_mfd)
	{
		hx8369_tianma_mfd = mfd;
	}

	queue_work(hx8369_tianma_disp_on_wq, &disp_on_work);
	return 0;
}

static int mipi_hx8369_tianma_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	printk("%s:mipi_hx8369_tianma_lcd_off enter\n", __func__);
	mipi_dsi_cmds_tx(mfd, &hx8369_tianma_tx_buf, hx8369_tianma_display_off_cmds,
			ARRAY_SIZE(hx8369_tianma_display_off_cmds));
	printk("%s:mipi_hx8369_tianma_lcd_off out\n", __func__);

	return 0;
}

static void mipi_hx8369_tianma_lcd_backlight(struct msm_fb_data_type *mfd)
{
#ifdef MIPI_hx8369_TIANMA_PWM_PMIC
	int ret;

	mfd->bl_level = mfd->bl_level*17;
	if (bl_lpm) {
		ret = pwm_config(bl_lpm, MIPI_hx8369_TIANMA_PWM_DUTY_LEVEL *
				mfd->bl_level, MIPI_hx8369_TIANMA_PWM_PERIOD_USEC);
		if (ret) {
			pr_err("pwm_config on lpm failed %d\n", ret);
			return;
		}
		if (mfd->bl_level) {
			ret = pwm_enable(bl_lpm);
			if (ret)
				pr_err("pwm enable/disable on lpm failed"
						"for bl %d\n",  mfd->bl_level);
		} else {
			pwm_disable(bl_lpm);
		}
	}
#else

	// printk("mipi_hx8369_tianma_lcd_backlight %d\n", mfd->bl_level);
	hx8369_set_brightness[1] = mfd->bl_level * 9;
	mutex_lock(&mfd->dma->ov_mutex);
	if (mdp4_overlay_dsi_state_get() == ST_DSI_SUSPEND) {
		mutex_unlock(&mfd->dma->ov_mutex);
		return;
	}

	mdp4_dsi_cmd_dma_busy_wait(mfd);
	mdp4_dsi_blt_dmap_busy_wait(mfd);
	mipi_dsi_mdp_busy_wait(mfd);
	mipi_dsi_cmds_tx(mfd, &hx8369_tianma_tx_buf, hx8369_tianma_backlight_cmds, ARRAY_SIZE(hx8369_tianma_backlight_cmds));
	mutex_unlock(&mfd->dma->ov_mutex);


#endif
}

static void disp_on_work_func(struct work_struct *work)
{
	struct dsi_cmd_desc disp_on_cmds[1];
	struct msm_fb_data_type *mfd = hx8369_tianma_mfd;
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
	disp_on_cmds[0].dlen = sizeof(hx8369_display_on);
	disp_on_cmds[0].payload = hx8369_display_on;

	mfd->pan_waiting = TRUE;
	wait_for_completion_killable(&mfd->pan_comp);
	mutex_lock(&mfd->dma->ov_mutex);
	mdp4_dsi_cmd_dma_busy_wait(mfd);
	mdp4_dsi_blt_dmap_busy_wait(mfd);
	mipi_dsi_mdp_busy_wait(mfd);
	mipi_dsi_cmds_tx(mfd, &hx8369_tianma_tx_buf, disp_on_cmds, ARRAY_SIZE(disp_on_cmds));
	mutex_unlock(&mfd->dma->ov_mutex);
}

static int __devinit mipi_hx8369_tianma_lcd_probe(struct platform_device *pdev)
{
	int rc;
	if (pdev->id == 0) {
		mipi_hx8369_tianma_pdata = pdev->dev.platform_data;
		return 0;
	}
	if (mipi_hx8369_tianma_pdata != NULL)
		bl_lpm = pwm_request(mipi_hx8369_tianma_pdata->gpio[0],
				"backlight");

	if (bl_lpm == NULL || IS_ERR(bl_lpm)) {
		pr_err("%s pwm_request() failed\n", __func__);
		bl_lpm = NULL;
	}
	pr_debug("bl_lpm = %p lpm = %d\n", bl_lpm,
			mipi_hx8369_tianma_pdata->gpio[0]);

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

	return 0;
}

static void mipi_hx8369_tianma_lcd_shutdown(struct platform_device *pdev)
{
	mipi_hx8369_tianma_panel_power(0);
}

static struct platform_driver this_driver = {
	.probe  = mipi_hx8369_tianma_lcd_probe,
	.shutdown  = mipi_hx8369_tianma_lcd_shutdown,
	.driver = {
		.name   = "mipi_hx8369_tianma_wvga",
	},
};

static struct msm_fb_panel_data hx8369_tianma_panel_data = {
	.on		= mipi_hx8369_tianma_lcd_on,
	.off		= mipi_hx8369_tianma_lcd_off,
	.set_backlight  = mipi_hx8369_tianma_lcd_backlight,
};

int mipi_hx8369_tianma_device_register(struct msm_panel_info *pinfo,
		u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_hx8369_tianma_wvga", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	hx8369_tianma_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &hx8369_tianma_panel_data,
			sizeof(hx8369_tianma_panel_data));
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

static int __init mipi_cmd_hx8369_tianma_wvga_init(void)
{
	int ret;
	if(0 != lcd_detect_panel("hx8369_tianma_mipi_cmd_wvga"))
	{
		return 0;
	}
	printk("%s:mipi_cmd_hx8369_tianma_wvga_pt_init\n", __func__);
	pinfo.xres = 480;
	pinfo.yres = 800;

	pinfo.mipi.xres_pad = 0;
	pinfo.mipi.yres_pad = 0;
	pinfo.type = MIPI_CMD_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.lcdc.h_back_porch = 10;
	pinfo.lcdc.h_front_porch = 10;
	pinfo.lcdc.h_pulse_width = 10;
	pinfo.lcdc.v_back_porch = 2;
	pinfo.lcdc.v_front_porch = 2;
	pinfo.lcdc.v_pulse_width = 5;
	pinfo.lcdc.border_clr = 0;	/* blk */
	pinfo.lcdc.underflow_clr = 0xff;	/* blue */
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 15;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
	pinfo.lcd.v_back_porch = 2;
	pinfo.lcd.v_front_porch = 2;
	pinfo.lcd.v_pulse_width = 5;

	/*over*/
	pinfo.lcd.vsync_enable = TRUE;
	pinfo.lcd.hw_vsync_mode = TRUE;
	pinfo.lcd.refx100 = 6000;
	pinfo.clk_rate = 480000000;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.t_clk_post = 0xb0;
	pinfo.mipi.t_clk_pre = 0x2f;
	pinfo.mipi.stream = 0; /* dma_p */
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;
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

	ret = mipi_hx8369_tianma_device_register(&pinfo, MIPI_DSI_PRIM,
			MIPI_DSI_PANEL_WVGA_PT);

	if (ret)
	{
		printk(KERN_ERR "%s: failed to register device!\n", __func__);
		goto device_reg_failed;
	}
	mipi_dsi_buf_alloc(&hx8369_tianma_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&hx8369_tianma_rx_buf, DSI_BUF_SIZE);

	ret = platform_driver_register(&this_driver);
	if (ret)
	{
		printk(KERN_ERR "%s: failed to register driver!\n", __func__);
		goto driver_reg_failed;
	}

	hx8369_tianma_disp_on_wq = create_singlethread_workqueue("disp_on_wq");
	if(!hx8369_tianma_disp_on_wq)
	{
		ret = -ENOMEM;
		goto err_create_workqueue_failed;
	}
	INIT_WORK(&disp_on_work, disp_on_work_func);

	reset_gpio = PM8921_GPIO_PM_TO_SYS(43);
	ret = gpio_request(reset_gpio, "disp_rst_n");
	if (ret) {
		printk(KERN_ERR "request gpio 43 failed, ret=%d\n", ret);
		goto reset_gpio_request_failed;
	}
	mutex_init(&power_lock);

	return 0;
reset_gpio_request_failed:
	destroy_workqueue(hx8369_tianma_disp_on_wq);
err_create_workqueue_failed:
	platform_driver_unregister(&this_driver);
driver_reg_failed:
	kfree(hx8369_tianma_tx_buf.data);
	memset(&hx8369_tianma_tx_buf,0,sizeof(struct dsi_buf));
	kfree(hx8369_tianma_rx_buf.data);
	memset(&hx8369_tianma_rx_buf,0,sizeof(struct dsi_buf));
device_reg_failed:
	return ret;
}

module_init(mipi_cmd_hx8369_tianma_wvga_init);
