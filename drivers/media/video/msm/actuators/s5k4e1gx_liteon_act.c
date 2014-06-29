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
/*
#undef CDBG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
*/



#define S5K4E1GX_TOTAL_STEPS_NEAR_TO_FAR_MAX 37   
#define INIT_MODE 0x3
#define INIT_CODE 0 

DEFINE_MUTEX(s5k4e1gx_act_mutex);
static int s5k4e1gx_actuator_debug_init(void);
static struct msm_actuator_ctrl_t s5k4e1gx_act_t;
static int8_t ad5820_vcm_mode = 0xFF; 

static int32_t s5k4e1gx_wrapper_i2c_write(struct msm_actuator_ctrl_t *a_ctrl,
	int16_t next_lens_position, void *params)
{


unsigned char buf[2];
uint8_t msb = 0, lsb = 0;

msb = (next_lens_position >> 4 )& 0x3F;
lsb = (next_lens_position & 0x000F) << 4;

if((ad5820_vcm_mode & 0xF) == 0xF) 
 { 
	lsb |= ((*(uint8_t *)params)& 0xF ) ;
	CDBG("__debug: %s: next_lens_position=%d, mode=0x%x \n", __func__,
    next_lens_position,(*(uint8_t *)params));

}
else
{//manual test mode
	lsb |= (ad5820_vcm_mode & 0xF ) ;
	CDBG("__debug: %s: next_lens_position=%d,mode=0x%x ,\n", __func__,
 	   next_lens_position,ad5820_vcm_mode);
}

buf[0] = msb;
buf[1] = lsb;
msm_camera_i2c_txdata(&a_ctrl->i2c_client, buf, 2);

	return next_lens_position;
}

static uint8_t s5k4e1gx_hw_params[] = { //vcm mode
    0x0,
	0x1,
	0x2,
	0x3,
	0x5,//800us,1ms
};

static uint16_t s5k4e1gx_macro_scenario[] = {
	/* MOVE_NEAR dir*/
	1,
	4,
	8,
	S5K4E1GX_TOTAL_STEPS_NEAR_TO_FAR_MAX,
};

static uint16_t s5k4e1gx_inf_scenario[] = {
	/* MOVE_FAR dir */
	1,
	4,
    8,
	16,
	S5K4E1GX_TOTAL_STEPS_NEAR_TO_FAR_MAX,
};


static struct region_params_t s5k4e1gx_regions[] = {
	/* step_bound[0] - macro side boundary
	 * step_bound[1] - infinity side boundary
	 */
	/* Region 1 */
	{
		.step_bound = {2, 0},
		.code_per_step = 120,
	},
	/* Region 2 */
	{
		.step_bound = {S5K4E1GX_TOTAL_STEPS_NEAR_TO_FAR_MAX, 2},
		.code_per_step = 16,
	}
};

static struct damping_params_t s5k4e1gx_macro_reg1_damping[] = {
	/* MOVE_NEAR Dir */
	/* Scene 1 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 800,
		.hw_params = &s5k4e1gx_hw_params[0],
	},
		/* Scene 2 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 800,
		.hw_params = &s5k4e1gx_hw_params[0],
	},
		/* Scene 3 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 800, 
		.hw_params = &s5k4e1gx_hw_params[0],
	},
		/* Scene 4 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 800, 
		.hw_params = &s5k4e1gx_hw_params[0],
	},

};

static struct damping_params_t s5k4e1gx_macro_reg2_damping[] = {
	/* MOVE_NEAR Dir */
	/* Scene 1 => Damping params */
	{
		.damping_step = 0xFF,//8ms
		.damping_delay = 200,
		.hw_params = &s5k4e1gx_hw_params[4],
	},
	/* Scene 2 => Damping params */
	{
		.damping_step = 0xFF,//8ms 9ms 10ms
		.damping_delay = 200,//>200
		.hw_params = &s5k4e1gx_hw_params[3],
	},
	/* Scene 3 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 333,
		.hw_params = &s5k4e1gx_hw_params[2],
	},
		/* Scene 4 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 333,
		.hw_params = &s5k4e1gx_hw_params[1],
	},

};

static struct damping_params_t s5k4e1gx_inf_reg1_damping[] = {
	/* MOVE_FAR Dir */
	/* Scene 1 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 400,
		.hw_params = &s5k4e1gx_hw_params[3],
	},
	/* Scene 2 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 400,
		.hw_params = &s5k4e1gx_hw_params[2],
	},
	/* Scene 3 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 400,
		.hw_params = &s5k4e1gx_hw_params[1],
	},
	/* Scene 4 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 400,
		.hw_params = &s5k4e1gx_hw_params[1],
	},
	/* Scene 5 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 400,
		.hw_params = &s5k4e1gx_hw_params[1],
	},
};
static struct damping_params_t s5k4e1gx_inf_reg2_damping[] = {
	/* MOVE_FAR Dir */
	/* Scene 1 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 0x8f,
		.hw_params = &s5k4e1gx_hw_params[4],
	},
	/* Scene 2 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 312,
		.hw_params = &s5k4e1gx_hw_params[3],
	},
	/* Scene 3 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 900,
		.hw_params = &s5k4e1gx_hw_params[2],
	},
	/* Scene 4 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 1,
		.hw_params = &s5k4e1gx_hw_params[1],
	},
	/* Scene 5 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 1,
		.hw_params = &s5k4e1gx_hw_params[1],
	},
};

static struct damping_t s5k4e1gx_macro_regions[] = {
	/* MOVE_NEAR dir */
	/* Region 1 */
	{
		.ringing_params = s5k4e1gx_macro_reg1_damping,
	},
	/* Region 2 */
	{
		.ringing_params = s5k4e1gx_macro_reg2_damping,
	},
};

static struct damping_t s5k4e1gx_inf_regions[] = {
	/* MOVE_FAR dir */
	/* Region 1 */
	{
		.ringing_params = s5k4e1gx_inf_reg1_damping,
	},
	/* Region 2 */
	{
		.ringing_params = s5k4e1gx_inf_reg2_damping,
	},
};

static int32_t s5k4e1gx_act_init_focus(struct msm_actuator_ctrl_t *a_ctrl)
{
	int32_t rc;
	int8_t mode= INIT_MODE;
	LINFO("%s called\n", __func__);

    //hardware init   init to inf
	msm_camera_i2c_write(&s5k4e1gx_act_t.i2c_client,
		0x00, 00, MSM_CAMERA_I2C_BYTE_DATA);
	msleep(50);

	rc = a_ctrl->func_tbl.actuator_i2c_write(a_ctrl, INIT_CODE, (void *)(&mode));

	a_ctrl->curr_step_pos = 0;
	return rc;
}

static const struct i2c_device_id s5k4e1gx_act_i2c_id[] = {
	{"s5k4e1gx_act", (kernel_ulong_t)&s5k4e1gx_act_t},
	{ }
};

static int s5k4e1gx_act_power_down(void *atuator_ctrl)
{
	int rc = 0;
	CDBG("%s called\n", __func__);

	if (s5k4e1gx_act_t.step_position_table[s5k4e1gx_act_t.curr_step_pos] !=
		s5k4e1gx_act_t.initial_code) {
		rc = s5k4e1gx_act_t.func_tbl.actuator_set_default_focus(&s5k4e1gx_act_t);
	
		CDBG("%s after msm_actuator_set_default_focus\n", __func__);
		msleep(50);
	}
	//enter power down
	msm_camera_i2c_write(&s5k4e1gx_act_t.i2c_client,
		0x80, 00, MSM_CAMERA_I2C_BYTE_DATA);
//	kfree(s5k4e1gx_act_t.step_position_table);
	return  (int) rc;
}
static int s5k4e1gx_act_config(
	void __user *argp)
{
	CDBG("%s called\n", __func__);
	return (int) msm_actuator_config(&s5k4e1gx_act_t, argp);
}

static int s5k4e1gx_i2c_add_driver_table(
	void)
{
	CDBG("%s called\n", __func__);
	return (int) msm_actuator_init_table(&s5k4e1gx_act_t);
}

static struct i2c_driver s5k4e1gx_act_i2c_driver = {
	.id_table = s5k4e1gx_act_i2c_id,
	.probe  = msm_actuator_i2c_probe,
	.remove = __exit_p(s5k4e1gx_act_i2c_remove),
	.driver = {
		.name = "s5k4e1gx_act",
	},
};

static int __init s5k4e1gx_i2c_add_driver(
	void)
{
	CDBG("__debug: %s called\n", __func__);
	return i2c_add_driver(s5k4e1gx_act_t.i2c_driver);
}

static struct v4l2_subdev_core_ops s5k4e1gx_act_subdev_core_ops;

static struct v4l2_subdev_ops s5k4e1gx_act_subdev_ops = {
	.core = &s5k4e1gx_act_subdev_core_ops,
};

static int32_t s5k4e1gx_act_probe(
	void *board_info,
	void *sdev)
{
	s5k4e1gx_actuator_debug_init();

	return (int) msm_actuator_create_subdevice(&s5k4e1gx_act_t,
		(struct i2c_board_info const *)board_info,
		(struct v4l2_subdev *)sdev);
}

static struct msm_actuator_ctrl_t s5k4e1gx_act_t = {
	.i2c_driver = &s5k4e1gx_act_i2c_driver,
	.i2c_addr = 0x18,
	.act_v4l2_subdev_ops = &s5k4e1gx_act_subdev_ops,
	.actuator_ext_ctrl = {
		.a_init_table = s5k4e1gx_i2c_add_driver_table,
		.a_create_subdevice = s5k4e1gx_act_probe,
		.a_config = s5k4e1gx_act_config,
		.a_power_down = s5k4e1gx_act_power_down,
	},

	.i2c_client = {
		.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
	},

	.set_info = {
		.total_steps = S5K4E1GX_TOTAL_STEPS_NEAR_TO_FAR_MAX,
	},

	.curr_step_pos = 0,
	.curr_region_index = 0,
	.initial_code = INIT_CODE,
	.actuator_mutex = &s5k4e1gx_act_mutex,


	.func_tbl = {
	//	.actuator_set_params = s5k4e1gx_set_params,
		.actuator_init_table = msm_actuator_init_table,
		.actuator_move_focus = msm_actuator_move_focus,
		.actuator_write_focus = msm_actuator_write_focus,
		.actuator_set_default_focus = msm_actuator_set_default_focus,
		.actuator_init_focus = s5k4e1gx_act_init_focus,
		.actuator_i2c_write = s5k4e1gx_wrapper_i2c_write,
	},

	.get_info = {
		.focal_length_num = 357,
		.focal_length_den = 100,
		.f_number_num = 28,
		.f_number_den = 10,
		.f_pix_num = 14,
		.f_pix_den = 10,
		.total_f_dist_num = 197681,////???
		.total_f_dist_den = 1000,///???
		.hor_view_angle_num = 548,
		.hor_view_angle_den = 10,
		.ver_view_angle_num = 425,
		.ver_view_angle_den = 10,

	},

	/* Initialize scenario */
	.ringing_scenario[MOVE_NEAR] = s5k4e1gx_macro_scenario,
	.scenario_size[MOVE_NEAR] = ARRAY_SIZE(s5k4e1gx_macro_scenario),
	.ringing_scenario[MOVE_FAR] = s5k4e1gx_inf_scenario,
	.scenario_size[MOVE_FAR] = ARRAY_SIZE(s5k4e1gx_inf_scenario),

	/* Initialize region params */
	.region_params = s5k4e1gx_regions,
	.region_size = ARRAY_SIZE(s5k4e1gx_regions),

	/* Initialize damping params */
	.damping[MOVE_NEAR] = s5k4e1gx_macro_regions,
	.damping[MOVE_FAR] = s5k4e1gx_inf_regions,
};

static int ad5820_actuator_set_hw(void *data, u64 val)
{
	CDBG("__debug:%s val=0x %x \n", __func__,(u32)val);

    s5k4e1gx_hw_params[0] = (val & 0x000000FF)>>0;
    s5k4e1gx_hw_params[1] = (val & 0x0000FF00)>>8;
    s5k4e1gx_hw_params[2] = (val & 0x00FF0000)>>16;
    s5k4e1gx_hw_params[3] = (val & 0xFF000000)>>24;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(ad5820_hw,
	NULL, ad5820_actuator_set_hw, "%llu\n");

static int ad5820_actuator_set_mode(void *data, u64 val)
{
	CDBG("__debug:%s val= %d \n", __func__,(int)val);


    ad5820_vcm_mode = (val & 0xF);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(ad5820_mode,
	NULL, ad5820_actuator_set_mode, "%llu\n");

static int ad5820_actuator_set_threshold(void *data, u64 val)
{
	CDBG("__debug:%s val= %d \n", __func__,(int)val);


	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(ad5820_threshold,
	NULL, ad5820_actuator_set_threshold, "%llu\n");

static int ad5820_actuator_set_sw(void *data, u64 val)
{
	CDBG("__debug:%s val= %d \n", __func__,(int)val);

    switch((val & 0xF0000000)>>28)
    {
        case 1://s5k4e1gx_inf_reg2_damping

            s5k4e1gx_inf_reg2_damping[(val & 0xF000000) >> 24].damping_step =
                (val & 0xFF0000) >> 16;
            s5k4e1gx_inf_reg2_damping[(val & 0xF000000) >> 24].damping_delay =
                (val & 0xFFFF);
            CDBG("__debug:%s: s5k4e1gx_inf_reg2_damping sence=%d, step: 0x%x, delay: 0x%x\n", __func__,(int)((val & 0xF000000) >> 24),
                s5k4e1gx_inf_reg2_damping[(val & 0xF000000) >> 24].damping_step,
                s5k4e1gx_inf_reg2_damping[(val & 0xF000000) >> 24].damping_delay);
            break;

        case 2://s5k4e1gx_macro_reg2_damping
            s5k4e1gx_macro_reg2_damping[(val & 0xF000000) >> 24].damping_step =
                (val & 0xFF0000) >> 16;
            s5k4e1gx_macro_reg2_damping[(val & 0xF000000) >> 24].damping_delay =
                (val & 0xFFFF);
         CDBG("__debug:%s: s5k4e1gx_macro_reg2_damping sence=%d, step: 0x%x, delay: 0x%x\n", __func__,(int)((val & 0xF000000) >> 24),
             s5k4e1gx_macro_reg2_damping[(val & 0xF000000) >> 24].damping_step,
             s5k4e1gx_macro_reg2_damping[(val & 0xF000000) >> 24].damping_delay);
            break;
            
        case 3://s5k4e1gx_inf_reg1_damping
            s5k4e1gx_inf_reg1_damping[(val & 0xF000000) >> 24].damping_step =
                (val & 0xFF0000) >> 16;
            s5k4e1gx_inf_reg1_damping[(val & 0xF000000) >> 24].damping_delay =
                (val & 0xFFFF);
            CDBG("__debug:%s: s5k4e1gx_inf_reg1_damping sence=%d, step: 0x%x, delay: 0x%x\n", __func__,(int)((val & 0xF000000) >> 24),
                s5k4e1gx_inf_reg1_damping[(val & 0xF000000) >> 24].damping_step,
                s5k4e1gx_inf_reg1_damping[(val & 0xF000000) >> 24].damping_delay);

            break;
            
        case 4:////s5k4e1gx_macro_reg1_damping
            s5k4e1gx_macro_reg1_damping[(val & 0xF000000) >> 24].damping_step =
                (val & 0xFF0000) >> 16;
            s5k4e1gx_macro_reg1_damping[(val & 0xF000000) >> 24].damping_delay =
                (val & 0xFFFF);
            CDBG("__debug:%s: s5k4e1gx_macro_reg1_damping  sence=%d, step: 0x%x, delay: 0x%x\n", __func__,(int)((val & 0xF000000) >> 24),
                s5k4e1gx_macro_reg1_damping[(val & 0xF000000) >> 24].damping_step,
                s5k4e1gx_macro_reg1_damping[(val & 0xF000000) >> 24].damping_delay);
               break;
            
            default:
                
                CDBG("__debug:%s:seting error ! \n", __func__);
                break;

    }


	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(ad5820_sw,
	NULL,
	ad5820_actuator_set_sw,
	"%llu\n");


static int ad5820_move_af(void *data, u64 val)
{
	struct msm_actuator_ctrl_t *a_ctrl = (struct msm_actuator_ctrl_t *)data;
	CDBG("__debug:%s val= %d \n", __func__,(int32_t)val);

	a_ctrl->func_tbl.actuator_move_focus(a_ctrl, (val & 0x100) >> 8,
		val & 0xFF);
	return 0;
}


DEFINE_SIMPLE_ATTRIBUTE(ad5820_move,
	NULL, ad5820_move_af, "%llu\n");

static uint16_t ad5820_linear_step = 1;
static int ad5820_set_linear_step(void *data, u64 val)
{
	CDBG("__debug:%s val= %d \n", __func__,(int)val);

	ad5820_linear_step = val & 0xFF;
	return 0;
}

static int ad5820_af_linearity_test(void *data, u64 *val)
{
	int16_t index = 0;
	struct msm_actuator_ctrl_t *a_ctrl = (struct msm_actuator_ctrl_t *)data;
	CDBG("__debug:%s val= %d \n", __func__,(int)val);

	a_ctrl->func_tbl.actuator_set_default_focus(a_ctrl);
	msleep(1000);

	for (index = 0; index < a_ctrl->set_info.total_steps;
		index+=ad5820_linear_step) {
		a_ctrl->func_tbl.actuator_move_focus(a_ctrl, MOVE_NEAR,
			ad5820_linear_step);
		CDBG("__debug:debugfs, new loc %d\n", index);
		msleep(1000);
	}

	CDBG("__debug: debugfs moved to macro boundary\n");

	for (index = 0; index < a_ctrl->set_info.total_steps;
		index+=ad5820_linear_step) {
		a_ctrl->func_tbl.actuator_move_focus(a_ctrl, MOVE_FAR,
			ad5820_linear_step);
		CDBG("debugfs, new loc %d\n", index);
		msleep(1000);
	}

	return 0;
}


DEFINE_SIMPLE_ATTRIBUTE(ad5820_linear,
	ad5820_af_linearity_test, ad5820_set_linear_step, "%llu\n");


static int ad5820_set_af_codestep(void *data, u64 val)
{
	struct msm_actuator_ctrl_t *a_ctrl = (struct msm_actuator_ctrl_t *)data;
	CDBG("__debug:%s val= %d \n", __func__,(int)val);
	
	/* Update last region's macro boundary in region params table */
	a_ctrl->region_params[(val & 0xF00) >> 8].code_per_step = val & 0xFF;

	/* Call init table to re-create step position table based on updated total steps */
	a_ctrl->func_tbl.actuator_init_table(a_ctrl);

	CDBG("__debug:%s called, code per step = %d\n",
		__func__, a_ctrl->region_params[(val & 0xF00) >> 8].code_per_step);
	return 0;
}

static int ad5820_get_af_codestep(void *data, u64 *val)
{
	struct msm_actuator_ctrl_t *a_ctrl = (struct msm_actuator_ctrl_t *)data;
	CDBG("__debug:%s called\n", __func__);

	*val = a_ctrl->region_params[a_ctrl->region_size-1].code_per_step;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(ad5820_codeperstep,
	ad5820_get_af_codestep, ad5820_set_af_codestep, "%llu\n");

static int s5k4e1gx_actuator_debug_init(void)
{

struct dentry *debugfs_base = debugfs_create_dir("ad5820_actuator",
    NULL);
if (!debugfs_base)
    return -ENOMEM;

if (!debugfs_create_file("ad5820_linear",
    S_IRUGO | S_IWUSR, debugfs_base, (void *)(&s5k4e1gx_act_t), &ad5820_linear))
    return -ENOMEM;

if (!debugfs_create_file("ad5820_move",
    S_IRUGO | S_IWUSR, debugfs_base, (void *)(&s5k4e1gx_act_t), &ad5820_move))
    return -ENOMEM;

if (!debugfs_create_file("ad5820_codeperstep",
    S_IRUGO | S_IWUSR, debugfs_base, (void *)(&s5k4e1gx_act_t), &ad5820_codeperstep))
    return -ENOMEM;

if (!debugfs_create_file("ad5820_hw",
    S_IRUGO | S_IWUSR, debugfs_base, (void *)(&s5k4e1gx_act_t), &ad5820_hw))
    return -ENOMEM;

if (!debugfs_create_file("ad5820_sw",
    S_IRUGO | S_IWUSR, debugfs_base, (void *)(&s5k4e1gx_act_t), &ad5820_sw))
    return -ENOMEM;

if (!debugfs_create_file("ad5820_threshold",
    S_IRUGO | S_IWUSR, debugfs_base, (void *)(&s5k4e1gx_act_t), &ad5820_threshold))
    return -ENOMEM;

if (!debugfs_create_file("ad5820_mode",
    S_IRUGO | S_IWUSR, debugfs_base, (void *)(&s5k4e1gx_act_t), &ad5820_mode))
	return 0;
	return 0;
}

subsys_initcall(s5k4e1gx_i2c_add_driver);
MODULE_DESCRIPTION("S5K4E1GX actuator");
MODULE_LICENSE("GPL v2");
