/************************************************************* 
* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
* hw-uniregulator.h
* Make regulator resources unitary according to Board-Id by HUAWEI CO.
*************************************************************/
#ifndef _CONFIG_HW_UNIREGULATOR_
#define _CONFIG_HW_UNIREGULATOR_
#include "machine.h"
#include "consumer.h"
#include "pm8xxx-regulator.h"
#include "gpio-regulator.h"
#include <mach/rpm-regulator.h>

struct hw_config_power_tree
{
   struct rpm_regulator_init_data *rpm_power_pdata;
   struct pm8xxx_regulator_platform_data *pmic_power_pdata;
   struct regulator_init_data *saw_power_pdata;
   struct gpio_regulator_platform_data *gpio_power_pdata;
   int rpm_reg_num;
   int pmic_reg_num;
   int saw_reg_num;
   int gpio_reg_num;
};

#endif
