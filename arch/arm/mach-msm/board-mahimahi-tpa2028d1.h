/* drivers/i2c/chips/board-mahimahi-tpa2028d1.h
 *
 * TI TPA2028D1 Speaker Amplifier
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * Copyright(c) 2011 by HUAWEI, Incorporated.
 * All Rights Reserved. HUAWEI Proprietary and Confidential.
 *
 */
 



#ifndef __ASM_ARM_ARCH_TPA2028D1_H
#define __ASM_ARM_ARCH_TPA2028D1_H

#include <linux/ioctl.h>

#define TPA2028D1_I2C_NAME "tpa2028d1"
#define TPA2028D1_CMD_LEN 8

/*enable it when tuning PA value*/
#define TPA2028D1_DEBUG

struct tpa2028d1_platform_data {
	uint32_t gpio_tpa2028_spk_en;
};

enum tpa2028d1_mode {
	TPA2028_MODE_OFF,
	TPA2028_MODE_PLAYBACK,
	TPA2028_MODE_RINGTONE,
	TPA2028_MODE_VOICE_CALL,
	TPA2028_NUM_MODES,
};

#define TPA2028_IOCTL_MAGIC	'a'
#define TPA2028_SET_CONFIG	_IOW(TPA2028_IOCTL_MAGIC, 1, unsigned)
#define TPA2028_READ_CONFIG	_IOR(TPA2028_IOCTL_MAGIC, 2, unsigned)
#define TPA2028_SET_PARAM	_IOW(TPA2028_IOCTL_MAGIC, 3, unsigned)
#define TPA2028_SET_MODE	_IOW(TPA2028_IOCTL_MAGIC, 4, unsigned)
#define TPA2028_SET_INCALLMODE	_IOW(TPA2028_IOCTL_MAGIC, 5, unsigned)

struct tpa2028d1_config_data {
	unsigned char *cmd_data;  /* [mode][cmd_len][cmds..] */
	unsigned int mode_num;
	unsigned int data_len;
};

extern void tpa2028d1_set_speaker_effect(int on);

MODULE_DESCRIPTION("tpa2028d1 speaker amp driver");
MODULE_LICENSE("GPL");
#endif /* __ASM_ARM_ARCH_TPA2028D1_H */


