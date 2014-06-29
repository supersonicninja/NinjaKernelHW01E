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
#define MT9P017_TOTAL_STEPS_NEAR_TO_FAR_MAX 37 
#define INIT_MODE 0x3
#define INIT_CODE 0  
#define REG_VCM_NEW_CODE 0x30F2
#define REG_VCM_CONTROL 0x30F0
#define REG_VCM_TIME 0x30F4

DEFINE_MUTEX(mt9p017_act_mutex);
static int mt9p017_actuator_debug_init(void);
static struct msm_actuator_ctrl_t mt9p017_act_t;
static int8_t vcm_mode_mt9p017 = 0xFF; 

static int mt9p017_vcm_actuator_step_time(void *data, u64 val)
{
    CDBG("__debug:%s val= %x \n", __func__,(int)val);
	msm_camera_i2c_write(&(mt9p017_act_t.i2c_client),
			     REG_VCM_TIME,
			     (u16)val,
			     MSM_CAMERA_I2C_WORD_DATA);
	return 0;
}

static int32_t mt9p017_wrapper_i2c_write(struct msm_actuator_ctrl_t *a_ctrl,
	int16_t next_lens_position, void *params)
{
#if 1
    if((vcm_mode_mt9p017 & 0xF) == 0xF) 
     { 
        	msm_camera_i2c_write(&a_ctrl->i2c_client,
    			     REG_VCM_CONTROL,
    			     (0X8010 & 0xFFF0)| ((*(uint8_t *)params)& 0xF ),
    			     MSM_CAMERA_I2C_WORD_DATA);
	    CDBG("__debug:%s, next_lens_position=%d,vcm_mode=%d \n",
            __func__,next_lens_position,(*(uint8_t *)params));
    }
    else
    {
    	msm_camera_i2c_write(&a_ctrl->i2c_client,
    			     REG_VCM_CONTROL,
    			     (0X8010 & 0xFFF0)| ((vcm_mode_mt9p017)& 0xF ),
    			     MSM_CAMERA_I2C_WORD_DATA);
        CDBG("__debug:%s, next_lens_position=%d,vcm_mode_mt9p017 =%d \n",
            __func__,next_lens_position,vcm_mode_mt9p017);

    }

#endif
	msm_camera_i2c_write(&a_ctrl->i2c_client,
			     REG_VCM_NEW_CODE,
			     next_lens_position,
			     MSM_CAMERA_I2C_WORD_DATA);

	return next_lens_position;
}

static uint8_t mt9p017_hw_params[] = {
	0x1, // 50us//1
	0x2,
	0x2,//100us
	0x3,//200us
	0x5,//800us,1ms
};
static uint16_t mt9p017_macro_scenario[] = {
	/* MOVE_NEAR dir*/
	1,
	4,
	8,
	MT9P017_TOTAL_STEPS_NEAR_TO_FAR_MAX,
};

static uint16_t mt9p017_inf_scenario[] = {
	/* MOVE_FAR dir */
	1,
	4,
    8,
	16,
	MT9P017_TOTAL_STEPS_NEAR_TO_FAR_MAX,
};

static struct region_params_t mt9p017_regions[] = {
	/* step_bound[0] - macro side boundary
	 * step_bound[1] - infinity side boundary
	 */
	/* Region 1 */
	{
		.step_bound = {2, 0},
		.code_per_step = 29,
	},
	/* Region 2 */
	{
		.step_bound = {MT9P017_TOTAL_STEPS_NEAR_TO_FAR_MAX, 2},
		.code_per_step = 4,
	}
};

static struct damping_params_t mt9p017_macro_reg1_damping[] = {
	/* MOVE_NEAR Dir */
	/* Scene 1 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 800,
		.hw_params = &mt9p017_hw_params[0],
	},
	/* Scene 2 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 800,
		.hw_params = &mt9p017_hw_params[0],
	},
	/* Scene 3 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 800, 
		.hw_params = &mt9p017_hw_params[0],
	},
	/* Scene 4 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 800, 
		.hw_params = &mt9p017_hw_params[0],
	},

};

static struct damping_params_t mt9p017_macro_reg2_damping[] = {
	/* MOVE_NEAR Dir */
	/* Scene 1 => Damping params */
	{
		.damping_step = 0xFF,//8ms
		.damping_delay = 200,
		.hw_params = &mt9p017_hw_params[4],
	},
	/* Scene 2 => Damping params */
	{
		.damping_step = 0xFF,//8ms 9ms 10ms
		.damping_delay = 200,//>200
		.hw_params = &mt9p017_hw_params[3],
	},
	/* Scene 3 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 333,
		.hw_params = &mt9p017_hw_params[2],
	},
	/* Scene 4 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 333,
		.hw_params = &mt9p017_hw_params[1],
	},
};

static struct damping_params_t mt9p017_inf_reg1_damping[] = {
	/* MOVE_FAR Dir */
	/* Scene 1 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 400,
		.hw_params = &mt9p017_hw_params[0],
	},
	/* Scene 2 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 400,
		.hw_params = &mt9p017_hw_params[0],
	},
	/* Scene 3 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 400,
		.hw_params = &mt9p017_hw_params[0],
	},
	/* Scene 4 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 400,
		.hw_params = &mt9p017_hw_params[0],
	},
	/* Scene 5 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 400,
		.hw_params = &mt9p017_hw_params[0],
	},
};

static struct damping_params_t mt9p017_inf_reg2_damping[] = {
	/* MOVE_FAR Dir */
	/* Scene 1 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 0x8f,
		.hw_params = &mt9p017_hw_params[4],
	},
	/* Scene 2 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 312,
		.hw_params = &mt9p017_hw_params[3],
	},
	/* Scene 3 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 900,
		.hw_params = &mt9p017_hw_params[2],
	},
	/* Scene 4 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 1,
		.hw_params = &mt9p017_hw_params[1],
	},
	/* Scene 5 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 1,
		.hw_params = &mt9p017_hw_params[1],
	},
};

static struct damping_t mt9p017_macro_regions[] = {
	/* MOVE_NEAR dir */
	/* Region 1 */
	{
		.ringing_params = mt9p017_macro_reg1_damping,
	},
	/* Region 2 */
	{
		.ringing_params = mt9p017_macro_reg2_damping,
	},
};

static struct damping_t mt9p017_inf_regions[] = {
	/* MOVE_FAR dir */
	/* Region 1 */
	{
		.ringing_params = mt9p017_inf_reg1_damping,
	},
	/* Region 2 */
	{
		.ringing_params = mt9p017_inf_reg2_damping,
	},
};

static int32_t mt9p017_act_init_focus(struct msm_actuator_ctrl_t *a_ctrl)
{
	int32_t rc;
	int8_t mode= INIT_MODE;
	LINFO("%s called\n", __func__);
	/* Initialize to infinity */
    //hardware init
	msm_camera_i2c_write(&mt9p017_act_t.i2c_client,
		     REG_VCM_CONTROL,0X8010,MSM_CAMERA_I2C_WORD_DATA);
			 
	msleep(50);

    mt9p017_vcm_actuator_step_time(NULL, 0x028A);	//650	
	rc = a_ctrl->func_tbl.actuator_i2c_write(a_ctrl, INIT_CODE, (void *)(&mode));
	a_ctrl->curr_step_pos = 0;
	return rc;
}

static const struct i2c_device_id mt9p017_act_i2c_id[] = {
	{"mt9p017_act", (kernel_ulong_t)&mt9p017_act_t},
	{ }
};

static int mt9p017_act_power_down(void *atuator_ctrl)
{
	int rc = 0;
	CDBG("%s called\n", __func__);

	if (mt9p017_act_t.step_position_table[mt9p017_act_t.curr_step_pos] !=
		mt9p017_act_t.initial_code) {
		rc = mt9p017_act_t.func_tbl.actuator_set_default_focus(&mt9p017_act_t);
	
		CDBG("%s after msm_actuator_set_default_focus\n", __func__);
		msleep(50);
	}
	msm_camera_i2c_write(&mt9p017_act_t.i2c_client,
		     REG_VCM_CONTROL,0X8000,MSM_CAMERA_I2C_WORD_DATA);
//	kfree(mt9p017_act_t.step_position_table);
	return  (int) rc;
}





static int mt9p017_act_config(
	void __user *argp)
{
	CDBG("%s called\n", __func__);

	return (int) msm_actuator_config(&mt9p017_act_t, argp);
}


static int mt9p017_i2c_add_driver_table(
	void)
{
	CDBG("%s called\n", __func__);
	return (int) msm_actuator_init_table(&mt9p017_act_t);
}

static struct i2c_driver mt9p017_act_i2c_driver = {
	.id_table = mt9p017_act_i2c_id,
	.probe  = msm_actuator_i2c_probe,
	.remove = __exit_p(mt9p017_act_i2c_remove),
	.driver = {
		.name = "mt9p017_act",
	},
};

static int __init mt9p017_i2c_add_driver(
	void)
{
	CDBG("%s called\n", __func__);
	return i2c_add_driver(mt9p017_act_t.i2c_driver);
}

static struct v4l2_subdev_core_ops mt9p017_act_subdev_core_ops;

static struct v4l2_subdev_ops mt9p017_act_subdev_ops = {
	.core = &mt9p017_act_subdev_core_ops,
};


static int32_t mt9p017_act_probe(
	void *board_info,
	void *sdev)
{
    int rc = 0;
	mt9p017_actuator_debug_init();
    rc =  (int) msm_actuator_create_subdevice(&mt9p017_act_t,
		(struct i2c_board_info const *)board_info,
		(struct v4l2_subdev *)sdev);
		
    return rc;
}


static struct msm_actuator_ctrl_t mt9p017_act_t = {
	.i2c_driver = &mt9p017_act_i2c_driver,
	.i2c_addr = 0x6C,
	.act_v4l2_subdev_ops = &mt9p017_act_subdev_ops,
	.actuator_ext_ctrl = {
		.a_init_table = mt9p017_i2c_add_driver_table,
		.a_create_subdevice = mt9p017_act_probe,
		.a_config = mt9p017_act_config,
		.a_power_down = mt9p017_act_power_down,
	},

	.i2c_client = {
		.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
	},

	.set_info = {
		.total_steps = MT9P017_TOTAL_STEPS_NEAR_TO_FAR_MAX,
	},

	.curr_step_pos = 0,
	.curr_region_index = 0,
	.initial_code = INIT_CODE,
	.actuator_mutex = &mt9p017_act_mutex,
	.func_tbl = {
	//	.actuator_set_params = mt9p017_set_params,
		.actuator_init_table = msm_actuator_init_table,
		.actuator_move_focus = msm_actuator_move_focus,
		.actuator_write_focus = msm_actuator_write_focus,
		.actuator_set_default_focus = msm_actuator_set_default_focus,
		.actuator_init_focus = mt9p017_act_init_focus,
		.actuator_i2c_write = mt9p017_wrapper_i2c_write,
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
	.ringing_scenario[MOVE_NEAR] = mt9p017_macro_scenario,
	.scenario_size[MOVE_NEAR] = ARRAY_SIZE(mt9p017_macro_scenario),
	.ringing_scenario[MOVE_FAR] = mt9p017_inf_scenario,
	.scenario_size[MOVE_FAR] = ARRAY_SIZE(mt9p017_inf_scenario),

	/* Initialize region params */
	.region_params = mt9p017_regions,
	.region_size = ARRAY_SIZE(mt9p017_regions),

	/* Initialize damping params */
	.damping[MOVE_NEAR] = mt9p017_macro_regions,
	.damping[MOVE_FAR] = mt9p017_inf_regions,

};

static int mt9p017_vcm_actuator_set_hw(void *data, u64 val)
{
	CDBG("__debug:%s val=0x %x \n", __func__,(u32)val);

    mt9p017_hw_params[0] = (val & 0x000000FF)>>0;
    mt9p017_hw_params[1] = (val & 0x0000FF00)>>8;
    mt9p017_hw_params[2] = (val & 0x00FF0000)>>16;
    mt9p017_hw_params[3] = (val & 0xFF000000)>>24;

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(mt9p017_vcm_hw,
	NULL, mt9p017_vcm_actuator_set_hw, "%llu\n");

static int mt9p017_vcm_actuator_set_mode(void *data, u64 val)
{
	CDBG("__debug:%s val= %d \n", __func__,(int)val);


    vcm_mode_mt9p017 = (val & 0xF);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(mt9p017_vcm_step_time,
	NULL, mt9p017_vcm_actuator_step_time, "%llu\n");


DEFINE_SIMPLE_ATTRIBUTE(mt9p017_vcm_mode,
	NULL, mt9p017_vcm_actuator_set_mode, "%llu\n");

static int mt9p017_vcm_actuator_set_threshold(void *data, u64 val)
{
	CDBG("__debug:%s val= %d \n", __func__,(int)val);


	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(mt9p017_vcm_threshold,
	NULL, mt9p017_vcm_actuator_set_threshold, "%llu\n");

static int mt9p017_vcm_actuator_set_sw(void *data, u64 val)
{
	CDBG("__debug:%s val= %d \n", __func__,(int)val);

    switch((val & 0xF0000000)>>28)
    {
        case 1://s5k4e1gx_inf_reg2_damping

            mt9p017_inf_reg2_damping[(val & 0xF000000) >> 24].damping_step =
                (val & 0xFF0000) >> 16;
            mt9p017_inf_reg2_damping[(val & 0xF000000) >> 24].damping_delay =
                (val & 0xFFFF);
            CDBG("__debug:%s: mt9p017_inf_reg2_damping sence=%d, step: 0x%x, delay: 0x%x\n", __func__,(int)((val & 0xF000000) >> 24),
                mt9p017_inf_reg2_damping[(val & 0xF000000) >> 24].damping_step,
                mt9p017_inf_reg2_damping[(val & 0xF000000) >> 24].damping_delay);
            break;

        case 2://s5k4e1gx_macro_reg2_damping
            mt9p017_macro_reg2_damping[(val & 0xF000000) >> 24].damping_step =
                (val & 0xFF0000) >> 16;
            mt9p017_macro_reg2_damping[(val & 0xF000000) >> 24].damping_delay =
                (val & 0xFFFF);
         CDBG("__debug:%s: mt9p017_macro_reg2_damping sence=%d, step: 0x%x, delay: 0x%x\n", __func__,(int)((val & 0xF000000) >> 24),
             mt9p017_macro_reg2_damping[(val & 0xF000000) >> 24].damping_step,
             mt9p017_macro_reg2_damping[(val & 0xF000000) >> 24].damping_delay);
            break;
            
        case 3://s5k4e1gx_inf_reg1_damping
            mt9p017_inf_reg1_damping[(val & 0xF000000) >> 24].damping_step =
                (val & 0xFF0000) >> 16;
            mt9p017_inf_reg1_damping[(val & 0xF000000) >> 24].damping_delay =
                (val & 0xFFFF);
            CDBG("__debug:%s: mt9p017_inf_reg1_damping sence=%d, step: 0x%x, delay: 0x%x\n", __func__,(int)((val & 0xF000000) >> 24),
                mt9p017_inf_reg1_damping[(val & 0xF000000) >> 24].damping_step,
                mt9p017_inf_reg1_damping[(val & 0xF000000) >> 24].damping_delay);

            break;
            
        case 4:////s5k4e1gx_macro_reg1_damping
            mt9p017_macro_reg1_damping[(val & 0xF000000) >> 24].damping_step =
                (val & 0xFF0000) >> 16;
            mt9p017_macro_reg1_damping[(val & 0xF000000) >> 24].damping_delay =
                (val & 0xFFFF);
            CDBG("__debug:%s: mt9p017_macro_reg1_damping  sence=%d, step: 0x%x, delay: 0x%x\n", __func__,(int)((val & 0xF000000) >> 24),
                mt9p017_macro_reg1_damping[(val & 0xF000000) >> 24].damping_step,
                mt9p017_macro_reg1_damping[(val & 0xF000000) >> 24].damping_delay);
               break;
            
            default:
                
                CDBG("__debug:%s:seting error ! \n", __func__);
                break;

    }


	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(mt9p017_vcm_sw,
	NULL,
	mt9p017_vcm_actuator_set_sw,
	"%llu\n");

static int mt9p017_vcm_move_af(void *data, u64 val)
{
	struct msm_actuator_ctrl_t *a_ctrl = (struct msm_actuator_ctrl_t *)data;
	CDBG("__debug:%s val= %d \n", __func__,(int32_t)val);

	a_ctrl->func_tbl.actuator_move_focus(a_ctrl, (val & 0x100) >> 8,
		val & 0xFF);
	return 0;
}


DEFINE_SIMPLE_ATTRIBUTE(mt9p017_vcm_move,
	NULL, mt9p017_vcm_move_af, "%llu\n");

static uint16_t mt9p017_vcm_linear_step = 1;
static int mt9p017_vcm_set_linear_step(void *data, u64 val)
{
	CDBG("__debug:%s val= %d \n", __func__,(int)val);

	mt9p017_vcm_linear_step = val & 0xFF;
	return 0;
}

static int mt9p017_vcm_af_linearity_test(void *data, u64 *val)
{
	int16_t index = 0;
	struct msm_actuator_ctrl_t *a_ctrl = (struct msm_actuator_ctrl_t *)data;
	CDBG("__debug:%s val= %d \n", __func__,(int)val);

	a_ctrl->func_tbl.actuator_set_default_focus(a_ctrl);
	msleep(1000);

	for (index = 0; index < a_ctrl->set_info.total_steps;
		index+=mt9p017_vcm_linear_step) {
		a_ctrl->func_tbl.actuator_move_focus(a_ctrl, MOVE_NEAR,
			mt9p017_vcm_linear_step);
		CDBG("__debug:debugfs, new loc %d\n", index);
		msleep(1000);
	}

	CDBG("__debug: debugfs moved to macro boundary\n");

	for (index = 0; index < a_ctrl->set_info.total_steps;
		index+=mt9p017_vcm_linear_step) {
		a_ctrl->func_tbl.actuator_move_focus(a_ctrl, MOVE_FAR,
			mt9p017_vcm_linear_step);
		CDBG("debugfs, new loc %d\n", index);
		msleep(1000);
	}

	return 0;
}


DEFINE_SIMPLE_ATTRIBUTE(mt9p017_vcm_linear,
	mt9p017_vcm_af_linearity_test, mt9p017_vcm_set_linear_step, "%llu\n");


static int mt9p017_vcm_set_af_codestep(void *data, u64 val)
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

static int mt9p017_vcm_get_af_codestep(void *data, u64 *val)
{
	struct msm_actuator_ctrl_t *a_ctrl = (struct msm_actuator_ctrl_t *)data;
	CDBG("__debug:%s called\n", __func__);

	*val = a_ctrl->region_params[a_ctrl->region_size-1].code_per_step;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(mt9p017_vcm_codeperstep,
	mt9p017_vcm_get_af_codestep, mt9p017_vcm_set_af_codestep, "%llu\n");

static int mt9p017_actuator_debug_init(void)
{

struct dentry *debugfs_base = debugfs_create_dir("mt9p017_vcm_actuator",
    NULL);
if (!debugfs_base)
    return -ENOMEM;

if (!debugfs_create_file("mt9p017_vcm_linear",
    S_IRUGO | S_IWUSR, debugfs_base, (void *)(&mt9p017_act_t), &mt9p017_vcm_linear))
    return -ENOMEM;

if (!debugfs_create_file("mt9p017_vcm_move",
    S_IRUGO | S_IWUSR, debugfs_base, (void *)(&mt9p017_act_t), &mt9p017_vcm_move))
    return -ENOMEM;

if (!debugfs_create_file("mt9p017_vcm_codeperstep",
    S_IRUGO | S_IWUSR, debugfs_base, (void *)(&mt9p017_act_t), &mt9p017_vcm_codeperstep))
    return -ENOMEM;

if (!debugfs_create_file("mt9p017_vcm_hw",
    S_IRUGO | S_IWUSR, debugfs_base, (void *)(&mt9p017_act_t), &mt9p017_vcm_hw))
    return -ENOMEM;

if (!debugfs_create_file("mt9p017_vcm_sw",
    S_IRUGO | S_IWUSR, debugfs_base, (void *)(&mt9p017_act_t), &mt9p017_vcm_sw))
    return -ENOMEM;

if (!debugfs_create_file("mt9p017_vcm_threshold",
    S_IRUGO | S_IWUSR, debugfs_base, (void *)(&mt9p017_act_t), &mt9p017_vcm_threshold))
    return -ENOMEM;

if (!debugfs_create_file("mt9p017_vcm_step_time",
    S_IRUGO | S_IWUSR, debugfs_base, (void *)(&mt9p017_act_t), &mt9p017_vcm_step_time))
    return -ENOMEM;
	
if (!debugfs_create_file("mt9p017_vcm_mode",
    S_IRUGO | S_IWUSR, debugfs_base, (void *)(&mt9p017_act_t), &mt9p017_vcm_mode))
	return 0;
	return 0;
}
subsys_initcall(mt9p017_i2c_add_driver);
MODULE_DESCRIPTION("MT9P017 actuator");
MODULE_LICENSE("GPL v2");
