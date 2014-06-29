/* Copyright (c) 2011, Huawei Technology Inc. All rights reserved.
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
 
#include "ak6921af.h"

static struct i2c_client *ak6921af_client;

static int ak6921af_i2c_read(struct i2c_client *client, uint8_t cmd, uint8_t *data, uint8_t length)
{
	int retry;
	uint8_t addr[1];

	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 1,
			.buf = addr, /*i2c address*/
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = length,
			.buf = data,
		}
	};
	addr[0] = cmd & 0xFF;

	for (retry = 0; retry < AK6921AF_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(client->adapter, msg, 2) == 2)/* read two times,because msg[2] */
			break;
		mdelay(10);
	}
	if (retry == AK6921AF_I2C_RETRY_TIMES) {
		printk(KERN_ERR "i2c_read_block retry over %d\n",
			AK6921AF_I2C_RETRY_TIMES);
		return -EIO;
	}
	return 0;

}

static int ak6921af_i2c_write(struct i2c_client *client, uint8_t cmd, uint8_t *data, uint8_t length)
{
	int retry, loop_i;
	uint8_t buf[length + 1];

	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = length + 1,
			.buf = buf,
		}
	};

	buf[0] = cmd & 0xFF;

	for (loop_i = 0; loop_i < length; loop_i++)
		buf[loop_i + 1] = data[loop_i];

	for (retry = 0; retry < AK6921AF_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(client->adapter, msg, 1) == 1)
			break;
		mdelay(10);
	}

	if (retry == AK6921AF_I2C_RETRY_TIMES) {
		printk(KERN_ERR "i2c_write_block retry over %d\n",
			AK6921AF_I2C_RETRY_TIMES);
		return -EIO;
	}
	return 0;    
}

int ak6921af_write_eeprom(int value)
{
    uint8_t data;
    
    data = 1;
    
    if (ak6921af_i2c_write(ak6921af_client, CMD_REG_EEPROM_SWITCH, &data, 1) < 0) {
        pr_err("change to eeprom mode error, %s\n", __func__);
        return -1;
    }
    
    if (ak6921af_i2c_write(ak6921af_client, CMD_WRITE_ENABLE, NULL, 0) < 0) {
        pr_err("eeprom write enable error, %s\n", __func__);
        return -1;
    }
    
    data = ((uint8_t)value) & 0x1;
    if (ak6921af_i2c_write(ak6921af_client, CMD_OUTPUT, &data, 1) < 0) {
        pr_err("write out reg to set eeprom error, %s\n", __func__);
        return -1;
    }
    mdelay(5);
    
    if (ak6921af_i2c_write(ak6921af_client, CMD_WRITE_DISABLE, NULL, 0) < 0) {
        pr_err("eeprom write disable error, %s\n", __func__);
        return -1;
    }
    
    data = 0;
    if (ak6921af_i2c_write(ak6921af_client, CMD_REG_EEPROM_SWITCH, &data, 1) < 0) {
        pr_err("change to register mode, %s\n", __func__);
        return -1;    
    }
    return 0;
}
EXPORT_SYMBOL(ak6921af_write_eeprom);

int ak6921af_read_eeprom(void)
{
    uint8_t data, result; 
    
    data = 1;
    if (ak6921af_i2c_write(ak6921af_client, CMD_REG_EEPROM_SWITCH, &data, 1) < 0) {
        pr_err("change to eeprom mode error, %s\n", __func__);
        return -1; 
    }
    
    result = 0;    
    if (ak6921af_i2c_read(ak6921af_client, CMD_OUTPUT, &result, 1) < 0) {
        pr_err("read out reg to read eeprom error, %s\n", __func__);
        return -1; 
    }
    result = result & 0x1;
    //printk("ak6921af eeprom value:0x%x\n", result);
    
    data = 0;
    if (ak6921af_i2c_write(ak6921af_client, CMD_REG_EEPROM_SWITCH, &data, 1) < 0) {
        pr_err("change to register mode error, %s\n", __func__);
        return -1;
    }
    
    return result;
}
EXPORT_SYMBOL(ak6921af_read_eeprom);

static int ak6921af_i2c_probe(struct i2c_client *client, const struct i2c_device_id *dev_id)
{
    int ret = 0;
    
    ret = i2c_check_functionality(client->adapter, I2C_FUNC_I2C);
	if (!ret)
	{
		pr_err("i2c_check_functionality failed\n");
		goto probe_failure;
	}
    ak6921af_client = client;    
    
probe_failure:
	pr_err("tps61310_i2c_probe failed! ret = %d\n", ret);
	return ret;
}

static int ak6921af_i2c_remove(struct i2c_client *client)
{
    return 0;
}

static int ak6921af_i2c_suspend(struct i2c_client *client, pm_message_t mesg)
{
    return 0;
}

static int ak6921af_i2c_resume(struct i2c_client *client)
{   
    return 0;
}

static struct i2c_device_id ak6921af_i2c_idtable[] = {
    {"ak6921af", 0},
};

static struct i2c_driver ak6921af_i2c_driver = {
    .driver = {
        .name = "ak6921af",
    },
    .id_table 	= ak6921af_i2c_idtable,
    .probe     = ak6921af_i2c_probe,
    .remove 	= __devexit_p(ak6921af_i2c_remove),

    .suspend	= ak6921af_i2c_suspend,
    .resume 	= ak6921af_i2c_resume,
};

static int __devinit ak6921af_plat_probe(struct platform_device *pdev)
{
    int ret;
    
    ret = i2c_add_driver(&ak6921af_i2c_driver);
    if (ret != 0)
        pr_err("Failed to register ak6921af I2C driver: %d\n", ret);
    
    return ret;
}

static int __devexit ak6921af_plat_remove(struct platform_device *pdev)
{   
    i2c_del_driver(&ak6921af_i2c_driver);
    return 0;
}

static int ak6921af_plat_suspend(struct platform_device *pdev, pm_message_t state)
{    
    return 0;
}

static int ak6921af_plat_resume(struct platform_device *pdev)
{    
    return 0;
}

struct platform_driver ak6921af_plat_driver = {
    .probe = ak6921af_plat_probe,
    .remove	= ak6921af_plat_remove,
    .suspend	= ak6921af_plat_suspend,
    .resume		= ak6921af_plat_resume,
    .driver = {
        .name = "ak6921af",
        .owner = THIS_MODULE,
    },
};

static int __init ak6921af_init_module(void)
{
    return platform_driver_register(&ak6921af_plat_driver);	
}

static void __exit ak6921af_exit_module(void)
{ 
    platform_driver_unregister(&ak6921af_plat_driver);
    return;
}
module_init(ak6921af_init_module);
module_exit(ak6921af_exit_module);
MODULE_DESCRIPTION("ak6921af driver");
MODULE_LICENSE("GPL v2");
