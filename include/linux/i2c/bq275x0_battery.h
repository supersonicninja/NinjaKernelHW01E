/*
 * Copyright 2010 HUAWEI Tech. Co., Ltd.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/*
 * Optical Finger Navigation driver
 *
 */
#ifndef __BQ275x0_BATTERY_H
 #define __BQ275x0_BATTERY_H

#include <linux/power_supply.h>

 struct bq275x0_device_info {
	struct device 		*dev;
	int			id;
	struct power_supply	bat;
	struct i2c_client	*client;

	struct  delayed_work test_printk;
	struct  delayed_work hw_config;
    struct  delayed_work notifier_work;
	uint32_t				irq;
	int gpio_config;
    unsigned long timeout_jiffies;
    struct wake_lock low_power_lock;
};

#define BATTERY_LOW_WARNING   1
#define BATTERY_LOW_SHUTDOWN  2
#define BQ275x0_FLAG_SOC1     (1 << 2)
#define BQ275x0_FLAG_SOCF     (1 << 1)
#define BQ275x0_FLAG_LOCK     (BQ275x0_FLAG_SOC1 | BQ275x0_FLAG_SOCF)


typedef void (*bq275x0_notify_handler)(void* callback_param,unsigned int event);

void bq275x0_register_notify_callback(bq275x0_notify_handler PFN,void* param);
int bq275x0_battery_temperature(struct bq275x0_device_info *di);
int bq275x0_battery_temperature_original(struct bq275x0_device_info *di);
int bq275x0_battery_voltage(struct bq275x0_device_info *di);
short bq275x0_battery_current(struct bq275x0_device_info *di);
int bq275x0_battery_capacity(struct bq275x0_device_info *di);
int bq275x0_battery_tte(struct bq275x0_device_info *di);
 int bq275x0_battery_ttf(struct bq275x0_device_info *di);
int is_bq275x0_battery_full(struct bq275x0_device_info *di);
int is_bq275x0_battery_exist(struct bq275x0_device_info *di);
int is_bq275x0_battery_discharging(struct bq275x0_device_info *di);
int bq275x0_battery_flags(struct bq275x0_device_info *di);
int bq275x0_battery_temperature_wrap(void);
#endif
