/* drivers/i2c/chips/board-mahimahi-tpa2028d1.c
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
 



/* TODO: content validation in TPA2028_SET_CONFIG */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <hsad/config_interface.h>
#include "board-mahimahi-tpa2028d1.h"

#ifdef TPA2028D1_DEBUG
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#endif

static int is_on;
static struct i2c_client *this_client;
static struct tpa2028d1_platform_data *pdata;

static char spk_amp_cfg[8];
static char spk_amp_on[8] = { /* same length as spk_amp_cfg */
        0x01, 0xc3, 0x05, 0x0b, 0x00, 0x06, 0x3a, 0xc2
};
static const char spk_amp_off[] = {0x01, 0xa2};

static DEFINE_MUTEX(spk_amp_lock);
static int tpa2028d1_opened;
static char *config_data;
static int tpa2028d1_num_modes;

#ifdef TPA2028D1_DEBUG
static struct kobject *tpa2028d1_kobj;
#endif


static int tpa2028_i2c_write(const char *txData, int length)
{
	int ret;
	struct i2c_msg msg = { 
			.addr = this_client->addr, 
			.flags = 0, 
			.buf = (unsigned char*)txData, .len = length 
	};
	ret = i2c_transfer (this_client->adapter, &msg, 1);
		return 0;
}

static int tpa2028_i2c_read(char *rxData, int length)
{
	int ret;
	u8 b0 [] = { 1 };
	struct i2c_msg msg [] = { 
		{ 
			.addr = this_client->addr, 
			.flags = 0, 
			.buf = b0, 
			.len = 1 
		},
		{ 
			.addr = this_client->addr, 
			.flags = I2C_M_RD, 
			.buf = rxData, 
			.len = length 
		} 
	};
	ret = i2c_transfer (this_client->adapter, msg, 2);
	if (ret != 2) return -EIO;
	return 0;
}

static int tpa2028d1_open(struct inode *inode, struct file *file)
{
	int rc = 0;
	mutex_lock(&spk_amp_lock);

	if (tpa2028d1_opened) {
		pr_err("%s: busy\n", __func__);
		rc = -EBUSY;
		goto done;
	}

	tpa2028d1_opened = 1;
done:
	mutex_unlock(&spk_amp_lock);
	return rc;
}

static int tpa2028d1_release(struct inode *inode, struct file *file)
{
	mutex_lock(&spk_amp_lock);
	tpa2028d1_opened = 0;
	mutex_unlock(&spk_amp_lock);
	return 0;
}

static int tpa2028d1_read_config(void __user *argp)
{
	int rc = 0;
	unsigned char reg_idx = 0x01;
	unsigned char tmp[7];

	if (!is_on) {
		gpio_set_value(pdata->gpio_tpa2028_spk_en, 1);
		msleep(5); /* According to TPA2028D1 Spec */
	}

	rc = tpa2028_i2c_write(&reg_idx, sizeof(reg_idx));
	if (rc < 0)
		goto err;

	rc = tpa2028_i2c_read(tmp, sizeof(tmp));
	if (rc < 0)
		goto err;

	if (copy_to_user(argp, &tmp, sizeof(tmp)))
		rc = -EFAULT;

err:
	if (!is_on)
		gpio_set_value(pdata->gpio_tpa2028_spk_en, 0);
	return rc;
}

static long tpa2028d1_ioctl(struct file *file,unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int rc = 0;
	int mode = -1;
	int offset = 0;
	struct tpa2028d1_config_data cfg;

	mutex_lock(&spk_amp_lock);

	switch (cmd) {
	case TPA2028_SET_CONFIG:
		if (copy_from_user(spk_amp_cfg, argp, sizeof(spk_amp_cfg)))
			rc = -EFAULT;
		break;

	case TPA2028_READ_CONFIG:
		rc = tpa2028d1_read_config(argp);
		break;

	case TPA2028_SET_MODE:
		if (copy_from_user(&mode, argp, sizeof(mode))) {
			rc = -EFAULT;
			break;
		}
		if (mode >= tpa2028d1_num_modes || mode < 0) {
			pr_err("%s: unsupported tpa2028d1 mode %d\n",
				__func__, mode);
			rc = -EINVAL;
			break;
		}
		if (!config_data) {
			pr_err("%s: no config data!\n", __func__);
			rc = -EIO;
			break;
		}
		memcpy(spk_amp_cfg, config_data + mode * TPA2028D1_CMD_LEN,
			TPA2028D1_CMD_LEN);
		break;

	case TPA2028_SET_PARAM:
		if (copy_from_user(&cfg, argp, sizeof(cfg))) {
			pr_err("%s: copy from user failed.\n", __func__);
			rc = -EFAULT;
			break;
		}
		tpa2028d1_num_modes = cfg.mode_num;
		if (tpa2028d1_num_modes > TPA2028_NUM_MODES) {
			pr_err("%s: invalid number of modes %d\n", __func__,
				tpa2028d1_num_modes);
			rc = -EINVAL;
			break;
		}
		if (cfg.data_len != tpa2028d1_num_modes*TPA2028D1_CMD_LEN) {
			pr_err("%s: invalid data length %d, expecting %d\n",
				__func__, cfg.data_len,
				tpa2028d1_num_modes * TPA2028D1_CMD_LEN);
			rc = -EINVAL;
			break;
		}
		/* Free the old data */
		if (config_data)
			kfree(config_data);
		config_data = kmalloc(cfg.data_len, GFP_KERNEL);
		if (!config_data) {
			pr_err("%s: out of memory\n", __func__);
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(config_data, cfg.cmd_data, cfg.data_len)) {
			pr_err("%s: copy data from user failed.\n", __func__);
			kfree(config_data);
			config_data = NULL;
			rc = -EFAULT;
			break;
		}
		/* replace default setting with playback setting */
		if (tpa2028d1_num_modes >= TPA2028_MODE_PLAYBACK) {
			offset = TPA2028_MODE_PLAYBACK * TPA2028D1_CMD_LEN;
			memcpy(spk_amp_cfg, config_data + offset,
					TPA2028D1_CMD_LEN);
		}
		break;
		
	case TPA2028_SET_INCALLMODE:
		if (arg) {
		static char spk_amp_call_on[8]; 
		memcpy(spk_amp_call_on, spk_amp_on, sizeof(spk_amp_on));
			if (get_pa_val_by_name("U9201L"))
			{
				spk_amp_call_on[0] = 0x01;
				spk_amp_call_on[1] = 0xc3;
				spk_amp_call_on[2] = 0x05;
				spk_amp_call_on[3] = 0x0b;
				spk_amp_call_on[4] = 0x00;
				spk_amp_call_on[5] = 0x06;
				spk_amp_call_on[6] = 0x5e;
				spk_amp_call_on[7] = 0xc1;
			}
			else if (get_pa_val_by_name("U9501L"))
			{
				spk_amp_call_on[0] = 0x01;
				spk_amp_call_on[1] = 0xc3;
				spk_amp_call_on[2] = 0x05;
				spk_amp_call_on[3] = 0x01;
				spk_amp_call_on[4] = 0x00;
				spk_amp_call_on[5] = 0x13;
				spk_amp_call_on[6] = 0x5b;
				spk_amp_call_on[7] = 0xc0;
			}
			else if (get_pa_val_by_name("C8869L"))
			{
				spk_amp_call_on[0] = 0x01;
				spk_amp_call_on[1] = 0xc3;
				spk_amp_call_on[2] = 0x05;
				spk_amp_call_on[3] = 0x01;
				spk_amp_call_on[4] = 0x00;
				spk_amp_call_on[5] = 0x15;
				spk_amp_call_on[6] = 0x5c;
				spk_amp_call_on[7] = 0xc0;
			}
			else if (get_pa_val_by_name("U9202L"))
			{
				spk_amp_call_on[0] = 0x01;
				spk_amp_call_on[1] = 0xc3;
				spk_amp_call_on[2] = 0x05;
				spk_amp_call_on[3] = 0x0b;
				spk_amp_call_on[4] = 0x00;
				spk_amp_call_on[5] = 0x06;
				spk_amp_call_on[6] = 0x3a;
				spk_amp_call_on[7] = 0xc2;
			}
			memcpy(spk_amp_cfg, spk_amp_call_on, sizeof(spk_amp_call_on));
		}
		else
		{
			memcpy(spk_amp_cfg, spk_amp_on, sizeof(spk_amp_on));
		}
        break;
		
	default:
		pr_err(",%s: invalid command %d\n", __func__, _IOC_NR(cmd));
		rc = -EINVAL;
		break;
	}
	mutex_unlock(&spk_amp_lock);
	return rc;
}

static struct file_operations tpa2028d1_fops = {
	.owner = THIS_MODULE,
	.open = tpa2028d1_open,
	.release = tpa2028d1_release,
	.unlocked_ioctl = tpa2028d1_ioctl,
};

static struct miscdevice tpa2028d1_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "tpa2028d1",
	.fops = &tpa2028d1_fops,
};

void tpa2028d1_set_speaker_effect(int on)
{
	if (!pdata) {
		pr_err("%s: no platform data!\n", __func__);
		return;
	}
	mutex_lock(&spk_amp_lock);
	if (on && !is_on) {
		msleep(5); /* According to TPA2028D1 Spec */

		if (tpa2028_i2c_write(spk_amp_cfg, sizeof(spk_amp_cfg)) == 0) {
			is_on = 1;
			pr_debug("%s: ON\n", __func__);
		}
			 
	} else if (!on && is_on) {
		if (tpa2028_i2c_write(spk_amp_off, sizeof(spk_amp_off)) == 0) {
			is_on = 0;
			msleep(5);
			pr_debug("%s: OFF\n", __func__);
		}
	}
	mutex_unlock(&spk_amp_lock);
}


#ifdef TPA2028D1_DEBUG
/*
 * show and stroe the pa register value 
 */
static ssize_t tpa_reg_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	int rc = 0;
	int i = 0;
	unsigned char tmp[7];
	rc = tpa2028_i2c_read(tmp, sizeof(tmp));
	if (rc < 0)
	{
		printk("tpa_reg_show  tpa2028_i2c_read error \n");
		return  -EINVAL;
	}
	
	for(i = 0;i < 7;i++)
	{
		printk("%x \n",tmp[i]);
	}
	return 0;
}

static ssize_t tpa_reg_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
	int i =0;
	int index = 0;
	int value = 0;
	printk("enter into tpa_reg_store \n");
	sscanf(buf, "%d,%x", &index,&value);

	if((index < 1) ||(index > 7))
	{
		printk("invaild value \n ");
		return -EINVAL;
	}
	
	spk_amp_cfg[index] = value;

	for(i = 0; i < 8; i++){
		printk("spk_amp_cfg[%d] = %x \n",i,spk_amp_cfg[i]);
	}
	
	return count;
}

static struct kobj_attribute tpa_reg_attri =
	__ATTR(tpa_reg, (S_IRUGO|S_IWUSR|S_IWGRP), tpa_reg_show, tpa_reg_store);

static struct attribute *tpa_attrs[] = {
	&tpa_reg_attri.attr,
	NULL,	/* need to NULL terminate the list of attributes */
};

static struct attribute_group tpa_attr_group = {
	.attrs = tpa_attrs,
};
#endif

static int tpa2028d1_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
       #ifdef TPA2028D1_DEBUG
       int tpa_retval;
       #endif
	int ret = 0;	
	pdata = client->dev.platform_data;

	if (!pdata) {
		ret = -EINVAL;
		pr_err("%s: platform data is NULL\n", __func__);
		goto err_no_pdata;
	}

	this_client = client;

	ret = gpio_request(pdata->gpio_tpa2028_spk_en, "tpa2028");
	if (ret < 0) {
		pr_err("%s: gpio request aud_spk_en pin failed\n", __func__);
		goto err_free_gpio;
	}

	ret = gpio_direction_output(pdata->gpio_tpa2028_spk_en, 1);
	if (ret < 0) {
		pr_err("%s: request aud_spk_en gpio direction failed\n",
			__func__);
		goto err_free_gpio;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s: i2c check functionality error\n", __func__);
		ret = -ENODEV;
		goto err_free_gpio;
	}
       
	gpio_set_value_cansleep(pdata->gpio_tpa2028_spk_en, 0); /* Default Low */
       
	ret = misc_register(&tpa2028d1_device);
	if (ret) {
		pr_err("%s: tpa2028d1_device register failed\n", __func__);
	}
	gpio_free(pdata->gpio_tpa2028_spk_en);

	if (get_pa_val_by_name("U9201L"))
	    	{
				spk_amp_on[0] = 0x01;
				spk_amp_on[1] = 0xc3;
				spk_amp_on[2] = 0x02;
				spk_amp_on[3] = 0x10;
				spk_amp_on[4] = 0x08;
				spk_amp_on[5] = 0x10;
				spk_amp_on[6] = 0x3e;
				spk_amp_on[7] = 0xc0;
			}
			else if (get_pa_val_by_name("U9501L"))
	    	{
				spk_amp_on[0] = 0x01;
				spk_amp_on[1] = 0xc3;
				spk_amp_on[2] = 0x05;
				spk_amp_on[3] = 0x01;
				spk_amp_on[4] = 0x00;
				spk_amp_on[5] = 0x13;
				spk_amp_on[6] = 0x5b;
				spk_amp_on[7] = 0xc0;
	    	}
	    	else if (get_pa_val_by_name("C8869L"))
	    	{
				spk_amp_on[0] = 0x01;
				spk_amp_on[1] = 0xc3;
				spk_amp_on[2] = 0x05;
				spk_amp_on[3] = 0x01;
				spk_amp_on[4] = 0x00;
				spk_amp_on[5] = 0x15;
				spk_amp_on[6] = 0x5c;
				spk_amp_on[7] = 0xc0;
	    	}
		else if (get_pa_val_by_name("U9202L"))
		{
				spk_amp_on[0] = 0x01;
				spk_amp_on[1] = 0xc3;
				spk_amp_on[2] = 0x05;
				spk_amp_on[3] = 0x02;
				spk_amp_on[4] = 0x00;
				spk_amp_on[5] = 0x12;
				spk_amp_on[6] = 0x1f;
				spk_amp_on[7] = 0xc0;
		}
	    
	memcpy(spk_amp_cfg, spk_amp_on, sizeof(spk_amp_on));

	#ifdef TPA2028D1_DEBUG
	tpa2028d1_kobj = kobject_create_and_add("tpa_kobj", kernel_kobj);
	if (!tpa2028d1_kobj)
	{
		printk("failed to create tpa2028d1_kobj");
		return -ENOMEM;
	}
       /* Create the files associated with this kobject */
	tpa_retval = sysfs_create_group(tpa2028d1_kobj, &tpa_attr_group);
	if (tpa_retval)
	{	
		printk("failed to create example group");
		kobject_put(tpa2028d1_kobj);
	}
	return tpa_retval;
	#endif
	
	return 0;
err_free_gpio:
	gpio_free(pdata->gpio_tpa2028_spk_en);
err_no_pdata:
	return ret;
}

static int tpa2028d1_suspend(struct i2c_client *client, pm_message_t mesg)
{
	return 0;
}

static int tpa2028d1_resume(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id tpa2028d1_id[] = {
	{ TPA2028D1_I2C_NAME, 0 },
	{ }
};

static struct i2c_driver tpa2028d1_driver = {
	.probe = tpa2028d1_probe,
	.suspend = tpa2028d1_suspend,
	.resume = tpa2028d1_resume,
	.id_table = tpa2028d1_id,
	.driver = {
		.name = TPA2028D1_I2C_NAME,
	},
};

static int __init tpa2028d1_init(void)
{
	pr_info("%s\n", __func__);
	return i2c_add_driver(&tpa2028d1_driver);
}

module_init(tpa2028d1_init);

MODULE_DESCRIPTION("tpa2028d1 speaker amp driver");
MODULE_LICENSE("GPL");

