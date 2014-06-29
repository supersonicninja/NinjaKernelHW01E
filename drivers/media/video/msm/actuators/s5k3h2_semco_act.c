/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
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

#include "msm_actuator.h"
#include <linux/debugfs.h>

#define S5K3H2_SEMCO_TOTAL_STEPS_NEAR_TO_FAR_MAX 41
//extern struct sensor_calib_data s5k3h2_semco_calib_data;

DEFINE_MUTEX(s5k3h2_semco_act_mutex);
static int s5k3h2_semco_actuator_debug_init(void);
static struct msm_actuator_ctrl_t s5k3h2_semco_act_t;

static int32_t s5k3h2_semco_wrapper_i2c_write(struct msm_actuator_ctrl_t *a_ctrl,
	int16_t next_lens_position, void *params)
{
	uint16_t msb = 0, lsb = 0;
	msb = (next_lens_position >> 4) & 0x3F;
	lsb = (next_lens_position << 4) & 0xF0;
	lsb |= (*(uint8_t *)params);
	CDBG("%s: Actuator MSB:0x%x, LSB:0x%x\n", __func__, msb, lsb);
	msm_camera_i2c_write(&a_ctrl->i2c_client,
		msb, lsb, MSM_CAMERA_I2C_BYTE_DATA);
	return next_lens_position;
}

static uint8_t s5k3h2_semco_hw_params[] = {
	0xF,
};

static uint16_t s5k3h2_semco_macro_scenario[] = {
	/* MOVE_NEAR dir*/
	S5K3H2_SEMCO_TOTAL_STEPS_NEAR_TO_FAR_MAX,
};

static uint16_t s5k3h2_semco_inf_scenario[] = {
	/* MOVE_FAR dir */
	8,
	22,
	S5K3H2_SEMCO_TOTAL_STEPS_NEAR_TO_FAR_MAX,
};

static struct region_params_t s5k3h2_semco_regions[] = {
	/* step_bound[0] - macro side boundary
	 * step_bound[1] - infinity side boundary
	 */
	/* Region 1 */
	{
		.step_bound = {2, 0},
		.code_per_step = 70,
	},
	/* Region 2 */
	{
		.step_bound = {S5K3H2_SEMCO_TOTAL_STEPS_NEAR_TO_FAR_MAX, 2},
		.code_per_step = 8,
	}
};

static struct damping_params_t s5k3h2_semco_macro_reg1_damping[] = {
	/* MOVE_NEAR Dir */
	/* Scene 1 => Damping params */
	{
		.damping_step = 0x1FF,
		.damping_delay = 1500,
		.hw_params = &s5k3h2_semco_hw_params[0],
	},
};

static struct damping_params_t s5k3h2_semco_macro_reg2_damping[] = {
	/* MOVE_NEAR Dir */
	/* Scene 1 => Damping params */
	{
		.damping_step = 0x1FF,
		.damping_delay = 4500,
		.hw_params = &s5k3h2_semco_hw_params[0],
	},
};

static struct damping_params_t s5k3h2_semco_inf_reg1_damping[] = {
	/* MOVE_FAR Dir */
	/* Scene 1 => Damping params */
	{
		.damping_step = 0x1FF,
		.damping_delay = 1500,
		.hw_params = &s5k3h2_semco_hw_params[0],
	},
	/* Scene 2 => Damping params */
	{
		.damping_step = 0x1FF,
		.damping_delay = 1500,
		.hw_params = &s5k3h2_semco_hw_params[0],
	},
	/* Scene 3 => Damping params */
	{
		.damping_step = 0x1FF,
		.damping_delay = 1500,
		.hw_params = &s5k3h2_semco_hw_params[0],
	},
};

static struct damping_params_t s5k3h2_semco_inf_reg2_damping[] = {
	/* MOVE_FAR Dir */
	/* Scene 1 => Damping params */
	{
		.damping_step = 0x1FF,
		.damping_delay = 4500,
		.hw_params = &s5k3h2_semco_hw_params[0],
	},
	/* Scene 2 => Damping params */
	{
		.damping_step = 0x1FF,
		.damping_delay = 12000,
		.hw_params = &s5k3h2_semco_hw_params[0],
	},
	/* Scene 3 => Damping params */
	{
		.damping_step = 0x1FF,
		.damping_delay = 15000,
		.hw_params = &s5k3h2_semco_hw_params[0],
	},
};

static struct damping_t s5k3h2_semco_macro_regions[] = {
	/* MOVE_NEAR dir */
	/* Region 1 */
	{
		.ringing_params = s5k3h2_semco_macro_reg1_damping,
	},
	/* Region 2 */
	{
		.ringing_params = s5k3h2_semco_macro_reg2_damping,
	},
};

static struct damping_t s5k3h2_semco_inf_regions[] = {
	/* MOVE_FAR dir */
	/* Region 1 */
	{
		.ringing_params = s5k3h2_semco_inf_reg1_damping,
	},
	/* Region 2 */
	{
		.ringing_params = s5k3h2_semco_inf_reg2_damping,
	},
};


static int32_t s5k3h2_semco_set_params(struct msm_actuator_ctrl_t *a_ctrl)
{

	#if 0
	printk("enter  %s, s5k3h2_semco_otp_af[0] is %d\n", __func__, s5k3h2_semco_calib_data.af_start_code);
	

	/*add for af otp*/
	if((s5k3h2_semco_calib_data.af_start_code != 0)&&(s5k3h2_semco_calib_data.af_max_code!= 0))   //判断是否读到otp 数值
	{
		printk(" enter %s, s5k3h2_semco_regions[0].code_per_step is %d, af start is %d, max is %d\n",
									__func__, 
									s5k3h2_semco_regions[0].code_per_step,
									s5k3h2_semco_calib_data.af_start_code,
									s5k3h2_semco_calib_data.af_max_code);
		s5k3h2_semco_regions[0].code_per_step = s5k3h2_semco_calib_data.af_start_code/s5k3h2_semco_regions[0].step_bound[0];
		s5k3h2_semco_regions[1].code_per_step = (s5k3h2_semco_calib_data.af_max_code- s5k3h2_semco_calib_data.af_start_code)/
											  (s5k3h2_semco_regions[1].step_bound[0] - s5k3h2_semco_regions[0].step_bound[0]);
		printk("after subtraction, code_per_step is %d\n", 
												s5k3h2_semco_regions[0].code_per_step);
	}

	#endif
	return 0;
}

static const struct i2c_device_id s5k3h2_semco_act_i2c_id[] = {
	{"s5k3h2_semco_act", (kernel_ulong_t)&s5k3h2_semco_act_t},
	{ }
};

static int s5k3h2_semco_act_config(
	void __user *argp)
{
	CDBG("%s called\n", __func__);
	return (int) msm_actuator_config(&s5k3h2_semco_act_t, argp);
}

static int s5k3h2_semco_i2c_add_driver_table(
	void)
{
	CDBG("%s called\n", __func__);
	return (int) msm_actuator_init_table(&s5k3h2_semco_act_t);
}

static struct i2c_driver s5k3h2_semco_act_i2c_driver = {
	.id_table = s5k3h2_semco_act_i2c_id,
	.probe  = msm_actuator_i2c_probe,
	.remove = __exit_p(s5k3h2_semco_act_i2c_remove),
	.driver = {
		.name = "s5k3h2_semco_act",
	},
};

static int32_t s5k3h2_semco_power_down(void *data)
{
	s5k3h2_semco_act_t.func_tbl.actuator_set_default_focus(&s5k3h2_semco_act_t);
	msleep(40);
	msm_camera_i2c_write(&s5k3h2_semco_act_t.i2c_client,
		0x00, 0x00, MSM_CAMERA_I2C_BYTE_DATA);
	return 0;
}
static int __init s5k3h2_semco_i2c_add_driver(
	void)
{
	CDBG("%s called\n", __func__);
	return i2c_add_driver(s5k3h2_semco_act_t.i2c_driver);
}

static struct v4l2_subdev_core_ops s5k3h2_semco_act_subdev_core_ops;

static struct v4l2_subdev_ops s5k3h2_semco_act_subdev_ops = {
	.core = &s5k3h2_semco_act_subdev_core_ops,
};

static int32_t s5k3h2_semco_act_probe(
	void *board_info,
	void *sdev)
{
	s5k3h2_semco_actuator_debug_init();

	return (int) msm_actuator_create_subdevice(&s5k3h2_semco_act_t,
		(struct i2c_board_info const *)board_info,
		(struct v4l2_subdev *)sdev);
}

static struct msm_actuator_ctrl_t s5k3h2_semco_act_t = {
	.i2c_driver = &s5k3h2_semco_act_i2c_driver,
	.i2c_addr = 0x18,
	.act_v4l2_subdev_ops = &s5k3h2_semco_act_subdev_ops,
	.actuator_ext_ctrl = {
		.a_init_table = s5k3h2_semco_i2c_add_driver_table,
		.a_create_subdevice = s5k3h2_semco_act_probe,
		.a_config = s5k3h2_semco_act_config,
		.a_power_down = s5k3h2_semco_power_down,
	},

	.i2c_client = {
		.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
	},

	.set_info = {
		.total_steps = S5K3H2_SEMCO_TOTAL_STEPS_NEAR_TO_FAR_MAX,
	},

	.curr_step_pos = 0,
	.curr_region_index = 0,
	.initial_code = 0,
	.actuator_mutex = &s5k3h2_semco_act_mutex,
//add exif information start
	.get_info = {
		.focal_length_num = 379,
		.focal_length_den = 100,
		.f_number_num = 22,
		.f_number_den = 10,
		.f_pix_num = 14,
		.f_pix_den = 10,
		.total_f_dist_num = 1784,
		.total_f_dist_den = 10,
		.hor_view_angle_num = 548,
		.hor_view_angle_den = 10,
		.ver_view_angle_num = 425,
		.ver_view_angle_den = 10,
	},
//add exif information end

	/* Initialize scenario */
	.ringing_scenario[MOVE_NEAR] = s5k3h2_semco_macro_scenario,
	.scenario_size[MOVE_NEAR] = ARRAY_SIZE(s5k3h2_semco_macro_scenario),
	.ringing_scenario[MOVE_FAR] = s5k3h2_semco_inf_scenario,
	.scenario_size[MOVE_FAR] = ARRAY_SIZE(s5k3h2_semco_inf_scenario),

	/* Initialize region params */
	.region_params = s5k3h2_semco_regions,
	.region_size = ARRAY_SIZE(s5k3h2_semco_regions),

	/* Initialize damping params */
	.damping[MOVE_NEAR] = s5k3h2_semco_macro_regions,
	.damping[MOVE_FAR] = s5k3h2_semco_inf_regions,

	.func_tbl = {
		.actuator_set_params = s5k3h2_semco_set_params,
		.actuator_init_focus = NULL,
		.actuator_init_table = msm_actuator_init_table,
		.actuator_move_focus = msm_actuator_move_focus,
		.actuator_write_focus = msm_actuator_write_focus,
		.actuator_i2c_write = s5k3h2_semco_wrapper_i2c_write,
		.actuator_set_default_focus = msm_actuator_set_default_focus,
	},

};

static int s5k3h2_semco_actuator_set_delay(void *data, u64 val)
{
	s5k3h2_semco_inf_reg2_damping[2].damping_delay = val;
	return 0;
}

static int s5k3h2_semco_actuator_get_delay(void *data, u64 *val)
{
	*val = s5k3h2_semco_inf_reg2_damping[2].damping_delay;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(s5k3h2_semco_delay,
	s5k3h2_semco_actuator_get_delay,
	s5k3h2_semco_actuator_set_delay,
	"%llu\n");

static int s5k3h2_semco_actuator_set_jumpparam(void *data, u64 val)
{
	s5k3h2_semco_inf_reg2_damping[2].damping_step = val & 0xFFF;
	return 0;
}

static int s5k3h2_semco_actuator_get_jumpparam(void *data, u64 *val)
{
	*val = s5k3h2_semco_inf_reg2_damping[2].damping_step;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(s5k3h2_semco_jumpparam,
	s5k3h2_semco_actuator_get_jumpparam,
	s5k3h2_semco_actuator_set_jumpparam,
	"%llu\n");

static int s5k3h2_semco_actuator_set_hwparam(void *data, u64 val)
{
//	s5k3h2_semco_hw_params[1] = val & 0xFF;
	s5k3h2_semco_hw_params[(val & 0x1FF)>>8] = val & 0xFF;

	return 0;
}

static int s5k3h2_semco_actuator_get_hwparam(void *data, u64 *val)
{
	*val = s5k3h2_semco_hw_params[1];
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(s5k3h2_semco_hwparam,
	s5k3h2_semco_actuator_get_hwparam,
	s5k3h2_semco_actuator_set_hwparam,
	"%llu\n");

static int s5k3h2_semco_actuator_debug_init(void)
{
	struct dentry *debugfs_base = debugfs_create_dir("s5k3h2_semco_actuator", NULL);
	if (!debugfs_base)
		return -ENOMEM;

	if (!debugfs_create_file("s5k3h2_semco_delay",
		S_IRUGO | S_IWUSR, debugfs_base, NULL, &s5k3h2_semco_delay))
		return -ENOMEM;

	if (!debugfs_create_file("s5k3h2_semco_jumpparam",
		S_IRUGO | S_IWUSR, debugfs_base, NULL, &s5k3h2_semco_jumpparam))
		return -ENOMEM;

	if (!debugfs_create_file("s5k3h2_semco_hwparam",
		S_IRUGO | S_IWUSR, debugfs_base, NULL, &s5k3h2_semco_hwparam))
		return -ENOMEM;

	return 0;
}
subsys_initcall(s5k3h2_semco_i2c_add_driver);
MODULE_DESCRIPTION("s5k3h2_semco actuator");
MODULE_LICENSE("GPL v2");
