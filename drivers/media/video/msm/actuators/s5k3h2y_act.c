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
#include <mach/gpio.h>

#define AD5823_TOTAL_STEPS_NEAR_TO_FAR_MAX 37
#define AD5823_REG_SW_RESET 0x01
#define AD5823_REG_MODE 0x02
#define AD5823_REG_VCM_MOVE_TIME 0x03
#define AD5823_REG_VCM_CODE_MSB 0x04
#define AD5823_REG_VCM_CODE_LSB 0x05
#define AD5823_REG_VCM_THRESHOLD_MSB 0x06
#define AD5823_REG_VCM_THRESHOLD_LSB 0x07

DEFINE_MUTEX(ad5823_act_mutex);
static struct msm_actuator_ctrl_t ad5823_act_t;

static int32_t ad5823_wrapper_i2c_write(struct msm_actuator_ctrl_t *a_ctrl,
	int16_t next_lens_position, void *hwparams)
{
	uint16_t ad5823_ring_ctrl = (*(uint16_t *)hwparams) << 2;
	uint16_t msb = ad5823_ring_ctrl | ((next_lens_position & 0x0300) >> 8);
	uint16_t lsb = next_lens_position & 0x00FF;
	CDBG("%s next_lens_position:%d, msb:0x%x lsb:0x%x\n",
		__func__, next_lens_position, msb, lsb);

	msm_camera_i2c_write(&a_ctrl->i2c_client, AD5823_REG_VCM_CODE_MSB,
		msb, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(&a_ctrl->i2c_client, AD5823_REG_VCM_CODE_LSB,
		lsb, MSM_CAMERA_I2C_BYTE_DATA);
	return next_lens_position;
}

static uint16_t ad5823_hw_param[] = {
	0x0,
	0x1,
	0x1,
	0x1,
};

static uint16_t ad5823_scenario[] = {
	/* MOVE_NEAR dir*/
	AD5823_TOTAL_STEPS_NEAR_TO_FAR_MAX,
};

static uint16_t ad5823_inf_scenario[] = {
	/* MOVE_NEAR dir*/
	1,
	AD5823_TOTAL_STEPS_NEAR_TO_FAR_MAX,
};

static struct region_params_t ad5823_regions[] = {
	/* step_bound[0] - macro side boundary
	 * step_bound[1] - infinity side boundary
	 */
	/* Region 1 */
	{
		.step_bound = {3, 0},
		.code_per_step = 45,
	},
	/* Region 2 */
	{
		.step_bound = {8, 3},
		.code_per_step = 9,
	},
	/* Region 3 */
	{
		.step_bound = {AD5823_TOTAL_STEPS_NEAR_TO_FAR_MAX, 8},
		.code_per_step = 9,
	}
};

static struct damping_params_t ad5823_damping_reg1[] = {
	/* scenario 1 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 200,
		.hw_params = &ad5823_hw_param[0],
	},
};

static struct damping_params_t ad5823_damping_reg2[] = {
	/* scenario 1 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 6000,
		.hw_params = &ad5823_hw_param[1],
	},
};

static struct damping_params_t ad5823_damping_reg3[] = {
	/* scenario 1 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 6000,
		.hw_params = &ad5823_hw_param[1],
	},
};

static struct damping_params_t ad5823_damping_inf_reg1[] = {
	/* scenario 1 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 200,
		.hw_params = &ad5823_hw_param[2],
	},
	/* scenario 2 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 200,
		.hw_params = &ad5823_hw_param[2],
	},
};

static struct damping_params_t ad5823_damping_inf_reg2[] = {
	/* scenario 1 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 6000,
		.hw_params = &ad5823_hw_param[1],
	},
	/* scenario 2 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 15000,
		.hw_params = &ad5823_hw_param[3],
	},
};

static struct damping_params_t ad5823_damping_inf_reg3[] = {
	/* scenario 1 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 6000,
		.hw_params = &ad5823_hw_param[1],
	},
	/* scenario 2 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 15000,
		.hw_params = &ad5823_hw_param[1],
	},
};

static struct damping_t ad5823_region_damp_params[] = {
	/* Region 1 */
	{
		.ringing_params = ad5823_damping_reg1,
	},
	/* Region 2 */
	{
		.ringing_params = ad5823_damping_reg2,
	},
	/* Region 3 */
	{
		.ringing_params = ad5823_damping_reg3,
	},
};

static struct damping_t ad5823_region_damp_inf_params[] = {
	/* Region 1 */
	{
		.ringing_params = ad5823_damping_inf_reg1,
	},
	/* Region 2 */
	{
		.ringing_params = ad5823_damping_inf_reg2,
	},
	/* Region 3 */
	{
		.ringing_params = ad5823_damping_inf_reg3,
	},
};

static int32_t ad5823_set_params(struct msm_actuator_ctrl_t *a_ctrl)
{
	uint16_t val =
		ad5823_regions[0].code_per_step * ad5823_regions[0].step_bound[0];
	CDBG("%s: Threshold:%d\n", __func__, val);
	msm_camera_i2c_write(&a_ctrl->i2c_client, AD5823_REG_MODE,
		0x02, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(&a_ctrl->i2c_client, AD5823_REG_VCM_MOVE_TIME,
		0x12, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(&a_ctrl->i2c_client, AD5823_REG_VCM_THRESHOLD_MSB,
		(val & 0x300) >> 8, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(&a_ctrl->i2c_client, AD5823_REG_VCM_THRESHOLD_LSB,
		(val & 0xFF), MSM_CAMERA_I2C_BYTE_DATA);
	return 0;
}

#if 0
int32_t ad5823_power_up(void *adata)
{
	int32_t rc = 0;
	const struct msm_actuator_info *data =
		(struct msm_actuator_info *)adata;
	CDBG("%s: %d\n", __func__, __LINE__);
	rc = gpio_request(data->vcm_pwd, "S5K3H2");
	if (!rc) {
		CDBG("%s: reset actuator\n", __func__);
		gpio_direction_output(data->vcm_pwd, 0);
		usleep_range(1000, 2000);
		gpio_set_value_cansleep(data->vcm_pwd, 1);
		usleep_range(4000, 5000);
	} else {
		CDBG("%s: gpio request fail", __func__);
	}
	return rc;
}
#endif

static int32_t ad5823_power_down(void *adata)
{
//	const struct msm_actuator_info *data =
//		(struct msm_actuator_info *)adata;
//	CDBG("%s: %d\n", __func__, __LINE__);
	ad5823_act_t.func_tbl.actuator_set_default_focus(&ad5823_act_t);
	msleep(100);
	msm_camera_i2c_write(&ad5823_act_t.i2c_client,
		AD5823_REG_SW_RESET, 1, MSM_CAMERA_I2C_BYTE_DATA);
//	gpio_set_value_cansleep(data->vcm_pwd, 0);
//	usleep_range(1000, 2000);
//	gpio_free(data->vcm_pwd);

	return 0;
}

static const struct i2c_device_id ad5823_act_i2c_id[] = {
	{"s5k3h2y_act", (kernel_ulong_t)&ad5823_act_t},
	{ }
};

static int ad5823_act_config(
	void __user *argp)
{
	CDBG("%s called\n", __func__);
	return (int) msm_actuator_config(&ad5823_act_t, argp);
}

static int ad5823_i2c_add_driver_table(
	void)
{
	CDBG("%s called\n", __func__);
	return (int) msm_actuator_init_table(&ad5823_act_t);
}

static struct i2c_driver ad5823_act_i2c_driver = {
	.id_table = ad5823_act_i2c_id,
	.probe  = msm_actuator_i2c_probe,
	.remove = __exit_p(ad5823_act_i2c_remove),
	.driver = {
		.name = "s5k3h2y_act",
	},
};

static int __init ad5823_i2c_add_driver(
	void)
{
	CDBG("%s called\n", __func__);
	return i2c_add_driver(ad5823_act_t.i2c_driver);
}

static struct v4l2_subdev_core_ops ad5823_act_subdev_core_ops;

static struct v4l2_subdev_ops ad5823_act_subdev_ops = {
	.core = &ad5823_act_subdev_core_ops,
};

static int32_t ad5823_act_probe(
	void *board_info,
	void *sdev)
{
	CDBG("%s called\n", __func__);
	//ad5823_actuator_debug_init(&ad5823_act_t);

	return (int) msm_actuator_create_subdevice(&ad5823_act_t,
		(struct i2c_board_info const *)board_info,
		(struct v4l2_subdev *)sdev);
}

static struct msm_actuator_ctrl_t ad5823_act_t = {
	.i2c_driver = &ad5823_act_i2c_driver,
	.i2c_addr = 0x18,
	.act_v4l2_subdev_ops = &ad5823_act_subdev_ops,
	.actuator_ext_ctrl = {
		.a_init_table = ad5823_i2c_add_driver_table,
		.a_create_subdevice = ad5823_act_probe,
		.a_config = ad5823_act_config,
//		.a_power_up = ad5823_power_up,
		.a_power_down = ad5823_power_down,
	},

	.i2c_client = {
		.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
	},
	.set_info = {
		.total_steps = AD5823_TOTAL_STEPS_NEAR_TO_FAR_MAX,
	},

	.curr_step_pos = 0,
	.curr_region_index = 0,
	.initial_code = 0,
	.actuator_mutex = &ad5823_act_mutex,

	/* Initialize scenario */
	.ringing_scenario[MOVE_NEAR] = ad5823_scenario,
	.scenario_size[MOVE_NEAR] = ARRAY_SIZE(ad5823_scenario),
	.ringing_scenario[MOVE_FAR] = ad5823_inf_scenario,
	.scenario_size[MOVE_FAR] = ARRAY_SIZE(ad5823_inf_scenario),

	/* Initialize region params */
	.region_params = ad5823_regions,
	.region_size = ARRAY_SIZE(ad5823_regions),

	/* Initialize damping params */
	.damping[MOVE_NEAR] = ad5823_region_damp_params,
	.damping[MOVE_FAR] = ad5823_region_damp_inf_params,

	.func_tbl = {
		.actuator_set_params = ad5823_set_params,
		.actuator_init_table = msm_actuator_init_table,
		.actuator_move_focus = msm_actuator_move_focus,
		.actuator_write_focus = msm_actuator_write_focus,
		.actuator_i2c_write = ad5823_wrapper_i2c_write,
		.actuator_set_default_focus = msm_actuator_set_default_focus,
	},
//add exif information start
	.get_info = {
		.focal_length_num = 4068,
		.focal_length_den = 1000,
		.f_number_num = 24,
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
};

subsys_initcall(ad5823_i2c_add_driver);
MODULE_DESCRIPTION("AD5823 actuator");
MODULE_LICENSE("GPL v2");
