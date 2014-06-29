/* Copyright (c) 2010-2011, Code Aurora Forum. All rights reserved.
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
#include <linux/bitops.h>
#include <linux/io.h>
#include <mach/msm_iomap.h>
#include <mach/gpiomux.h>
#ifdef CONFIG_HUAWEI_KERNEL
#include <linux/module.h>
#endif

#define GPIO_CFG(n)    (MSM_TLMM_BASE + 0x1000 + (0x10 * n))
#define GPIO_IN_OUT(n) (MSM_TLMM_BASE + 0x1004 + (0x10 * n))

#ifdef CONFIG_HUAWEI_KERNEL
#define GPIO_OUT_0 (MSM_TLMM_BASE + 0x3000)
#define GPIO_OUT_1 (MSM_TLMM_BASE + 0x3004)
#define GPIO_OUT_2 (MSM_TLMM_BASE + 0x3008)
#define GPIO_OUT_3 (MSM_TLMM_BASE + 0x300C)
#define GPIO_OUT_4 (MSM_TLMM_BASE + 0x3010)
#define GPIO_OUT_5 (MSM_TLMM_BASE + 0x3014)
#endif

void __msm_gpiomux_write(unsigned gpio, struct gpiomux_setting val)
{
	uint32_t bits;

	bits = (val.drv << 6) | (val.func << 2) | val.pull;
	if (val.func == GPIOMUX_FUNC_GPIO) {
		bits |= val.dir > GPIOMUX_IN ? BIT(9) : 0;
		__raw_writel(val.dir == GPIOMUX_OUT_HIGH ? BIT(1) : 0,
			GPIO_IN_OUT(gpio));
	}
	__raw_writel(bits, GPIO_CFG(gpio));
	mb();
}

#ifdef CONFIG_HUAWEI_KERNEL
unsigned gpio_func_config_read(unsigned gpio)
{
	int gpio_cfg;
	gpio_cfg = __raw_readl(GPIO_CFG(gpio));
	return gpio_cfg;
}
EXPORT_SYMBOL(gpio_func_config_read);

void gpio_func_config_write(unsigned gpio,uint32_t bits)
{
	__raw_writel(bits, GPIO_CFG(gpio));
}
EXPORT_SYMBOL(gpio_func_config_write);

/*
 *return 1----out_put,return 0----in_put & readonly
 */
unsigned gpio_in_out_config_read(unsigned gpio)
{
	int gpio_cfg;
	gpio_cfg = __raw_readl(GPIO_IN_OUT(gpio));
	return gpio_cfg;
}
EXPORT_SYMBOL(gpio_in_out_config_read);

void gpio_in_out_config_write(unsigned gpio,uint32_t bits)
{
	__raw_writel(bits, GPIO_IN_OUT(gpio));
}
EXPORT_SYMBOL(gpio_in_out_config_write);

void gpio_0_out_config_write(uint32_t bits)
{
	__raw_writel(bits, GPIO_OUT_0);
}
EXPORT_SYMBOL(gpio_0_out_config_write);

#endif
