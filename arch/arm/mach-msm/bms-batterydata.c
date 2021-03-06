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

static struct single_row_lut palladium_1500_fcc_temp = {
	.x		= {-30, -20, -10, 0, 10, 25, 40, 65},
	.y		= {1658, 1666, 1668, 1669, 1671, 1671, 1673, 1675},
	.cols	= 8
};

static struct single_row_lut palladium_1500_fcc_sf = {
	.x		= {0},
	.y		= {100},
	.cols	= 1
};

static struct sf_lut palladium_1500_pc_sf = {
	.rows		= 1,
	.cols		= 1,
	.row_entries		= {0},
	.percent	= {100},
	.sf			= {
				{100}
	}
};

static struct sf_lut palladium_1500_rbatt_sf = {
        .rows           = 28, 
        .cols           = 8,
        /* row_entries are temperature */
        .row_entries            = {-30, -20, -10, 0, 10, 25, 40, 65},
        .percent        = {100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 45, 40, 35, 30, 25, 20, 15, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1},
        .sf                     = {

{805,388,268,206,138,100,87,79},
{809,396,264,204,140,103,89,81},
{823,393,258,201,141,104,91,83},
{843,400,255,197,141,105,92,84},
{865,406,254,195,141,106,93,85},
{893,411,253,194,141,106,94,86},
{940,424,255,195,137,106,94,86},
{1003,443,258,195,136,105,93,86},
{1077,470,264,196,136,99,87,81},
{1163,502,273,197,137,99,88,81},
{1275,540,288,203,139,100,89,83},
{1409,585,308,211,144,102,90,85},
{1563,636,333,220,151,105,92,86},
{1741,696,361,231,159,109,94,86},
{1936,773,393,240,167,112,96,83},
{2233,872,436,248,174,113,95,85},
{2705,1020,491,268,179,111,93,84},
{3557,1280,592,321,209,114,93,84},
{2866,850,379,222,155,104,89,83},
{3127,918,399,225,156,105,90,83},
{3513,1022,428,238,160,106,91,84},
{4007,1154,471,254,166,108,92,85},
{4620,1351,531,277,173,110,93,85},
{5463,1640,618,306,182,110,92,84},
{6643,2085,749,348,195,110,91,83},
{8405,2778,963,422,222,115,93,85},
{11033,3875,1297,536,262,123,97,89},
{15461,6760,1857,729,330,143,103,93},
}
};

static struct pc_temp_ocv_lut palladium_1500_pc_temp_ocv = {
	.rows		= 29,
	.cols		= 8,
	.temp		= {-30, -20, -10, 0, 10, 25, 40, 65},
	.percent	= {100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 45, 40, 35, 30, 25, 20, 15, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
	.ocv		= {
				{4173, 4172, 4171, 4169, 4169, 4166, 4163, 4158},
				{4089, 4100, 4107, 4110, 4112, 4111, 4110, 4107},
				{4011, 4049, 4061, 4066, 4068, 4067, 4066, 4062},
				{3955, 3987, 4011, 4020, 4025, 4027, 4026, 4022},
				{3914, 3945, 3964, 3978, 3988, 3990, 3988, 3984},
				{3879, 3909, 3931, 3941, 3955, 3957, 3955, 3951},
				{3852, 3874, 3897, 3907, 3924, 3927, 3926, 3920},
				{3830, 3845, 3866, 3876, 3884, 3900, 3899, 3893},
				{3811, 3821, 3839, 3849, 3852, 3869, 3870, 3865},
				{3795, 3801, 3815, 3824, 3827, 3827, 3827, 3826},
				{3781, 3786, 3794, 3803, 3806, 3806, 3806, 3804},
				{3766, 3772, 3776, 3788, 3789, 3791, 3789, 3789},
				{3752, 3760, 3762, 3776, 3777, 3778, 3777, 3776},
				{3737, 3749, 3751, 3768, 3771, 3771, 3767, 3765},
				{3722, 3737, 3740, 3761, 3765, 3765, 3760, 3751},
				{3706, 3725, 3728, 3749, 3757, 3755, 3749, 3729},
				{3686, 3712, 3717, 3724, 3736, 3735, 3726, 3708},
				{3661, 3695, 3703, 3701, 3701, 3700, 3692, 3677},
				{3625, 3671, 3687, 3686, 3679, 3674, 3667, 3654},
				{3616, 3664, 3682, 3681, 3676, 3672, 3666, 3652},
				{3604, 3655, 3675, 3677, 3673, 3670, 3664, 3650},
				{3591, 3644, 3668, 3672, 3670, 3667, 3661, 3648},
				{3575, 3630, 3659, 3666, 3666, 3662, 3656, 3640},
				{3556, 3609, 3643, 3655, 3656, 3649, 3639, 3616},
				{3532, 3582, 3616, 3626, 3626, 3614, 3600, 3574},
				{3499, 3544, 3571, 3576, 3572, 3559, 3545, 3519},
				{3451, 3485, 3505, 3503, 3500, 3488, 3477, 3453},
				{3371, 3388, 3402, 3400, 3401, 3393, 3382, 3359},
				{3200, 3200, 3200, 3200, 3200, 3200, 3200, 3200}
	}
};

struct pm8921_bms_battery_data palladium_1500_data = {
	.fcc			= 1650,
	.fcc_temp_lut		= &palladium_1500_fcc_temp,
	.fcc_sf_lut		= &palladium_1500_fcc_sf,
	.pc_temp_ocv_lut	= &palladium_1500_pc_temp_ocv,
	.pc_sf_lut		= &palladium_1500_pc_sf,
	.rbatt_sf_lut		= &palladium_1500_rbatt_sf,
    .default_rbatt_mohm        =236,

};

