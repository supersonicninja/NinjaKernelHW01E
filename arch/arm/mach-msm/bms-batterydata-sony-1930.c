/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/mfd/pm8xxx/pm8921-bms.h>


static struct single_row_lut palladium_1930_fcc_temp = {
	.x		= {-20, 0, 25, 40, 60},
	.y		= {1931, 1957, 1966, 1966, 1960},
	.cols	= 5
};

static struct single_row_lut palladium_1930_fcc_sf = {
	.x		= {0},
	.y		= {100},
	.cols	= 1
};

static struct sf_lut palladium_1930_pc_sf = {
	.rows		= 1,
	.cols		= 1,
	.row_entries		= {0},
	.percent	= {100},
	.sf			= {
				{100}
	}
};

static struct sf_lut palladium_1930_rbatt_sf = {
        .rows           = 28, 
        .cols           = 5,
        /* row_entries are temperature */
        .row_entries            = {-20, 0, 25, 40, 60},
        .percent        = {100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 45, 40, 35, 30, 25, 20, 15, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1},
        .sf                     = {

{493,183,100,88,90},
{556,190,102,90,92},
{587,192,104,92,93},
{620,194,105,93,94},
{653,195,106,94,95},
{688,195,106,95,96},
{719,197,104,95,96},
{753,198,104,92,94},
{781,199,104,91,94},
{812,200,104,91,94},
{850,201,105,92,94},
{896,203,106,93,95},
{955,207,107,94,96},
{1030,212,108,95,96},
{1134,218,109,94,95},
{1282,228,110,95,96},
{1549,248,110,95,97},
{2184,296,112,97,98},
{1010,194,102,91,93},
{1064,195,103,92,94},
{1143,198,105,93,95},
{1260,201,106,94,95},
{1446,205,106,94,95},
{1732,210,106,93,94},
{2266,220,107,94,94},
{3492,234,108,95,95},
{6440,263,111,98,98},
{12425,323,117,104,103},


        }
};

static struct pc_temp_ocv_lut palladium_1930_pc_temp_ocv = {
	.rows		= 29,
	.cols		= 5,
	.temp		= {-20, 0, 25, 40, 60},
	.percent	= {100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 45, 40, 35, 30, 25, 20, 15, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
	.ocv		= {
				{4171, 4170, 4166, 4161, 4153},
				{4085, 4095, 4097, 4094, 4089},
				{4030, 4045, 4051, 4048, 4044},
				{3984, 4002, 4011, 4009, 4006},
				{3942, 3963, 3975, 3974, 3971},
				{3906, 3927, 3941, 3943, 3940},
				{3873, 3894, 3908, 3913, 3912},
				{3842, 3865, 3875, 3884, 3885},
				{3816, 3840, 3848, 3851, 3854},
				{3793, 3817, 3826, 3826, 3824},
				{3772, 3797, 3807, 3807, 3804},
				{3755, 3778, 3792, 3792, 3790},
				{3739, 3763, 3779, 3780, 3778},
				{3725, 3751, 3769, 3770, 3766},
				{3710, 3739, 3759, 3757, 3749},
				{3692, 3725, 3745, 3738, 3726},
				{3670, 3710, 3724, 3717, 3705},
				{3639, 3690, 3697, 3691, 3680},
				{3599, 3663, 3664, 3661, 3653},
				{3592, 3654, 3659, 3657, 3649},
				{3581, 3644, 3653, 3651, 3643},
				{3568, 3631, 3644, 3642, 3633},
				{3550, 3615, 3626, 3622, 3612},
				{3527, 3591, 3594, 3589, 3578},
				{3494, 3555, 3550, 3545, 3534},
				{3447, 3503, 3496, 3490, 3480},
				{3382, 3432, 3425, 3422, 3414},
				{3298, 3336, 3331, 3330, 3327},
				{3200, 3200, 3200, 3200, 3200}
	}
};

struct pm8921_bms_battery_data palladium_1930_data = {
	.fcc				= 1930,
	.fcc_temp_lut		= &palladium_1930_fcc_temp,
	.fcc_sf_lut		= &palladium_1930_fcc_sf,
	.pc_temp_ocv_lut	= &palladium_1930_pc_temp_ocv,
	.pc_sf_lut		= &palladium_1930_pc_sf,
	.rbatt_sf_lut		= &palladium_1930_rbatt_sf,
	.default_rbatt_mohm		= 176,
};


