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
#include "msm_actuator.h"
#include <linux/debugfs.h>

#define IMX105_TOTAL_STEPS_NEAR_TO_FAR_MAX 37

extern unsigned short imx105_sunny_otp_af[2];   //from sensor driver, [0] is start code, [1] is max code

/*#undef CDBG
#define CDBG(fmt,args...)  pr_err(fmt,##args)*/
DEFINE_MUTEX(imx105_act_mutex);
static int imx105_actuator_debug_init(void);
static struct msm_actuator_ctrl_t imx105_act_t;

static int32_t imx105_wrapper_i2c_write(struct msm_actuator_ctrl_t *a_ctrl,
	int16_t next_lens_position, void *params)
{
	uint16_t msb = 0, lsb = 0;

	msb = (next_lens_position >> 4) & 0x3F;
	lsb = (next_lens_position << 4) & 0xF0;
       lsb |= (*(uint8_t *)params);
	CDBG("%s: Actuator next_lens_position =%d\n", __func__, next_lens_position);

	msm_camera_i2c_write(&a_ctrl->i2c_client,
		msb, lsb, MSM_CAMERA_I2C_BYTE_DATA);
	return next_lens_position;
}

static uint16_t imx105_hw_params[] = {
	//0xD0D0,//step 36, TSRC:11010 S[3:0]=0000 MCLK[1:0]=0x01
       0xB, // 0xA 41msec, 0xB 74msec
       0x6,
       0x7,
       0x6,
       0x7, 
};


static uint16_t imx105_macro_scenario[] = {
	/* MOVE_NEAR dir*/
                4,
	IMX105_TOTAL_STEPS_NEAR_TO_FAR_MAX,

};

static uint16_t imx105_inf_scenario[] = {
	/* MOVE_FAR dir */
                8,
                22,
	IMX105_TOTAL_STEPS_NEAR_TO_FAR_MAX,
};

static struct region_params_t imx105_regions[] = {
	/* step_bound[0] - macro side boundary
	 * step_bound[1] - infinity side boundary
	 */

	/* Region 1 */
	{
		.step_bound = {2, 0},
		.code_per_step = 80,
	},
	/* Region 1 */
	{
		.step_bound = {IMX105_TOTAL_STEPS_NEAR_TO_FAR_MAX, 2}, // JP
		.code_per_step = 0x7,
	},
};


static struct damping_params_t imx105_macro_reg1_damping[] = {
	/* MOVE_NEAR Dir */
       /* Scene 1 => Damping params */
       {
              .damping_step = 0xFF,
              .damping_delay = 1500,
              .hw_params = &imx105_hw_params[0],
       },
       /* Scene 2 => Damping params */
       {
              .damping_step = 0xFF,
              .damping_delay = 1500,
              .hw_params = &imx105_hw_params[0],
       },

};

static struct damping_params_t imx105_macro_reg2_damping[] = {
	/* MOVE_NEAR Dir */
       /* Scene 1 => Damping params */
       {
              .damping_step = 0xFF,
              .damping_delay = 4500,
              .hw_params = &imx105_hw_params[4],
       },
       /* Scene 2 => Damping params */
       {
              .damping_step = 0xFF,
              .damping_delay = 4500,
              .hw_params = &imx105_hw_params[3],
       },
};

static struct damping_params_t imx105_inf_reg1_damping[] = {
	/* MOVE_FAR Dir */
       /* Scene 1 => Damping params */
       {
              .damping_step = 0xFF,
              .damping_delay = 450,
              .hw_params = &imx105_hw_params[0],
       },
       /* Scene 2 => Damping params */
       {
              .damping_step = 0xFF,
              .damping_delay = 450,
              .hw_params = &imx105_hw_params[0],
       },
       /* Scene 3 => Damping params */
       {
              .damping_step = 0xFF,
              .damping_delay = 450,
              .hw_params = &imx105_hw_params[0],
       },
};

static struct damping_params_t imx105_inf_reg2_damping[] = {
	/* MOVE_FAR Dir */
       /* Scene 1 => Damping params */
       {
              .damping_step = 0x1FF,
              .damping_delay = 4500,
              .hw_params = &imx105_hw_params[2],
       },
       /* Scene 2 => Damping params */
       {
              .damping_step = 0x1FF,
              .damping_delay = 4500,
              .hw_params = &imx105_hw_params[1],
       },
       /* Scene 3 => Damping params */
       {
              .damping_step = 0x1FF,
              .damping_delay = 2700,
              .hw_params = &imx105_hw_params[0],
       },
};

static struct damping_t imx105_macro_regions[] = {
	/* MOVE_NEAR dir */
	/* Region 2 */
	{
		.ringing_params = imx105_macro_reg1_damping,
	},
	/* Region 2 */
	{
		.ringing_params = imx105_macro_reg2_damping,
	},
};

static struct damping_t imx105_inf_regions[] = {
	/* MOVE_FAR dir */
	/* Region 2 */
	{
		.ringing_params = imx105_inf_reg1_damping,
	},
	/* Region 2 */
	{
		.ringing_params = imx105_inf_reg2_damping,
	},
};



static const struct i2c_device_id imx105_act_i2c_id[] = {
	{"imx105_act", (kernel_ulong_t)&imx105_act_t},
	{ }
};


static int32_t imx105_set_params(struct msm_actuator_ctrl_t * a_ctrl)
{
#if 0
	uint8_t F2;
	uint8_t A1;

	if((imx105_sunny_otp_af[0] != 0)&&(imx105_sunny_otp_af[1] != 0))   //判断是否读到otp 数值
	{
	//								__func__, 
	//								imx105_regions[0].code_per_step,
	//								imx105_sunny_otp_af[0]);
		imx105_regions[0].code_per_step = imx105_sunny_otp_af[0]/2;
	}
	
	F2 = (imx105_hw_params[0]&0xFF00)>>8;
	A1 = (imx105_hw_params[0]&0xF0) >> 4;

	msm_camera_i2c_write(&a_ctrl->i2c_client,0xEC, 0xA3, MSM_CAMERA_I2C_BYTE_DATA);
	//Dual Level Control cicyle time is 15.3 msec
	msm_camera_i2c_write(&a_ctrl->i2c_client,0xA1, A1, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(&a_ctrl->i2c_client,0xF2, F2, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(&a_ctrl->i2c_client,0xDC, 0x51, MSM_CAMERA_I2C_BYTE_DATA);
#endif
	return 0;
}


static int imx105af_power_down(void *atuator_ctrl)
{
	int rc = 0;
	CDBG("%s called\n", __func__);

	if (imx105_act_t.step_position_table[imx105_act_t.curr_step_pos] !=
		imx105_act_t.initial_code) {
		rc = imx105_act_t.func_tbl.actuator_set_default_focus(&imx105_act_t);
		CDBG("%s after msm_actuator_set_default_focus\n", __func__);
		msleep(50);
	}
	msm_camera_i2c_write(&imx105_act_t.i2c_client,
		0x80, 00, MSM_CAMERA_I2C_BYTE_DATA);
	kfree(imx105_act_t.step_position_table);
	return  (int) rc;
}

static int imx105_act_config(
	void __user *argp)
{
	CDBG("%s called\n", __func__);
	return (int) msm_actuator_config(&imx105_act_t, argp);
}

static int imx105_i2c_add_driver_table(
	void)
{
	CDBG("%s called\n", __func__);
	return (int) msm_actuator_init_table(&imx105_act_t);
}

static struct i2c_driver imx105_act_i2c_driver = {
	.id_table = imx105_act_i2c_id,
	.probe  = msm_actuator_i2c_probe,
	.remove = __exit_p(imx105_act_i2c_remove),
	.driver = {
		.name = "imx105_act",
	},
};

static int __init imx105_i2c_add_driver(
	void)
{
	CDBG("%s called\n", __func__);
	return i2c_add_driver(imx105_act_t.i2c_driver);
}

static struct v4l2_subdev_core_ops imx105_act_subdev_core_ops;

static struct v4l2_subdev_ops imx105_act_subdev_ops = {
	.core = &imx105_act_subdev_core_ops,
};

static int32_t imx105_act_probe(
	void *board_info,
	void *sdev)
{
	imx105_actuator_debug_init();

	return (int) msm_actuator_create_subdevice(&imx105_act_t,
		(struct i2c_board_info const *)board_info,
		(struct v4l2_subdev *)sdev);
}

static struct msm_actuator_ctrl_t imx105_act_t = {
	.i2c_driver = &imx105_act_i2c_driver,
	.i2c_addr = 0x18,
	.act_v4l2_subdev_ops = &imx105_act_subdev_ops,
	.actuator_ext_ctrl = {
		.a_init_table = imx105_i2c_add_driver_table,
		.a_create_subdevice = imx105_act_probe,
		.a_config = imx105_act_config,
		.a_power_down = imx105af_power_down,
	},

	.i2c_client = {
		.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
	},

	.set_info = {
		.total_steps = IMX105_TOTAL_STEPS_NEAR_TO_FAR_MAX,
	},

	.curr_step_pos = 0,
	.curr_region_index = 0,
	.initial_code = 0,
	.actuator_mutex = &imx105_act_mutex,

	/* Initialize scenario */
	.ringing_scenario[MOVE_NEAR] = imx105_macro_scenario,
	.scenario_size[MOVE_NEAR] = ARRAY_SIZE(imx105_macro_scenario),
	.ringing_scenario[MOVE_FAR] = imx105_inf_scenario,
	.scenario_size[MOVE_FAR] = ARRAY_SIZE(imx105_inf_scenario),

	/* Initialize region params */
	.region_params = imx105_regions,
	.region_size = ARRAY_SIZE(imx105_regions),

	/* Initialize damping params */
	.damping[MOVE_NEAR] = imx105_macro_regions,
	.damping[MOVE_FAR] = imx105_inf_regions,

	.func_tbl = {
		.actuator_set_params = imx105_set_params,
		.actuator_init_focus = NULL,
		.actuator_init_table = msm_actuator_init_table,
		.actuator_move_focus = msm_actuator_move_focus,
		.actuator_write_focus = msm_actuator_write_focus,
		.actuator_i2c_write = imx105_wrapper_i2c_write,
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

static int imx105_actuator_set_delay(void *data, u64 val)
{
	imx105_inf_reg2_damping[(val & 0xF0000)>>16].damping_delay = val;
	return 0;
}

static int imx105_actuator_get_delay(void *data, u64 *val)
{
	*val = imx105_inf_reg2_damping[0].damping_delay;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(imx105_delay,
	imx105_actuator_get_delay,
	imx105_actuator_set_delay,
	"%llu\n");

static int imx105_actuator_set_jumpparam(void *data, u64 val)
{
	imx105_inf_reg2_damping[(val & 0xF000)>>12].damping_step = val & 0xFFF;
	return 0;
}

static int imx105_actuator_get_jumpparam(void *data, u64 *val)
{
	*val = imx105_inf_reg2_damping[0].damping_step;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(imx105_jumpparam,
	imx105_actuator_get_jumpparam,
	imx105_actuator_set_jumpparam,
	"%llu\n");

static int imx105_actuator_set_hwparam(void *data, u64 val)
{

	imx105_hw_params[(val & 0xF00)>>8] = val & 0xFF;
	return 0;
}

static int imx105_actuator_get_hwparam(void *data, u64 *val)
{
	*val = imx105_hw_params[0];
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(imx105_hwparam,
	imx105_actuator_get_hwparam,
	imx105_actuator_set_hwparam,
	"%llu\n");

static int imx105_move_af(void *data, u64 val)
{
	struct msm_actuator_ctrl_t *a_ctrl = (struct msm_actuator_ctrl_t *)data;

	a_ctrl->func_tbl.actuator_move_focus(a_ctrl, (val & 0x100) >> 8,
		val & 0xFF);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(imx105_move,
	NULL, imx105_move_af, "%llu\n");


static uint16_t imx105_linear_step = 1;
static int imx105_set_linear_step(void *data, u64 val)
{
	imx105_linear_step = val & 0xFF;
	return 0;
}

static int imx105_af_linearity_test(void *data, u64 *val)
{
	int16_t index = 0;
	struct msm_actuator_ctrl_t *a_ctrl = (struct msm_actuator_ctrl_t *)data;

	a_ctrl->func_tbl.actuator_set_default_focus(a_ctrl);
	usleep(1000000);

	for (index = 0; index < a_ctrl->set_info.total_steps;
		index+=imx105_linear_step) {
		a_ctrl->func_tbl.actuator_move_focus(a_ctrl, MOVE_NEAR,
			imx105_linear_step);
		CDBG("debugfs, new loc %d\n", index);
		usleep(1000000);
	}

	CDBG("debugfs moved to macro boundary\n");

	for (index = 0; index < a_ctrl->set_info.total_steps;
		index+=imx105_linear_step) {
		a_ctrl->func_tbl.actuator_move_focus(a_ctrl, MOVE_FAR,
			imx105_linear_step);
		CDBG("debugfs, new loc %d\n", index);
		usleep(1000000);
	}

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(imx105_linear,
	imx105_af_linearity_test, imx105_set_linear_step, "%llu\n");

static int imx105_set_af_codestep(void *data, u64 val)
{
	struct msm_actuator_ctrl_t *a_ctrl = (struct msm_actuator_ctrl_t *)data;


	/* Update last region's macro boundary in region params table */
	a_ctrl->region_params[(val & 0xF00) >> 8].code_per_step = val & 0xFF;

	/* Call init table to re-create step position table based on updated total steps */
	a_ctrl->func_tbl.actuator_init_table(a_ctrl);

	CDBG("%s called, code per step = %d\n",
		__func__, a_ctrl->region_params[(val & 0xF00) >> 8].code_per_step);
	return 0;
}

static int imx105_get_af_codestep(void *data, u64 *val)
{
	struct msm_actuator_ctrl_t *a_ctrl = (struct msm_actuator_ctrl_t *)data;
	CDBG("%s called\n", __func__);

	*val = a_ctrl->region_params[a_ctrl->region_size-1].code_per_step;
	return 0;
}


DEFINE_SIMPLE_ATTRIBUTE(imx105_codeperstep,
	imx105_get_af_codestep, imx105_set_af_codestep, "%llu\n");

static int imx105_get_hw3(void *data, u64 *val)
{

	*val = imx105_hw_params[3];
	CDBG("imx105_get_hw hw_params2 = 0x%x hw_param3 = 0x%x\n", imx105_hw_params[2],imx105_hw_params[3]);
	return 0;
}

static int imx105_set_hw(void *data, u64 val)
{
	struct msm_actuator_ctrl_t *a_ctrl = (struct msm_actuator_ctrl_t *)data;
	imx105_hw_params[(val & 0xF0000)>>16]= val &0xFFFF;
	CDBG("imx105_set_hw\n");
	a_ctrl->func_tbl.actuator_set_params(a_ctrl);

	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(imx105_hw,
	imx105_get_hw3, imx105_set_hw, "%llu\n");

static int imx105_actuator_debug_init(void)
{
	struct dentry *debugfs_base = debugfs_create_dir("imx105_actuator", NULL);
	if (!debugfs_base)
		return -ENOMEM;

	if (!debugfs_create_file("imx105_delay",
		S_IRUGO | S_IWUSR, debugfs_base, NULL, &imx105_delay))
		return -ENOMEM;

	if (!debugfs_create_file("imx105_jumpparam",
		S_IRUGO | S_IWUSR, debugfs_base, NULL, &imx105_jumpparam))
		return -ENOMEM;

	if (!debugfs_create_file("imx105_hwparam",
		S_IRUGO | S_IWUSR, debugfs_base, NULL, &imx105_hwparam))
		return -ENOMEM;

	if (!debugfs_create_file("imx105_linear",
		S_IRUGO | S_IWUSR, debugfs_base,  &imx105_act_t, &imx105_linear))
		return -ENOMEM;

	if (!debugfs_create_file("imx105_move",
		S_IRUGO | S_IWUSR, debugfs_base,  &imx105_act_t, &imx105_move))
		return -ENOMEM;

	if (!debugfs_create_file("imx105_codeperstep",
		S_IRUGO | S_IWUSR, debugfs_base,  &imx105_act_t, &imx105_codeperstep))
		return -ENOMEM;

	if (!debugfs_create_file("imx105_hw",
		S_IRUGO | S_IWUSR, debugfs_base,  &imx105_act_t, &imx105_hw))
		return -ENOMEM;


	return 0;
}
subsys_initcall(imx105_i2c_add_driver);
MODULE_DESCRIPTION("IMX105 actuator");
MODULE_LICENSE("GPL v2");
