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
/* create driver */
#include <hsad/config_interface.h>
/*add for lcd power down timing*/
#include <linux/gpio.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mdp4.h"
#include <linux/pwm.h>

//#define MIPI_R61408_CMI_PWM_PMIC
#define MIPI_R61408_CMI_PWM_FREQ_HZ 300
#define MIPI_R61408_CMI_PWM_PERIOD_USEC (USEC_PER_SEC / MIPI_R61408_CMI_PWM_FREQ_HZ)
#define MIPI_R61408_CMI_PWM_LEVEL 100
#define MIPI_R61408_CMI_PWM_DUTY_LEVEL \
	(MIPI_R61408_CMI_PWM_PERIOD_USEC / MIPI_R61408_CMI_PWM_LEVEL)
/*add for lcd power down timing*/
#define PM8921_GPIO_BASE		NR_GPIO_IRQS
#define PM8921_GPIO_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_GPIO_BASE)

static struct msm_panel_info pinfo;
static int ch_used[3];
static struct dsi_buf r61408_cmi_tx_buf;
static struct dsi_buf r61408_cmi_rx_buf;
static struct mipi_dsi_panel_platform_data *mipi_r61408_cmi_pdata;
static struct msm_fb_data_type *r61408_cmi_mfd = NULL;
static struct workqueue_struct *r61408_cmi_disp_on_wq;
static struct work_struct disp_on_work;
static struct pwm_device *bl_lpm;
/*add for lcd power down timing*/
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
static char r61408_soft_reset[2] = {0x01, 0x00};
static char r61408_cmd_access_protect[2] = {0xb0, 0x04};
static char r61408_frame_mem_access[3] = {0xb3, 0x02, 0x00};
static char r61408_dsi_ctl[3] = {0xb6, 0x52, 0x83};
static char r61408_bl_ctl_1[21] = {0xb8, 0x00, 0x09, 0x09,
                                 0xff, 0xff, 0xd8, 0xd8,
                                 0x02, 0x18, 0x10, 0x10,
                                 0x37, 0x5a, 0x87, 0xbe,
                                 0xff, 0x00, 0x00, 0x00,
                                 0x00};
static char r61408_bl_ctl_2[5] = {0xb9, 0x00, 0xff, 0x02, 0x08};
static char r61408_panel_drv_set_2[16] = {0xc1, 0x42, 0x31, 0x04,
                                          0x26, 0x26, 0x32, 0x12,
                                          0x28, 0x0e, 0x14, 0xa5,
                                          0x0f, 0x58, 0x21, 0x01};
static char r61408_v_timing_set[7] = {0xc2, 0x08, 0x06, 0x06, 0x01, 0x03, 0x00};
#if 0
static char r61408_gamma22_set_a[25] = {0xc8, 0x02, 0x17, 0x1d,
                                      0x29, 0x36, 0x50, 0x33,
                                      0x21, 0x16, 0x11, 0x08,
                                      0x02, 0x02, 0x17, 0x1d,
                                      0x29, 0x36, 0x50, 0x33,
                                      0x21, 0x16, 0x11, 0x08,
                                      0x02};
static char r61408_gamma22_set_b[25] = {0xc9, 0x02, 0x17, 0x1d,
                                      0x29, 0x36, 0x50, 0x33,
                                      0x21, 0x16, 0x11, 0x08,
                                      0x02, 0x02, 0x17, 0x1d,
                                      0x29, 0x36, 0x50, 0x33,
                                      0x21, 0x16, 0x11, 0x08,
                                      0x02};
static char r61408_gamma22_set_c[25] = {0xca, 0x02, 0x17, 0x1d,
                                      0x29, 0x36, 0x50, 0x33,
                                      0x21, 0x16, 0x11, 0x08,
                                      0x02, 0x02, 0x17, 0x1d,
                                      0x29, 0x36, 0x50, 0x33,
                                      0x21, 0x16, 0x11, 0x08,
                                      0x02};
#endif
static char r61408_gamma_set_a[25] = {0xc8, 0x02, 0x17, 0x1d,
                                      0x29, 0x39, 0x54, 0x30,
                                      0x20, 0x17, 0x13, 0x08,
                                      0x02, 0x02, 0x17, 0x1d,
                                      0x29, 0x39, 0x54, 0x30,
                                      0x20, 0x17, 0x13, 0x08,
                                      0x02};
static char r61408_gamma_set_b[25] = {0xc9, 0x02, 0x17, 0x1d,
                                      0x29, 0x39, 0x54, 0x30,
                                      0x20, 0x17, 0x13, 0x08,
                                      0x02, 0x02, 0x17, 0x1d,
                                      0x29, 0x39, 0x54, 0x30,
                                      0x20, 0x17, 0x13, 0x08,
                                      0x02};
static char r61408_gamma_set_c[25] = {0xca, 0x02, 0x17, 0x1d,
                                      0x29, 0x39, 0x54, 0x30,
                                      0x20, 0x17, 0x13, 0x08,
                                      0x02, 0x02, 0x17, 0x1d,
                                      0x29, 0x39, 0x54, 0x30,
                                      0x20, 0x17, 0x13, 0x08,
                                      0x02};
static char r61408_pwr_set_chg_bump[17] = {0xd0, 0x29, 0x03, 0xcc,
                                           0xa6, 0x0c, 0x53, 0x20,
                                           0x10, 0x01, 0x00, 0x01,
                                           0x01, 0x00, 0x03, 0x01,
                                           0x00};
static char r61408_pwr_set_switch_regulator[8] = {0xd1, 0x18, 0x0c, 0x23,
                                                  0x03, 0x75, 0x02, 0x50};
static char r61408_pwr_set_internal_mode[2] = {0xd3, 0x33};
static char r61408_vpnlvl_set[3] = {0xd5, 0x2a, 0x2a};
static char r61408_vcomdc_set[3] = {0xde, 0x01, 0x43};
static char r61408_vdc_sel_set[2] = {0xfa, 0x01};
static char r61408_misc_set_0[2] = {0xd6, 0x28};
static char r61408_set_col_addr[5] = {0x2a, 0x00, 0x00, 0x01, 0xdf};
static char r61408_set_page_addr[5] = {0x2b, 0x00, 0x00, 0x03, 0x1f};
static char r61408_set_tear_on[5] = {0x35, 0x00, 0x44, 0x01, 0x90};
static char r61408_set_addr_mode[2] = {0x36, 0x00};
static char r61408_set_pix_mode[2] = {0x3a, 0x77};
static char r61408_exit_sleep[2] = {0x11, 0x00};
static char r61408_display_on[2] = {0x29, 0x00};
//static char r61408_write_mem_start[2] = {0x2c, 0x00};
static char r61408_enter_sleep[2] = {0x10, 0x00};

static struct dsi_cmd_desc r61408_cmi_display_on_cmds[] =
{
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(r61408_soft_reset),r61408_soft_reset},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(r61408_cmd_access_protect),r61408_cmd_access_protect},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_frame_mem_access),r61408_frame_mem_access},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_dsi_ctl),r61408_dsi_ctl},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_bl_ctl_1),r61408_bl_ctl_1},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_bl_ctl_2),r61408_bl_ctl_2},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_panel_drv_set_2),r61408_panel_drv_set_2},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_v_timing_set),r61408_v_timing_set},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_gamma_set_a),r61408_gamma_set_a},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_gamma_set_b),r61408_gamma_set_b},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_gamma_set_c),r61408_gamma_set_c},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_pwr_set_chg_bump),r61408_pwr_set_chg_bump},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_pwr_set_switch_regulator),r61408_pwr_set_switch_regulator},
    {DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(r61408_pwr_set_internal_mode),r61408_pwr_set_internal_mode},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_vpnlvl_set),r61408_vpnlvl_set},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_vcomdc_set),r61408_vcomdc_set},
    {DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(r61408_vdc_sel_set),r61408_vdc_sel_set},
    {DTYPE_GEN_WRITE2, 1, 0, 0, 100, sizeof(r61408_misc_set_0),r61408_misc_set_0},

    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r61408_set_col_addr),r61408_set_col_addr},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r61408_set_page_addr),r61408_set_page_addr},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r61408_set_tear_on),r61408_set_tear_on},
    {DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(r61408_set_addr_mode),r61408_set_addr_mode},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(r61408_set_pix_mode),r61408_set_pix_mode},
    {DTYPE_DCS_WRITE, 1, 0, 0, 200, sizeof(r61408_exit_sleep),r61408_exit_sleep},
   // {DTYPE_DCS_WRITE, 1, 0, 0, 20, sizeof(r61408_display_on),r61408_display_on},
   // {DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(r61408_write_mem_start),r61408_write_mem_start},
};

static struct dsi_cmd_desc r61408_cmi_display_off_cmds[] =
{
    {DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(r61408_enter_sleep),r61408_enter_sleep},
};
#ifndef MIPI_R61408_CMI_PWM_PMIC
 static struct dsi_cmd_desc r61408_cmi_backlight_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_bl_ctl_2), r61408_bl_ctl_2},
 };
#endif

/*add for lcd power down timing*/
extern struct platform_device msm_mipi_dsi1_device;

static int mipi_r61408_cmi_panel_power(int on)
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
/*add for lcd power down timing*/
static int mipi_r61408_cmi_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
    mipi_r61408_cmi_panel_power(1);

	printk("%s:mipi_r61408_cmi_lcd_on enter\n", __func__);
	mipi_dsi_cmds_tx(mfd, &r61408_cmi_tx_buf, r61408_cmi_display_on_cmds,
			ARRAY_SIZE(r61408_cmi_display_on_cmds));
	printk("%s:mipi_r61408_cmi_lcd_on out\n", __func__);


    if(!r61408_cmi_mfd)
    {
        r61408_cmi_mfd = mfd;
    }

	queue_work(r61408_cmi_disp_on_wq, &disp_on_work);
	return 0;
}

static int mipi_r61408_cmi_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	printk("%s:mipi_r61408_cmi_lcd_off enter\n", __func__);
	mipi_dsi_cmds_tx(mfd, &r61408_cmi_tx_buf, r61408_cmi_display_off_cmds,
			ARRAY_SIZE(r61408_cmi_display_off_cmds));
	printk("%s:mipi_r61408_cmi_lcd_off out\n", __func__);

	return 0;
}

static void mipi_r61408_cmi_lcd_backlight(struct msm_fb_data_type *mfd)
{
#ifdef MIPI_R61408_CMI_PWM_PMIC
    int ret;

    mfd->bl_level = mfd->bl_level*17;
    if (bl_lpm) {
        ret = pwm_config(bl_lpm, MIPI_R61408_CMI_PWM_DUTY_LEVEL *
            mfd->bl_level, MIPI_R61408_CMI_PWM_PERIOD_USEC);
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
   //  printk("mipi_r61408_cmi_lcd_backlight %d\n", mfd->bl_level);
    r61408_bl_ctl_2[2] = mfd->bl_level * 17;
    mutex_lock(&mfd->dma->ov_mutex);
	if (mdp4_overlay_dsi_state_get() == ST_DSI_SUSPEND) {
		mutex_unlock(&mfd->dma->ov_mutex);
		return;
    }
    mdp4_dsi_cmd_dma_busy_wait(mfd);
    mdp4_dsi_blt_dmap_busy_wait(mfd);
    mipi_dsi_mdp_busy_wait(mfd);
    mipi_dsi_cmds_tx(mfd, &r61408_cmi_tx_buf, r61408_cmi_backlight_cmds, ARRAY_SIZE(r61408_cmi_backlight_cmds));
    mutex_unlock(&mfd->dma->ov_mutex);

    // mipi_dsi_cmds_tx(mfd, &r61408_cmi_tx_buf, r61408_cmi_backlight_cmds,
    //        ARRAY_SIZE(r61408_cmi_backlight_cmds));
#endif
}

static void disp_on_work_func(struct work_struct *work)
{
    struct dsi_cmd_desc disp_on_cmds[1];
    struct msm_fb_data_type *mfd = r61408_cmi_mfd;

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
    disp_on_cmds[0].dlen = sizeof(r61408_display_on);
    disp_on_cmds[0].payload = r61408_display_on;

    mfd->pan_waiting = TRUE;
    wait_for_completion_killable(&mfd->pan_comp);

    mutex_lock(&mfd->dma->ov_mutex);
    mdp4_dsi_cmd_dma_busy_wait(mfd);
    mdp4_dsi_blt_dmap_busy_wait(mfd);
    mipi_dsi_mdp_busy_wait(mfd);
    mipi_dsi_cmds_tx(mfd, &r61408_cmi_tx_buf, disp_on_cmds, ARRAY_SIZE(disp_on_cmds));
    mutex_unlock(&mfd->dma->ov_mutex);
}

/*add for lcd power down timing*/
static int __devinit mipi_r61408_cmi_lcd_probe(struct platform_device *pdev)
{
    int rc;

	if (pdev->id == 0) {
		mipi_r61408_cmi_pdata = pdev->dev.platform_data;
		return 0;
	}
	if (mipi_r61408_cmi_pdata != NULL)
		bl_lpm = pwm_request(mipi_r61408_cmi_pdata->gpio[0],
			"backlight");

	if (bl_lpm == NULL || IS_ERR(bl_lpm)) {
		pr_err("%s pwm_request() failed\n", __func__);
		bl_lpm = NULL;
	}
	pr_debug("bl_lpm = %p lpm = %d\n", bl_lpm,
		mipi_r61408_cmi_pdata->gpio[0]);

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

static void mipi_r61408_cmi_lcd_shutdown(struct platform_device *pdev)
{
    mipi_r61408_cmi_panel_power(0);
}

static struct platform_driver this_driver = {
	.probe  = mipi_r61408_cmi_lcd_probe,
    .shutdown  = mipi_r61408_cmi_lcd_shutdown,
	.driver = {
		.name   = "mipi_r61408_cmi_wvga",
	},
};

static struct msm_fb_panel_data r61408_cmi_panel_data = {
	.on		= mipi_r61408_cmi_lcd_on,
	.off		= mipi_r61408_cmi_lcd_off,
	.set_backlight  = mipi_r61408_cmi_lcd_backlight,
};

int mipi_r61408_cmi_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_r61408_cmi_wvga", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	r61408_cmi_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &r61408_cmi_panel_data,
		sizeof(r61408_cmi_panel_data));
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

static int __init mipi_cmd_r61408_cmi_wvga_init(void)
{
	int ret;

	if(0 != lcd_detect_panel("r61408_cmi_amoled_mipi_cmd_wvga"))
	{
        return 0;
	}
	printk("%s:mipi_cmd_r61408_cmi_wvga_pt_init\n", __func__);
	pinfo.xres = 480;
	pinfo.yres = 800;

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
	pinfo.bl_max = 15;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
	pinfo.lcd.v_back_porch = 8;
	pinfo.lcd.v_front_porch = 4;
	pinfo.lcd.v_pulse_width = 1;

	/*over*/
	pinfo.lcd.vsync_enable = TRUE;
	pinfo.lcd.hw_vsync_mode = TRUE;
	pinfo.lcd.refx100 = 6000;
	pinfo.clk_rate = 480000000;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.t_clk_post = 0x19;
	pinfo.mipi.t_clk_pre = 0x2e;
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

	ret = mipi_r61408_cmi_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_WVGA_PT);

	if (ret)
	{
		printk(KERN_ERR "%s: failed to register device!\n", __func__);
		goto device_reg_failed;
	}
    mipi_dsi_buf_alloc(&r61408_cmi_tx_buf, DSI_BUF_SIZE);
    mipi_dsi_buf_alloc(&r61408_cmi_rx_buf, DSI_BUF_SIZE);

    ret = platform_driver_register(&this_driver);
	if (ret)
    {
		printk(KERN_ERR "%s: failed to register driver!\n", __func__);
        goto driver_reg_failed;
    }

	r61408_cmi_disp_on_wq = create_singlethread_workqueue("disp_on_wq");
    if(!r61408_cmi_disp_on_wq)
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
    destroy_workqueue(r61408_cmi_disp_on_wq);
err_create_workqueue_failed:
    platform_driver_unregister(&this_driver);
driver_reg_failed:
    kfree(r61408_cmi_tx_buf.data);
    memset(&r61408_cmi_tx_buf,0,sizeof(struct dsi_buf));
    kfree(r61408_cmi_rx_buf.data);
    memset(&r61408_cmi_rx_buf,0,sizeof(struct dsi_buf));
device_reg_failed:
	return ret;
}

module_init(mipi_cmd_r61408_cmi_wvga_init);
