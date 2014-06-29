/*
 * BQ275x0 battery driver
 *
 * Copyright (C) 2008 Rodolfo Giometti <giometti@linux.it>
 * Copyright (C) 2008 Eurotech S.p.A. <info@eurotech.it>
 *
 * Based on a previous work by Copyright (C) 2008 Texas Instruments, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */


#include <linux/module.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <linux/idr.h>
#include <linux/i2c.h>
#include <asm/unaligned.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <linux/module.h> 
#include <linux/i2c/bq275x0_battery.h>
#include <linux/io.h>
#include <linux/uaccess.h> 
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/ctype.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/mfd/pm8xxx/pm8921-charger.h>
#include <hsad/config_interface.h>
#define DRIVER_VERSION			"1.0.0"
/*====================== Configuration Begin ===================== */
#define DISABLE_ACCESS_TIME                 3000
#define SET_FLAG_TIME                       2000
#define TEMPERATURE_CONVERT(reg_val)	    (reg_val-2730)
#define BATTERY_IS_ALWAYS_PRESENT

#define LOCAL_ASSERT(exp) if(!exp)dump_stack()
/*====================== Configuration end   ===================== */
#define BQ275x0_REG_TEMP	0x06
#define BQ275x0_REG_VOLT	0x08
#define BQ275x0_REG_FLAGS 0x0a
#define BQ275x0_REG_TTE		0x16		/*Time to Empty*/
#define BQ275x0_REG_TTF		0x18		/*Time to Full*/
#define BQ275x0_REG_SOC		0x2C		/* State-of-Charge */
#define BQ275x0_REG_AI		0x14		/*Average Current*/
#define BQ275x0_REG_RM        0x10          /*Remainning Capacity*/
#define BQ275x0_REG_FCC       0x12          /*Full Charge Capacity*/
#define BQ275x0_REG_SI          0x1a            /*Standby Current*/
#define BQ275x0_REG_CTRL     0x00          /*Control*/
#define BQ275x0_REG_CTRS     0x0000      /*Control Status*/
#define BQ275x0_REG_DFCLS    0x3e       /*Data Flash Class*/
#define BQ275x0_REG_CLASS_ID   82
#define BQ275x0_REG_QMAX     0x42
#define BQ275x0_REG_QMAX1   0x43
#define BQ275x0_REG_FLASH     0X40
#define BQ275x0_REG_FIRMWARE_ID     0X0008
#define BQ275x0_REG_FIRMWARE_VERSION     0X0039
#define BQ275x0_REG_DFBLK    0x3f       /*Data Flash Block*/
#define BQ275x0_REG_BLKDAT    0x40       /*Block  data  */
#define BQ275x0_REG_BLKDAT1    0x00       /*Block  data  */
#define BQ275x0_REG_CHKSUM    0x60       /*Blockdata  Checksum */
#define BQ275x0_REG_BLKDAT2    0x41       /*Block  data2  */
#define BQ275x0_NOSLEEP_SETTING    0xdf
#define BQ275x0_SLEEP_SETTING        0x20
#define BQ275x0_DATA_RESET    0x0041       /*RESET	ALL */
#define BQ275x0_REG_RESET    0x00


#define BQ275x0_FLAG_FC		    1<<9		/* Full-charged bit */
#define BQ275x0_FLAG_DET		1<<3
#define BQ275x0_FLAG_DISCHARGE  1
#define BQ275x0_FLAG_OTC		1<<15		/* Over-Temperature-Charge bit */
#define BQ275x0_FLAG_OTD		1<<14       /* Over-Temperature-Discharge bit */

#define CONST_NUM_10		      10
#define CONST_NUM_2730		      2730
#define TEMP_K2C(temp_unit_p1)    (temp_unit_p1-CONST_NUM_2730)
#define TEMP_C2K(temp_unit_p1)    (temp_unit_p1+CONST_NUM_2730)

#define BATTERY_TEMPERATURE_COLD  -200      /*this number is equal to -20 degree * 10*/

#define BSP_UPGRADE_FIRMWARE_BQFS_CMD       "upgradebqfs"
#define BSP_UPGRADE_FIRMWARE_DFFS_CMD       "upgradedffs"
/*库仑计的完整的firmware，包含可执行镜像及数据*/
#define BSP_UPGRADE_FIRMWARE_BQFS_NAME      "/system/etc/coulometer/bq27510_pro.bqfs"
/*库仑计的的数据信息*/
#define BSP_UPGRADE_FIRMWARE_DFFS_NAME      "/system/etc/coulometer/bq27510_pro.dffs"

#define BSP_ROM_MODE_I2C_ADDR               0x0B
#define BSP_NORMAL_MODE_I2C_ADDR            0x55
#define BSP_FIRMWARE_FILE_SIZE              (400*1024)
#define BSP_I2C_MAX_TRANSFER_LEN            128
#define BSP_MAX_ASC_PER_LINE                400
#define BSP_ENTER_ROM_MODE_CMD              0x00
#define BSP_ENTER_ROM_MODE_DATA             0x0F00
#define BSP_FIRMWARE_DOWNLOAD_MODE          0xDDDDDDDD
#define BSP_NORMAL_MODE                     0x00
#define FIRMWARE_FULLNAME_LEN               48
#define MAGIC_NUMBER                        0x12345678
#define GAS_GAUGE_FIRMWARE_NAME             32
#define FIRMWARE_VERSION_ID_LEN             12
#define STATE_HARDWARE_ERR                  (-2)
#define FIRMWARE_STORE_PATH                 "/system/etc/batt_fw.bin"

typedef unsigned int uint32;
struct firmware_header_entry
{
    uint32 magic_number;
    char   file_name[FIRMWARE_FULLNAME_LEN];
    uint32 offset;
    uint32 length;
};



#if defined(BQ275x0_DEBUG_FLAG)
#define BQ275x0_DBG(format,arg...)     do { printk(KERN_ALERT format, ## arg);  } while (0)
#define BQ275x0_ERR(format,arg...)	  do { printk(KERN_ERR format, ## arg);  } while (0)
#define BQ275x0_FAT(format,arg...)     do { printk(KERN_CRIT format, ## arg); } while (0)
#else
#define BQ275x0_DBG(format,arg...)     do { (void)(format); } while (0)
#define BQ275x0_ERR(format,arg...)	  do { (void)(format); } while (0)
#define BQ275x0_FAT(format,arg...)	  do { (void)(format); } while (0)
#endif

//#define CONFIG_BQ27520_TEST_ENABLE
#define BQ275x0_INIT_DELAY ((HZ)*1)
#define DELAY_TEST_PRINTK ((HZ)*30)
struct bq275x0_device_info *g_bq275x0_device_info_di=NULL;
#ifdef CONFIG_BQ27520_TEST_ENABLE
static void bq275x0_test_printk(struct work_struct *work);
#endif
#define GPIO_GAUGE_INT  42

static DEFINE_IDR(bq275x0_battery_id);
static DEFINE_MUTEX(bq275x0_battery_mutex);


static enum power_supply_property bq275x0_battery_props[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW,
	POWER_SUPPLY_PROP_TIME_TO_FULL_NOW,
};
struct i2c_client* g_battery_measure_by_bq275x0_i2c_client = NULL;


static struct i2c_driver bq275x0_battery_driver;
/*在固件下载器件不允许Bq275x0其他类型的I2C操作*/
static unsigned int gBq275x0DownloadFirmwareFlag = BSP_NORMAL_MODE;


/*
 * Common code for bq275x0 devices
 */
 enum
{
	BQ275x0_NORMAL_MODE,
	BQ275x0_UPDATE_FIRMWARE,
	BQ275x0_LOCK_MODE
};

struct bq275x0_context
{
    unsigned int temperature;
	unsigned int capacity;
	unsigned int volt;
	unsigned int bat_current;
	unsigned int remaining_capacity;
	unsigned int battery_present;
	unsigned char state;
	unsigned long locked_timeout_jiffies;//after updating firmware, device can not be accessed immediately
	unsigned int i2c_error_count;
	unsigned int lock_count;

	bq275x0_notify_handler pfn_notify_call;
	void* callback_param_p;
};

struct bq275x0_context gauge_context =
{
	.temperature = TEMP_C2K(100),//10*10 degree
	.capacity = 50,//50 percent
	.volt = 3700,// 3.7 V
	.bat_current = 200,// 200 mA
	.remaining_capacity = 800,//mAH
    .battery_present = 1,
	.state = BQ275x0_NORMAL_MODE,
	.i2c_error_count = 0
};

#define GAS_GAUGE_I2C_ERR_STATICS() ++gauge_context.i2c_error_count
#define GAS_GAUGE_LOCK_STATICS() ++gauge_context.lock_count

#ifdef BATTERY_IS_ALWAYS_PRESENT
#define is_bq275x0_battery_exist(dummy)  1
#endif
static int bq275x0_is_accessible(void)
{
	if(gauge_context.state == BQ275x0_UPDATE_FIRMWARE)
	{
		BQ275x0_DBG("bq275x0 isn't accessible,It's updating firmware!\n");
		GAS_GAUGE_LOCK_STATICS();
		return 0;
	}
	else if(gauge_context.state == BQ275x0_NORMAL_MODE)return 1;
	else //if(gauge_context.state == BQ275x0_LOCK_MODE)
	{
		if(time_is_before_jiffies(gauge_context.locked_timeout_jiffies))
		{
			gauge_context.state = BQ275x0_NORMAL_MODE;
			return 1;
		}
		else
		{
			BQ275x0_DBG("bq275x0 isn't accessible after firmware updated immediately!\n");
			return 0;
		}
	}
}
static int bq275x0_i2c_read_word(struct bq275x0_device_info *di,u8 reg)
{
	int err;
//    if(BSP_FIRMWARE_DOWNLOAD_MODE == gBq275x0DownloadFirmwareFlag)
//    {
//        return -1;
//    }
	mutex_lock(&bq275x0_battery_mutex);
	err = i2c_smbus_read_word_data(di->client,reg);
	if (err < 0) {
		GAS_GAUGE_I2C_ERR_STATICS();
		BQ275x0_ERR("[%s,%d] i2c_smbus_read_byte_data failed\n",__FUNCTION__,__LINE__);
       }
	mutex_unlock(&bq275x0_battery_mutex);

	return err;
}


static int bq275x0_i2c_word_write(struct i2c_client *client, u8 reg, u16 value)
{
	int err;

	mutex_lock(&bq275x0_battery_mutex);
    err = i2c_smbus_write_word_data(client, reg, value);
	if (err < 0) 
    {
		GAS_GAUGE_I2C_ERR_STATICS();
		BQ275x0_ERR("[%s,%d] i2c_smbus_write_word_data failed\n",__FUNCTION__,__LINE__);
    }
	mutex_unlock(&bq275x0_battery_mutex);

	return err;
}

static int bq275x0_i2c_bytes_write(struct i2c_client *client, u8 reg, u8 *pBuf, u16 len)
{
	int i2c_ret, i,j;
    u8 *p;

    p = pBuf;

    mutex_lock(&bq275x0_battery_mutex);
    for(i=0; i<len; i+=I2C_SMBUS_BLOCK_MAX)
    {
        j = ((len - i) > I2C_SMBUS_BLOCK_MAX) ? I2C_SMBUS_BLOCK_MAX : (len - i);
        i2c_ret = i2c_smbus_write_i2c_block_data(client, reg+i, j, p+i);
        if (i2c_ret < 0) 
        {
    		GAS_GAUGE_I2C_ERR_STATICS();
			BQ275x0_ERR("[%s,%d] i2c_transfer failed\n",__FUNCTION__,__LINE__);
			break;
    	}
    }
    mutex_unlock(&bq275x0_battery_mutex);

	return i2c_ret;
}


static int bq275x0_i2c_bytes_read(struct i2c_client *client, u8 reg, u8 *pBuf, u16 len)
{
	int i2c_ret = 0, i = 0, j = 0;
    u8 *p;

    p = pBuf;

    mutex_lock(&bq275x0_battery_mutex);
    for(i=0; i<len; i+=I2C_SMBUS_BLOCK_MAX)
    {
        j = ((len - i) > I2C_SMBUS_BLOCK_MAX) ? I2C_SMBUS_BLOCK_MAX : (len - i);
        i2c_ret = i2c_smbus_read_i2c_block_data(client, reg+i, j, p+i);
        if (i2c_ret < 0) 
        {
			GAS_GAUGE_I2C_ERR_STATICS();
    		BQ275x0_ERR("[%s,%d] i2c_transfer failed\n",__FUNCTION__,__LINE__);
			break;
    	}
    }
    mutex_unlock(&bq275x0_battery_mutex);

	return i2c_ret;
    
}


static int bq275x0_i2c_bytes_read_and_compare(struct i2c_client *client, u8 reg, u8 *pSrcBuf, u8 *pDstBuf, u16 len)
{
    int i2c_ret;

    i2c_ret = bq275x0_i2c_bytes_read(client, reg, pSrcBuf, len);
    if(i2c_ret < 0)
    {
		GAS_GAUGE_I2C_ERR_STATICS();
        BQ275x0_ERR("[%s,%d] bq275x0_i2c_bytes_read failed\n",__FUNCTION__,__LINE__);
        return i2c_ret;
    }

    i2c_ret = strncmp(pDstBuf, pSrcBuf, len);

    return i2c_ret;
}


/*
 * Return the battery temperature in Celcius degrees
 * Or < 0 if something fails.
 */
int bq275x0_battery_temperature(struct bq275x0_device_info *di)
{
	int data = -1;

	int batt_therm =0;
	if(bq275x0_is_accessible())
	{
		if (is_bq275x0_battery_exist(di))
		{
			data = bq275x0_i2c_read_word(di,BQ275x0_REG_TEMP);
			if(data<0)BQ275x0_ERR("i2c error in reading temperature!");
			else gauge_context.temperature = data;
		}
	}
	if(data < 0)
	{
    	data = gauge_context.temperature;
	}

	data = TEMP_K2C(data);
	
	batt_therm = batt_therm_filter(data);
	BQ275x0_DBG("read temperature result = %d Celsius\n",batt_therm);
	return batt_therm;
}

int bq275x0_battery_temperature_original(struct bq275x0_device_info *di)
{
	int data=0;

	data = bq275x0_i2c_read_word(di,BQ275x0_REG_TEMP);
	//data = (data-CONST_NUM_2730)/CONST_NUM_10;
	//BQ275x0_DBG("read temperature result = %d Celsius\n",data);
	return data;
}

/*
 * Return the battery Voltage in milivolts
 * Or < 0 if something fails.
 */
int bq275x0_battery_voltage(struct bq275x0_device_info *di)
{
	int data = -1;

	if(bq275x0_is_accessible()){
		data = bq275x0_i2c_read_word(di,BQ275x0_REG_VOLT);
		if(data<0)BQ275x0_ERR("i2c error in reading voltage!");
		else gauge_context.volt = data;
	}
	if(data < 0)data = gauge_context.volt;
	BQ275x0_DBG("read voltage result = %d mvolts\n",data);
	return data;
}

/*
 * Return the battery average current
 * Note that current can be negative signed as well
 * Or 0 if something fails.
 */
 short bq275x0_battery_current(struct bq275x0_device_info *di)
{
	int data = -1;
	short nCurr = 0;

	if(bq275x0_is_accessible()){
		data = bq275x0_i2c_read_word(di,BQ275x0_REG_AI);
		if(data<0)BQ275x0_ERR("i2c error in reading current!");
		else gauge_context.bat_current = data;
		}
	if(data < 0)data = gauge_context.bat_current;
	nCurr = (signed short )data;
	BQ275x0_DBG("read current result = %d mA\n",nCurr);
	return nCurr;
}
/*
 * Return the battery Relative State-of-Charge
 * Or < 0 if something fails.
 */
int bq275x0_battery_capacity(struct bq275x0_device_info *di)
{
	int data = 0;
	int battery_voltage = 0;
	if(!bq275x0_is_accessible())return gauge_context.capacity;
	if (!is_bq275x0_battery_exist(di))
	{
		battery_voltage = bq275x0_battery_voltage(di)*1000;  //uV
		if (battery_voltage < 3500000)
			data = 5;
		else if (battery_voltage < 3600000 && battery_voltage >= 3500000)/*3500000 = 3.5V*/
			data = 20;
		else if (battery_voltage < 3700000 && battery_voltage >= 3600000)/*3600000 = 3.6V*/
			data = 50;
		else if (battery_voltage < 3800000 && battery_voltage >= 3700000)/*3700000 = 3.7V*/
			data = 75;
		else if (battery_voltage < 3900000 && battery_voltage >= 3800000)/*3800000 = 3.8V*/
			data = 90;
		else if (battery_voltage >= 3900000) {
			data = 100;
		}
		return data;
	}
	else
	{
			if(bq275x0_is_accessible())
			{
				data = bq275x0_i2c_read_word(di,BQ275x0_REG_SOC);
				if(data < 0)
				{
					BQ275x0_ERR("i2c error in reading capacity!");
					data = gauge_context.capacity;
				}
				else gauge_context.capacity = data;
			}
			else data=gauge_context.capacity;
	}
	BQ275x0_DBG("read soc result = %d Hundred Percents\n",data);
	return data;
}

int bq275x0_battery_health(struct bq275x0_device_info *di)
{
	int data=0;
	int status =0;
	int battery_temperature = 0;

	if(!bq275x0_is_accessible())
	{
	    return POWER_SUPPLY_HEALTH_UNKNOWN;
	}
	
	if (is_bq275x0_battery_exist(di))
	{
        data = bq275x0_i2c_read_word(di,BQ275x0_REG_FLAGS);
        if(data < 0)  //if i2c read error
        {
            return POWER_SUPPLY_HEALTH_GOOD;
        }
        
        battery_temperature = bq275x0_battery_temperature(di);

        //printk("read health result=%d ,temperature=%d\n",data,battery_temperature);
		if (battery_temperature < BATTERY_TEMPERATURE_COLD)
		{
			status = POWER_SUPPLY_HEALTH_COLD;
		}
		else if (data & (BQ275x0_FLAG_OTC | BQ275x0_FLAG_OTD))
        {
    		status = POWER_SUPPLY_HEALTH_OVERHEAT;
		}
        else
        {
    		status = POWER_SUPPLY_HEALTH_GOOD;
		}
	}
	else
	{
    	status = POWER_SUPPLY_HEALTH_UNKNOWN;
	}

	return status;
}


/*
 * Return the battery Relative State-of-Charge
 * Or < 0 if something fails.
 */
int bq275x0_battery_capacity_rm(struct bq275x0_device_info *di)
{
	int data=0;

	if(!bq275x0_is_accessible())
	{
	    return gauge_context.remaining_capacity;
	}

	data = bq275x0_i2c_read_word(di,BQ275x0_REG_RM);
	if(data < 0)
	{
	    BQ275x0_ERR("i2c error in reading remain capacity!");    
        data = gauge_context.remaining_capacity;
	}
	else
	{
	    gauge_context.remaining_capacity = data;
	}

	BQ275x0_DBG("read soc result = %d mAh\n",data);
	return data;
}

int bq27510_battery_fcc(struct bq275x0_device_info *di)
{
	int data=0;
	if(!bq275x0_is_accessible())return 0;
	data = bq275x0_i2c_read_word(di,BQ275x0_REG_FCC);

	if(data < 0)
		return 0;
	BQ275x0_DBG("read fcc result = %d mAh\n",data);
	return data;
}

/*
 * Return the battery Time to Empty
 * Or < 0 if something fails
 */
int bq275x0_battery_tte(struct bq275x0_device_info *di)
{
	int data=0;

	if(!bq275x0_is_accessible())return 0;
	data = bq275x0_i2c_read_word(di,BQ275x0_REG_TTE);
	if(data<0)
	{
	    return 0;
	}
	BQ275x0_DBG("read tte result = %d minutes\n",data);
	return data;
}

/*
 * Return the battery Time to Full
 * Or < 0 if something fails
 */
int bq275x0_battery_ttf(struct bq275x0_device_info *di)
{
	int data=0;

	if(!bq275x0_is_accessible())
	{
    	return 0;
    }
	data = bq275x0_i2c_read_word(di,BQ275x0_REG_TTF);
	if(data < 0)
	{
	    return 0;
	}
	BQ275x0_DBG("read ttf result = %d minutes\n",data);
	return data;
}

/*
 * Return whether the battery charging is full
 * 
 */
int is_bq275x0_battery_full(struct bq275x0_device_info *di)
{
	int data = 0;
	if(!bq275x0_is_accessible())return 0;
	data = bq275x0_i2c_read_word(di,BQ275x0_REG_FLAGS);

	if(data < 0)
		return 0;
	BQ275x0_DBG("read flags result = 0x%x \n",data);
	return (data & BQ275x0_FLAG_FC);	
}


/*
 * Return whether the battery is discharging
 * 
 */
int is_bq275x0_battery_discharging(struct bq275x0_device_info *di)
{
	int data = 0;
	if(!bq275x0_is_accessible())return 0;
	data = bq275x0_i2c_read_word(di,BQ275x0_REG_FLAGS);
	if(data < 0)
		return 0;
	BQ275x0_DBG("read flags result = 0x%x \n",data);
	return (data & BQ275x0_FLAG_DISCHARGE);	
}

static int bq275x0_get_firmware_version_by_i2c(struct i2c_client *client)
{
    unsigned int data = 0;
    int id = 0;
    int ver = 0;

    mutex_lock(&bq275x0_battery_mutex);
    if (0 != i2c_smbus_write_word_data(client,BQ275x0_REG_CTRL,BQ275x0_REG_FIRMWARE_ID))
    {
        mutex_unlock(&bq275x0_battery_mutex);
        return -1;
    }
    mdelay(2);
    id = i2c_smbus_read_word_data(client,BQ275x0_REG_CTRL);
    mdelay(2);
    if (0 != i2c_smbus_write_word_data(client,BQ275x0_REG_DFCLS,BQ275x0_REG_FIRMWARE_VERSION))
    {
        mutex_unlock(&bq275x0_battery_mutex);
        return -1;
    }
    mdelay(2);
    ver = i2c_smbus_read_byte_data(client,BQ275x0_REG_FLASH);
    mdelay(2);
    mutex_unlock(&bq275x0_battery_mutex);

    if (id < 0 || ver < 0)
    {
        return -1;
    }

    data = (id << 16) | ver;
    return data;
} 


/*
 * bq275x0_get_firmware_version: Return the gas_gauge`s firmware version
 * Return:
 *      Fail to read: hardware or firmware is broken
 *      x(x)        : current_version/the version should be
 */
static ssize_t bq275x0_get_firmware_version(struct device_driver *driver, char *buf)
{
	int version = 0;
	int version_config = 0;
	int error = 0;

    if(!bq275x0_is_accessible())
    {
        error = -1;
		goto exit;
    }
	version = bq275x0_get_firmware_version_by_i2c(g_battery_measure_by_bq275x0_i2c_client);
	if(version < 0)
	{
	    error = -1;
	}
exit:		
	if(error)
	{
	    return sprintf(buf, "Fail to read");
	}
	else
	{
	    get_hw_config_int("gas_gauge/version", &version_config, NULL);
	    return sprintf(buf,"%x(%x)",version,version_config);
	}
}

/*
 * bq275x0_battery_check_firmware_version:check if the gauge firmware version is proper
 * Return:
 *		 0: firmware is OK
 *		-1: firmware version is not right
 *      -2: hardware or firmware is broken
 */
static ssize_t bq275x0_check_firmware_version(struct device_driver *driver, char *buf)
{
    int version = 0;
    int version_config = 0;
    int error = -1;

    if(!bq275x0_is_accessible())return 0;

    version = bq275x0_get_firmware_version_by_i2c(g_battery_measure_by_bq275x0_i2c_client);
    if(version < 0)
    {
        error = STATE_HARDWARE_ERR;
    }
    else if (get_hw_config_int("gas_gauge/version", &version_config, NULL))
    {
        if(version == version_config)
        {
            error = 0;
        }
    }
    
    return sprintf(buf,"%d",error);;
}

 static ssize_t bq275x0_check_qmax(struct device_driver *driver, char *buf)
{
	int control_status;
	int qmax,qmax1,data;
	int design_capacity = 0;
	int error = 0;
	mutex_lock(&bq275x0_battery_mutex);
	i2c_smbus_write_word_data(g_battery_measure_by_bq275x0_i2c_client,BQ275x0_REG_CTRL,BQ275x0_REG_CTRS);
	mdelay(2);
	control_status  = i2c_smbus_read_word_data(g_battery_measure_by_bq275x0_i2c_client,BQ275x0_REG_CTRL);
	mdelay(2);
	i2c_smbus_write_word_data(g_battery_measure_by_bq275x0_i2c_client,BQ275x0_REG_DFCLS,BQ275x0_REG_CLASS_ID);
	mdelay(2);
	qmax = i2c_smbus_read_byte_data(g_battery_measure_by_bq275x0_i2c_client,BQ275x0_REG_QMAX);
	printk("qmax = %d\n",qmax);
	mdelay(2);
	qmax1 = i2c_smbus_read_byte_data(g_battery_measure_by_bq275x0_i2c_client,BQ275x0_REG_QMAX1);
	printk("qmax1 = %d\n",qmax1);
	mutex_unlock(&bq275x0_battery_mutex);
	data = (qmax << 8) | qmax1;
	printk("qmaxData = %d\n",data);
	if (!get_hw_config_int("gas_gauge/capacity", &design_capacity, NULL))
		printk(KERN_ERR "[%s,%d] gas gauge capacity required in hw_configs.xml\n",__FUNCTION__,__LINE__);
	if (data >= (design_capacity * 4 / 5) && data <= (design_capacity * 6 / 5 ))
		error = 1;
	return sprintf(buf,"%d\n",error);
}

/*
 * Return the battery flags() params
 */
int bq275x0_battery_flags(struct bq275x0_device_info *di)
{
	int data = 0;
	data = bq275x0_i2c_read_word(di,BQ275x0_REG_FLAGS);
	
	if(data < 0)
		return 0;
	
	BQ275x0_DBG("read flags result = %d \n",data);
	return data;	
}
#ifndef BATTERY_IS_ALWAYS_PRESENT
/*===========================================================================
  Function:       is_bq275x0_battery_exist
  Description:    get the status of battery exist
  Calls:         
  Called By:      
  Input:          struct bq275x0_device_info *
  Output:         none
  Return:         1->battery present; 0->battery not present.
  Others:         none
===========================================================================*/

int is_bq275x0_battery_exist(struct bq275x0_device_info *di)
{
	int data = 0;
	if(!bq275x0_is_accessible())
	{
	    return gauge_context.battery_present;
	}
	data = bq275x0_i2c_read_word(di,BQ275x0_REG_FLAGS);
	BQ275x0_DBG("is_exist flags result = 0x%x \n",data);
	if(data < 0)
	{
		data = bq275x0_i2c_read_word(di,BQ275x0_REG_FLAGS);
	}
	if(data >= 0)
	{
		gauge_context.battery_present = !!(data & BQ275x0_FLAG_DET);
	}
	else
	{
		printk("i2c read BQ27510_REG_FLAGS error = %d \n",data);
	}

	return gauge_context.battery_present;
}
#endif

short bq275x0_battery_current_wrap(void)
{
       if(g_bq275x0_device_info_di)
	   	return bq275x0_battery_current(g_bq275x0_device_info_di);
	return -1;
}

int bq275x0_battery_voltage_wrap(void)
{
       if(g_bq275x0_device_info_di)
	   	return bq275x0_battery_voltage(g_bq275x0_device_info_di);
	return -1;
}

int bq275x0_battery_temperature_wrap(void)
{
       if(g_bq275x0_device_info_di)
	   	return bq275x0_battery_temperature(g_bq275x0_device_info_di);
	return -1;
}

int bq275x0_battery_capacity_wrap(void)
{
       if(g_bq275x0_device_info_di)
	   	return bq275x0_battery_capacity(g_bq275x0_device_info_di);
	return -1;
}

int bq275x0_battery_capacity_rm_wrap(void)
{
	if(g_bq275x0_device_info_di)
		return bq275x0_battery_capacity_rm(g_bq275x0_device_info_di);
	return -1;
}
int bq275x0_battery_tte_wrap(void)
{
       if(g_bq275x0_device_info_di)
	   	return bq275x0_battery_tte(g_bq275x0_device_info_di);
	return -1;
}

int bq275x0_battery_ttf_wrap(void)
{
       if(g_bq275x0_device_info_di)
	   	return bq275x0_battery_voltage(g_bq275x0_device_info_di);
	return -1;
}

int is_bq275x0_battery_full_wrap(void)
{
       if(g_bq275x0_device_info_di)
	   	return bq275x0_battery_voltage(g_bq275x0_device_info_di);
	return -1;
}

int is_bq275x0_battery_exist_wrap(void)
{
       if(g_bq275x0_device_info_di)
	   	return is_bq275x0_battery_exist(g_bq275x0_device_info_di);
	return -1;
}

int bq275x0_battery_temperature_original_wrap(void)
{
       if(g_bq275x0_device_info_di)
	   	return bq275x0_battery_temperature_original(g_bq275x0_device_info_di);
	return -1;
}

int is_bq275x0_battery_discharging_wrap(void)
{
      if(g_bq275x0_device_info_di)
	  	return is_bq275x0_battery_discharging(g_bq275x0_device_info_di);
      return -1;
}

int bq275x0_battery_flags_wrap(void)
{
       if(g_bq275x0_device_info_di)
	   	return bq275x0_battery_flags(g_bq275x0_device_info_di);
	return -1;
}


static struct msm_battery_8921gauge bq275x0_batt_gauge = {
	.get_battery_voltage		= bq275x0_battery_voltage_wrap,
	.get_battery_temperature	= bq275x0_battery_temperature_wrap,
	.get_battery_capacity		= bq275x0_battery_capacity_wrap,
	.get_battery_capacity_rm    = bq275x0_battery_capacity_rm_wrap,
	.get_battery_current	       = bq275x0_battery_current_wrap,
	.get_battery_tte		       = bq275x0_battery_tte_wrap,
	.get_battery_ttf		       = bq275x0_battery_ttf_wrap,
	.is_battery_full	                       = is_bq275x0_battery_full_wrap,
	.is_battery_exist                     = is_bq275x0_battery_exist_wrap,
	.is_battery_discharging          =is_bq275x0_battery_discharging_wrap,	
	.get_battery_fcc			= (int (*)(void*))bq27510_battery_fcc,
	.get_battery_health          = (int (*)(void*))bq275x0_battery_health,
	.get_battery_status          = (int (*)(void*))bq275x0_battery_flags,
};


/*
 * Return the battery Control
 * Or < 0 if something fails
 */
#define to_bq275x0_device_info(x) container_of((x), \
				struct bq275x0_device_info, bat);

static int bq275x0_battery_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
	struct bq275x0_device_info *di = to_bq275x0_device_info(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = bq275x0_battery_voltage(di);
		if (psp == POWER_SUPPLY_PROP_PRESENT)
			val->intval = val->intval <= 0 ? 0 : 1;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = bq275x0_battery_current(di);
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = bq275x0_battery_capacity(di);
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = bq275x0_battery_temperature(di);
		break;
	case POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW:
		val->intval = bq275x0_battery_tte(di);
		break;
	case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
		val->intval = bq275x0_battery_ttf(di);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static void bq275x0_hw_config(void)
{
  msm_battery_8921gauge_register(&bq275x0_batt_gauge);
	//enable_irq(g_bq275x0_device_info_di->client->irq);
}

static void bq275x0_powersupply_init(struct bq275x0_device_info *di)
{
	di->bat.type = POWER_SUPPLY_TYPE_BATTERY;
	di->bat.properties = bq275x0_battery_props;
	di->bat.num_properties = ARRAY_SIZE(bq275x0_battery_props);
	di->bat.get_property = bq275x0_battery_get_property;
	di->bat.external_power_changed = NULL;
}

int bq275x0_register_power_supply(struct power_supply ** ppBq275x0_power_supply)
{
	int rc = 0;
	struct bq275x0_device_info *di = NULL;
	
	if(g_battery_measure_by_bq275x0_i2c_client) {
		di = i2c_get_clientdata(g_battery_measure_by_bq275x0_i2c_client);
		rc = power_supply_register(&g_battery_measure_by_bq275x0_i2c_client->dev,&di->bat);
		if (rc) {
			BQ275x0_ERR("failed to register bq275x0 battery\n");
			return rc;
		}
		*ppBq275x0_power_supply = &di->bat;
	}
	else
		rc = -ENODEV;

	return rc;
}


static int bq275x0_atoi(const char *s)
{
	int k = 0;

	k = 0;
	while (*s != '\0' && *s >= '0' && *s <= '9') {
		k = 10 * k + (*s - '0');
		s++;
	}
	return k;
}

static unsigned long bq275x0_strtoul(const char *cp, unsigned int base)
{
	unsigned long result = 0,value;

	while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
	    ? toupper(*cp) : *cp)-'A'+10) < base) 
        {
		result = result*base + value;
		cp++;
	}

	return result;
}


static int bq275x0_firmware_program(struct i2c_client *client, const unsigned char *pgm_data, unsigned int filelen)
{
    unsigned int i = 0, j = 0, ulDelay = 0, ulReadNum = 0;
    unsigned int ulCounter = 0, ulLineLen = 0;
    unsigned char temp = 0; 
    unsigned char *p_cur;
    unsigned char pBuf[BSP_MAX_ASC_PER_LINE] = { 0 };
    unsigned char p_src[BSP_I2C_MAX_TRANSFER_LEN] = { 0 };
    unsigned char p_dst[BSP_I2C_MAX_TRANSFER_LEN] = { 0 };
    unsigned char ucTmpBuf[16] = { 0 };

bq275x0_firmware_program_begin:
    if(ulCounter > 10)
    {
        return -1;
    }
    
    p_cur = (unsigned char *)pgm_data;       

    while(1)
    {
        while (*p_cur == '\r' || *p_cur == '\n')
        {
            p_cur++;
        }

        if((p_cur - pgm_data) >= filelen)
        {
            printk("Download success\n");
            break;
        }
        i = 0;
        ulLineLen = 0;

        memset(p_src, 0x00, sizeof(p_src));
        memset(p_dst, 0x00, sizeof(p_dst));
        memset(pBuf, 0x00, sizeof(pBuf));

        /*获取一行数据，去除空格*/
        while(i < BSP_MAX_ASC_PER_LINE)
        {
            temp = *p_cur++;      
            i++;
            if(('\r' == temp) || ('\n' == temp))
            {
                break;  
            }
            if(' ' != temp)
            {
                pBuf[ulLineLen++] = temp;
            }
        }

        
        p_src[0] = pBuf[0];
        p_src[1] = pBuf[1];

        if(('W' == p_src[0]) || ('C' == p_src[0]))
        {
            for(i=2,j=0; i<ulLineLen; i+=2,j++)
            {
                memset(ucTmpBuf, 0x00, sizeof(ucTmpBuf));
                memcpy(ucTmpBuf, pBuf+i, 2);
                p_src[2+j] = bq275x0_strtoul(ucTmpBuf, 16);
            }

            temp = (ulLineLen -2)/2;
            ulLineLen = temp + 2;
        }
        else if('X' == p_src[0])
        {
            memset(ucTmpBuf, 0x00, sizeof(ucTmpBuf));
            memcpy(ucTmpBuf, pBuf+2, ulLineLen-2);
            ulDelay = bq275x0_atoi(ucTmpBuf);
        }
        else if('R' == p_src[0])
        {
            memset(ucTmpBuf, 0x00, sizeof(ucTmpBuf));
            memcpy(ucTmpBuf, pBuf+2, 2);
            p_src[2] = bq275x0_strtoul(ucTmpBuf, 16);
            memset(ucTmpBuf, 0x00, sizeof(ucTmpBuf));
            memcpy(ucTmpBuf, pBuf+4, 2);
            p_src[3] = bq275x0_strtoul(ucTmpBuf, 16);
            memset(ucTmpBuf, 0x00, sizeof(ucTmpBuf));
            memcpy(ucTmpBuf, pBuf+6, ulLineLen-6);
            ulReadNum = bq275x0_atoi(ucTmpBuf);
        }

        if(':' == p_src[1])
        {
            switch(p_src[0])
            {
                case 'W' :

                    #if 0
                    printk("W: ");
                    for(i=0; i<ulLineLen-4; i++)
                    {
                        printk("%x ", p_src[4+i]);
                    }
                    printk(KERN_ERR "\n");
                    #endif                    

                    if(bq275x0_i2c_bytes_write(client, p_src[3], &p_src[4], ulLineLen-4) < 0)
                    {
                         printk(KERN_ERR "[%s,%d] bq275x0_i2c_bytes_write failed len=%d\n",__FUNCTION__,__LINE__,ulLineLen-4);                        
                    }

                    break;
                
                case 'R' :
                    if(bq275x0_i2c_bytes_read(client, p_src[3], p_dst, ulReadNum) < 0)
                    {
                        printk(KERN_ERR "[%s,%d] bq275x0_i2c_bytes_read failed\n",__FUNCTION__,__LINE__);
                    }
                    break;
                    
                case 'C' :
                    if(bq275x0_i2c_bytes_read_and_compare(client, p_src[3], p_dst, &p_src[4], ulLineLen-4))
                    {
                        ulCounter++;
                        printk(KERN_ERR "[%s,%d] bq275x0_i2c_bytes_read_and_compare failed\n",__FUNCTION__,__LINE__);
                        goto bq275x0_firmware_program_begin;
                    }
                    break;
                    
                case 'X' :                    
                    mdelay(ulDelay);
                    break;
                  
                default:
                    return 0;
            }
        }
      
    }

    return 0;
    
}

static int bq275x0_firmware_download(struct i2c_client *client, const unsigned char *pgm_data, unsigned int len)
{
    int iRet;
    unsigned short i2c_addr_backup;
	gauge_context.state = BQ275x0_UPDATE_FIRMWARE;
    /*Enter Rom Mode */
    iRet = bq275x0_i2c_word_write(client, BSP_ENTER_ROM_MODE_CMD, BSP_ENTER_ROM_MODE_DATA);
    if(0 != iRet)
    {
        printk(KERN_ERR "[%s,%d] bq275x0_i2c_word_write failed\n",__FUNCTION__,__LINE__);
    }
    mdelay(10);

    /*change i2c addr*/
    i2c_addr_backup = client->addr;
    client->addr = BSP_ROM_MODE_I2C_ADDR;

    /*program bqfs*/
    iRet = bq275x0_firmware_program(client, pgm_data, len);
    if(0 != iRet)
    {
        printk(KERN_ERR "[%s,%d] bq275x0_firmware_program failed\n",__FUNCTION__,__LINE__);
    }

    /*restore i2c addr*/
    client->addr = i2c_addr_backup;
    LOCAL_ASSERT(client->addr == BSP_NORMAL_MODE_I2C_ADDR);
   gauge_context.locked_timeout_jiffies =  jiffies + msecs_to_jiffies(5000);
   gauge_context.state = BQ275x0_LOCK_MODE;
    if(iRet < 0){
		if(bq275x0_batt_gauge.gauge_valid){
				bq275x0_batt_gauge.gauge_valid = 0;
				disable_irq(g_bq275x0_device_info_di->client->irq);
			}
    	}
    else{
		if(!bq275x0_batt_gauge.gauge_valid){	
			bq275x0_batt_gauge.gauge_valid = 1;
			enable_irq_wake(g_bq275x0_device_info_di->client->irq);
			}
		/* reset gas gauge after firmware download*/
		if (0 != bq275x0_i2c_word_write(client,BQ275x0_REG_CTRL,BQ275x0_DATA_RESET))
	        printk(KERN_ERR "[%s,%d] write reset failed\n",__FUNCTION__,__LINE__);
		else
		printk(KERN_ERR "[%s,%d] bq27510 download reset\n",__FUNCTION__,__LINE__);
    	}
   
    return iRet;
    
}


static bool get_gas_gauge_firmware_name(char *config_name)
{
    bool nRet = false;

    if(get_hw_config("gas_gauge/firmware_name", config_name, GAS_GAUGE_FIRMWARE_NAME, NULL))
    {
        nRet = true;
    }

    return nRet;
}

static int get_gas_version_id(char * id, char * name)
{
    char *end   = NULL;
    char *start = NULL;
    
    
    start = strrchr(name, '_');
    end = strchr(name, '.');
    
    if (start == NULL || end == NULL || start > end)
    {
        return 1;
    }
        
    start++;
    while(start < end) 
    {
        *id++ = *start++;
    }
    *id = '\0';
    return 0;
}
static int bq275x0_update_firmware(struct i2c_client *client, const char *pFilePath,char *firmware_name) 
{
    char *buf;
    struct file *filp;
    struct inode *inode = NULL;
    mm_segment_t oldfs;
    unsigned int length;
    int ret = 0;
    struct firmware_header_entry entry;
    loff_t pos;
    ssize_t vfs_read_retval = 0;
    char config_name[GAS_GAUGE_FIRMWARE_NAME] = "unknown";
    char id[FIRMWARE_VERSION_ID_LEN];
    char current_id[FIRMWARE_VERSION_ID_LEN];
    int temp;

    if(firmware_name!= NULL)
    {
        if(strlen(firmware_name)<GAS_GAUGE_FIRMWARE_NAME)
        {
            memcpy(config_name, firmware_name, strlen(firmware_name)+1);
            printk("copy end config_name is %s\n",config_name);
        }
        else
            return -1;
    }
    /* open file */
    else if (!get_gas_gauge_firmware_name(config_name)) //as BQ27510_GUANYU_1670
    {
        printk(KERN_ERR "[%s,%d] gas gauge firmware name required in hw_configs.xml\n",__FUNCTION__,
               __LINE__);
        return -1;
    }
    oldfs = get_fs();
    set_fs(KERNEL_DS);
    filp = filp_open(pFilePath, O_RDONLY, S_IRUSR);
    if (IS_ERR(filp)) 
    {
        printk(KERN_ERR "[%s,%d] filp_open failed\n",__FUNCTION__,__LINE__);
        set_fs(oldfs);
        return -1;
    }

    if (!filp->f_op) 
    {
        printk(KERN_ERR "[%s,%d] File Operation Method Error\n",__FUNCTION__,__LINE__);        
        filp_close(filp, NULL);
        set_fs(oldfs);
        return -1;
    }

    //read magicnum if is merged bqfs or just single bqfs file
    vfs_read_retval = filp->f_op->read(filp, (char __user *)(&temp), sizeof(int), &filp->f_pos);
    filp->f_pos = 0;

    if(vfs_read_retval != sizeof(int))
    {
        printk(KERN_ERR "[%s,%d] read file error\n",__FUNCTION__,__LINE__);
        filp_close(filp, NULL);
        set_fs(oldfs);
        return -1;
    }

    if(MAGIC_NUMBER != temp)  //is single bqfs file
    {
        inode = filp->f_path.dentry->d_inode;
        if (!inode) 
        {
            printk(KERN_ERR "[%s,%d] Get inode from filp failed\n",__FUNCTION__,__LINE__);          
            filp_close(filp, NULL);
            set_fs(oldfs);
            return -1;
        }
        length = i_size_read(inode->i_mapping->host);
        entry.offset = 0;
    }
    else  //is merged bqfs file
    {
        while (1) 
        {
            vfs_read_retval = filp->f_op->read(filp, (char __user *)(&entry), 
                                    sizeof(struct firmware_header_entry), &filp->f_pos);
            printk("%s--- %s---0---\n", config_name, entry.file_name);
            
            if(vfs_read_retval != sizeof(entry)) 
            {
                printk(KERN_ERR "[%s,%d] Get magic_number error\n",__FUNCTION__,__LINE__);
                filp_close(filp, NULL);
                set_fs(oldfs);
                return -1;
            } 
            if (entry.magic_number != MAGIC_NUMBER) 
            {
                printk(KERN_ERR "[%s,%d] Get magic_number error\n",__FUNCTION__,__LINE__);
                filp_close(filp, NULL);
                set_fs(oldfs);
                return -1;
            }
            if (strncmp(entry.file_name, config_name, strlen(config_name)) == 0) 
            {
                length = entry.length;
                if (!get_gas_version_id(id, entry.file_name))
                printk(KERN_ERR "gas gauge firmware download version ID = [%s] \n",id);

                sprintf(current_id, "%x", bq275x0_get_firmware_version_by_i2c(client));
                printk(KERN_ERR "gas gauge curent firmware version ID = [%s] \n",current_id);
                if (strncasecmp(id, current_id, FIRMWARE_VERSION_ID_LEN) == 0)
                {
                    printk(KERN_ERR "no need to update gas gauge firmware\n");
                    return 0;
                }
                break;
            }
            if (entry.file_name[0] == '\0')   //if not find referred bqfs file in merged file
            {
                printk(KERN_ERR "[%s,%d] no gas gauge firmware to download \n",__FUNCTION__,__LINE__);
                filp_close(filp, NULL);
                set_fs(oldfs);
                return -1;
            }
        }
    }

    /* file's size */
    printk("bq275x0 firmware image size is %d \n",length);
    if (!( length > 0 && length < BSP_FIRMWARE_FILE_SIZE))
    {
        printk(KERN_ERR "[%s,%d] Get file size error\n",__FUNCTION__,__LINE__);
        filp_close(filp, NULL);
        set_fs(oldfs);
        return -1;
    }

    /* allocation buff size */
    buf = vmalloc(length+(length%2));       /* buf size if even */
    if (!buf) 
    {
        printk(KERN_ERR "[%s,%d] Alloctation memory failed\n",__FUNCTION__,__LINE__);
        filp_close(filp, NULL);
        set_fs(oldfs);
        return -1;
    }

    /* read data */
    pos = (loff_t)entry.offset;
    if (filp->f_op->read(filp, buf, length, &pos) != length) 
    {
        printk(KERN_ERR "[%s,%d] File read error\n",__FUNCTION__,__LINE__);
        filp_close(filp, NULL);
        filp_close(filp, NULL);
        set_fs(oldfs);
        vfree(buf);
        return -1;
    }

    ret = bq275x0_firmware_download(client, (const char*)buf, length);

    filp_close(filp, NULL);
    set_fs(oldfs);
    vfree(buf);
    
    return ret;
    
}


static ssize_t bq275x0_attr_store(struct device_driver *driver,const char *buf, size_t count)
{
    int iRet = 0;
    unsigned char path_image[255];
    
    if(NULL == buf || count >255 || count == 0 || strnchr(buf, count, 0x20))
        return -1;

    memcpy (path_image, buf,  count);
    /* replace '\n' with  '\0'  */ 
    if((path_image[count-1]) == '\n')
        path_image[count-1] = '\0'; 
    else
        path_image[count] = '\0';    	

    /*enter firmware bqfs download*/
    gBq275x0DownloadFirmwareFlag = BSP_FIRMWARE_DOWNLOAD_MODE;
    iRet = bq275x0_update_firmware(g_battery_measure_by_bq275x0_i2c_client, path_image,NULL);
    gBq275x0DownloadFirmwareFlag = BSP_NORMAL_MODE;	

    i2c_smbus_write_word_data(g_battery_measure_by_bq275x0_i2c_client,0x00,0x0021);

    return iRet;
}


static ssize_t bq275x0_attr_show(struct device_driver *driver, char *buf)
{
    int iRet = 0;
    int data = 0;
    int data1 = 0 ;
    int data2 = 0;
   
    if(NULL == buf)
    {
        return -1;
    }
    
    mutex_lock(&bq275x0_battery_mutex);
	i2c_smbus_write_word_data(g_battery_measure_by_bq275x0_i2c_client,0x00,0x0008);
    mdelay(2);
	iRet = i2c_smbus_read_word_data(g_battery_measure_by_bq275x0_i2c_client,0x00);
	data = i2c_smbus_read_word_data(g_battery_measure_by_bq275x0_i2c_client,BQ275x0_REG_FLAGS);
    
    //read terminate voltage
    i2c_smbus_write_byte_data(g_battery_measure_by_bq275x0_i2c_client,0x3e,0x50);
    mdelay(2);
	i2c_smbus_write_byte_data(g_battery_measure_by_bq275x0_i2c_client,0x3f,0x01);
	mdelay(2);
	data1 = i2c_smbus_read_byte_data(g_battery_measure_by_bq275x0_i2c_client,0x4c);
	data2 = i2c_smbus_read_byte_data(g_battery_measure_by_bq275x0_i2c_client,0x4d);
    mutex_unlock(&bq275x0_battery_mutex);
	if(iRet < 0)
	{
        return sprintf(buf, "%s\n", "Coulometer Damaged or Firmware Error");
	}
    else
    {
        return sprintf(buf, "id=0x%x,flag=0x%x,tervol=0x%x,%x\n", iRet,data,data1,data2);
    }

}

static ssize_t bq275x0_firmware_switch(struct device_driver *driver,const char *buf, size_t count)
{
    char firmware_name[GAS_GAUGE_FIRMWARE_NAME];
    int iRet = 0;
    char *path_image = FIRMWARE_STORE_PATH;

    memset(firmware_name, 0, GAS_GAUGE_FIRMWARE_NAME);

    if(NULL == buf || 0 == count)
    {
        return -1;
    }
    memcpy (firmware_name, buf, count);
    firmware_name[count] = '\0';

    gBq275x0DownloadFirmwareFlag = BSP_FIRMWARE_DOWNLOAD_MODE;
    iRet = bq275x0_update_firmware(g_battery_measure_by_bq275x0_i2c_client, path_image,firmware_name);
    gBq275x0DownloadFirmwareFlag = BSP_NORMAL_MODE;

    i2c_smbus_write_word_data(g_battery_measure_by_bq275x0_i2c_client,0x00,0x0021);
    return count;
}


static DRIVER_ATTR(state, S_IRUGO|(S_IWUSR|S_IWGRP), bq275x0_attr_show, bq275x0_attr_store);
static DRIVER_ATTR(firmware_version, S_IRUGO, bq275x0_get_firmware_version, NULL);
static DRIVER_ATTR(firmware_check, S_IRUGO, bq275x0_check_firmware_version, NULL);
static DRIVER_ATTR(qmax, S_IRUGO, bq275x0_check_qmax,NULL);
static DRIVER_ATTR(firmware_switch, S_IRUGO|(S_IWUSR|S_IWGRP), NULL, bq275x0_firmware_switch);

void bq275x0_sleep_config(bool no_sleep)
{
	u16 read_1;
	u8 old_checksum;
	mdelay(2);
	i2c_smbus_write_byte_data(g_battery_measure_by_bq275x0_i2c_client,BQ275x0_REG_DFCLS,BQ275x0_REG_BLKDAT);
	mdelay(2);
	i2c_smbus_write_byte_data(g_battery_measure_by_bq275x0_i2c_client,BQ275x0_REG_DFBLK,BQ275x0_REG_BLKDAT1);
	mdelay(2);
	read_1= i2c_smbus_read_byte_data(g_battery_measure_by_bq275x0_i2c_client,BQ275x0_REG_BLKDAT2);

	if(no_sleep)
	{
		old_checksum= i2c_smbus_read_byte_data(g_battery_measure_by_bq275x0_i2c_client,BQ275x0_REG_CHKSUM);
		mdelay(2);
		i2c_smbus_write_byte_data(g_battery_measure_by_bq275x0_i2c_client,BQ275x0_REG_BLKDAT2,read_1&BQ275x0_NOSLEEP_SETTING);//write configure
		mdelay(2);
		i2c_smbus_write_byte_data(g_battery_measure_by_bq275x0_i2c_client,BQ275x0_REG_CHKSUM,old_checksum+( read_1-(read_1&BQ275x0_NOSLEEP_SETTING)));
		mdelay(2);
		i2c_smbus_write_word_data(g_battery_measure_by_bq275x0_i2c_client,BQ275x0_REG_RESET,BQ275x0_DATA_RESET);	//reset
	}
	else
	{
		old_checksum=  i2c_smbus_read_byte_data(g_battery_measure_by_bq275x0_i2c_client,BQ275x0_REG_CHKSUM);
		mdelay(2);
		i2c_smbus_write_byte_data(g_battery_measure_by_bq275x0_i2c_client,BQ275x0_REG_BLKDAT2,read_1|BQ275x0_SLEEP_SETTING);	
		mdelay(2);
		i2c_smbus_write_byte_data(g_battery_measure_by_bq275x0_i2c_client,BQ275x0_REG_CHKSUM,old_checksum+(read_1-(read_1|BQ275x0_SLEEP_SETTING)));
		mdelay(2);
		i2c_smbus_write_word_data(g_battery_measure_by_bq275x0_i2c_client,BQ275x0_REG_RESET,BQ275x0_DATA_RESET);	//reset
	}
}

static ssize_t bq275x0_show_Tgaugelog(struct device_driver *driver, char *buf)
{
    int temp, voltage, cur, capacity, rm , fcc, control_status,ttf,si;
    u16 flag;
    u8 qmax, qmax1;
    if(NULL == buf)
    {
        return -1;
    }
    if(!bq275x0_is_accessible())return sprintf(buf,"bq275x0 is busy because of updating(%d)",gauge_context.state);
    temp =  bq275x0_battery_temperature_wrap();
    mdelay(2);
    voltage = bq275x0_battery_voltage_wrap();
    mdelay(2);
    cur = bq275x0_i2c_read_word(g_bq275x0_device_info_di,BQ275x0_REG_AI);
    mdelay(2);
    capacity = bq275x0_i2c_read_word(g_bq275x0_device_info_di,BQ275x0_REG_SOC);
    mdelay(2);
    flag = bq275x0_i2c_read_word(g_bq275x0_device_info_di,BQ275x0_REG_FLAGS);
    mdelay(2);
    rm =  bq275x0_i2c_read_word(g_bq275x0_device_info_di,BQ275x0_REG_RM);
    mdelay(2);
    fcc =  bq275x0_i2c_read_word(g_bq275x0_device_info_di,BQ275x0_REG_FCC);
    mdelay(2);
    ttf = bq275x0_i2c_read_word(g_bq275x0_device_info_di,BQ275x0_REG_TTF);
    mdelay(2);
    si = bq275x0_i2c_read_word(g_bq275x0_device_info_di,BQ275x0_REG_SI);
    mutex_lock(&bq275x0_battery_mutex);
    i2c_smbus_write_word_data(g_battery_measure_by_bq275x0_i2c_client,BQ275x0_REG_CTRL,BQ275x0_REG_CTRS);
    mdelay(2);
    control_status  = i2c_smbus_read_word_data(g_battery_measure_by_bq275x0_i2c_client,BQ275x0_REG_CTRL);
    mdelay(2);
    i2c_smbus_write_word_data(g_battery_measure_by_bq275x0_i2c_client,BQ275x0_REG_DFCLS,BQ275x0_REG_CLASS_ID);
    mdelay(2);
    qmax = i2c_smbus_read_byte_data(g_battery_measure_by_bq275x0_i2c_client,BQ275x0_REG_QMAX);
    mdelay(2);
    qmax1 = i2c_smbus_read_byte_data(g_battery_measure_by_bq275x0_i2c_client,BQ275x0_REG_QMAX1);
    mdelay(2);
    mutex_unlock(&bq275x0_battery_mutex);

    if(qmax < 0)
    {
        return sprintf(buf, "%s", "Coulometer Damaged or Firmware Error \n");
    }
    else
    {
      sprintf(buf, "%-9d  %-9d  %-4d  %-6d  %-6d  %-5d  %-6d  %-6d  0x%-5.4x  0x%-5.2x  0x%-5.2x  0x%-5.2x  ",                                
                    voltage,  (signed short)cur, capacity, temp/10, fcc, rm, (signed short)ttf, (signed short)si, flag, control_status, qmax, qmax1 );
        return strlen(buf);
    }
}
static DRIVER_ATTR(Tgaugelog, S_IRUGO, bq275x0_show_Tgaugelog,NULL);

static int bq275x0_atoh(const char *s)
{
	int k = 0;

	k = 0;
	while (*s != '\0' && ((*s >= '0' && *s <= '9' )||(*s >= 'A' && *s <= 'F' ))) {
		k<<=4;
		if(*s>='A'&& *s<='F')k |= 10+*s -'A';
		else k |=*s - '0';
		s++;
	}
	return k;
}


static ssize_t bq275x0_debug_store(struct device_driver *driver,const char *buf, size_t count)
{
    g_battery_measure_by_bq275x0_i2c_client->addr =  bq275x0_atoh(buf);
    return count;
}


static ssize_t bq275x0_debug_show(struct device_driver *driver, char *buf)
{
    return sprintf(buf,"bq275x0 i2c addr=%x I2C error count=%d lock count=%d",g_battery_measure_by_bq275x0_i2c_client->addr,gauge_context.i2c_error_count,
                    gauge_context.lock_count);
}
static DRIVER_ATTR(debug, S_IRUGO|S_IWUSR,bq275x0_debug_show,bq275x0_debug_store);

void bq275x0_register_notify_callback(bq275x0_notify_handler PFN,void* param)
{
    if(!PFN || !param)
    {
        return;
    }
    gauge_context.pfn_notify_call = PFN;
    gauge_context.callback_param_p = param;
}
//EXPORT_SYMBOL(bq275x0_register_notify_callback);


void bq275x0_low_capcity_notify(unsigned long event)
{
    if(gauge_context.pfn_notify_call)
    {
        gauge_context.pfn_notify_call(gauge_context.callback_param_p, event);
    }
}


static void interrupt_notifier_work(struct work_struct *work)
{
    struct bq275x0_device_info *di = container_of(work,
		struct bq275x0_device_info, notifier_work.work);
	long int events;
	
	int low_bat_flag = bq275x0_battery_flags(di);

	printk("bq275x0 interrupt_notifier_work \n");
	if(!is_bq275x0_battery_exist(di) || !(low_bat_flag & BQ275x0_FLAG_SOC1))
	{
	    goto ret;
    }

    if(!(low_bat_flag & BQ275x0_FLAG_SOCF))
    {
        events = BATTERY_LOW_WARNING;
        printk("BATTERY_LOW_WARNING");
	}
	else if(low_bat_flag & BQ275x0_FLAG_SOCF)
	{
	    events = BATTERY_LOW_SHUTDOWN;
	    printk("BATTERY_LOW_SHUTDOWN");
	}
	else
	{
	    goto ret;
	}

	bq275x0_low_capcity_notify(events);
ret:
	wake_unlock(&di->low_power_lock);
}
static irqreturn_t soc_irqhandler(int irq, void *dev_id)
{
    struct bq275x0_device_info *di = dev_id;
    
    printk("SOC BAT_LOW Arrive:\n");

    if(time_is_before_jiffies((unsigned long)di->timeout_jiffies))
	{
	    printk("handle the low_battery interrupt\n");
	    wake_lock(&di->low_power_lock);
        schedule_delayed_work(&di->notifier_work, msecs_to_jiffies(SET_FLAG_TIME));
		di->timeout_jiffies = jiffies + msecs_to_jiffies(DISABLE_ACCESS_TIME);
    }

	return IRQ_HANDLED;
}
static int bq275x0_battery_remove(struct i2c_client *client)
{
	struct bq275x0_device_info *di = i2c_get_clientdata(client);

	free_irq(di->client->irq,di);
	wake_lock_destroy(&di->low_power_lock);
	power_supply_unregister(&di->bat);
	msm_battery_8921gauge_unregister(&bq275x0_batt_gauge);
	cancel_delayed_work_sync(&di->hw_config);
	kfree(di->bat.name);

	mutex_lock(&bq275x0_battery_mutex);
	idr_remove(&bq275x0_battery_id, di->id);
	mutex_unlock(&bq275x0_battery_mutex);

	kfree(di);
	return 0;
}


static int bq275x0_battery_probe(struct i2c_client *client,
				 const struct i2c_device_id *id)
{
	int num;
	char *name;
	int retval = 0;
	int i;
	int version = 0;
	struct bq275x0_device_info *di;

	printk("bq275x0_battery_probe begin\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		BQ275x0_ERR("[%s,%d]: need I2C_FUNC_I2C\n",__FUNCTION__,__LINE__);
		printk("bq275x0_battery_probe fail\n");
		return -ENODEV;
	}
    for(i=0; i<3; i++)
    {
        i2c_smbus_write_word_data(client,0x00,0x0008);
        mdelay(2);
        retval = i2c_smbus_read_word_data(client,0x00);
        if(retval >=0)break;
        mdelay(2);
    }
	
	if(retval<0)
	{
        printk(KERN_ERR "[%s,%d] Coulometer Damaged or Firmware Error\n",__FUNCTION__,__LINE__);
        bq275x0_batt_gauge.gauge_valid = 0;
	}
    else
    {
        bq275x0_batt_gauge.gauge_valid = 1;
        printk(KERN_ERR "Normal Mode and read Firmware version=%04x\n", retval);
    }
    
    retval = driver_create_file(&(bq275x0_battery_driver.driver), &driver_attr_state);
    if (0 != retval)
    {
        printk("failed to create sysfs entry(state): %d\n", retval);
        return -1;
    }

    retval = driver_create_file(&(bq275x0_battery_driver.driver), &driver_attr_Tgaugelog);
    if (0 != retval)
    {
        printk("failed to create sysfs entry(Tgaugelog): %d\n", retval);
        return -1;
    }

    retval = driver_create_file(&(bq275x0_battery_driver.driver), &driver_attr_firmware_version);
    if (0 != retval) 
    {
    	printk("failed to create sysfs entry(firmware_version): %d\n", retval);
    	return -1;
    }
    retval = driver_create_file(&(bq275x0_battery_driver.driver), &driver_attr_firmware_check);
    if (0 != retval) 
    {
    	printk("failed to create sysfs entry(firmware_check): %d\n", retval);
    	return -1;
    }

    retval = driver_create_file(&(bq275x0_battery_driver.driver), &driver_attr_firmware_switch);
    if (0 != retval)
    {
        printk("failed to create sysfs entry(firmware_switch): %d\n", retval);
        return -1;
    }

    retval = driver_create_file(&(bq275x0_battery_driver.driver), &driver_attr_qmax);
    if (0 != retval)
    {
	printk("failed to create sysfs entry(qmax): %d\n", retval);
	return -1;
     }


	retval = driver_create_file(&(bq275x0_battery_driver.driver), &driver_attr_debug);
	if(0 != retval)printk("failed to create sysfs entry(debug): %d\n", retval);

	retval = idr_pre_get(&bq275x0_battery_id, GFP_KERNEL);
	if (retval == 0) {
		retval = -ENOMEM;
		goto batt_failed_0;
	}
	mutex_lock(&bq275x0_battery_mutex);
	retval = idr_get_new(&bq275x0_battery_id, client, &num);
	mutex_unlock(&bq275x0_battery_mutex);
	if (retval < 0) {
		goto batt_failed_0;
	}

	name = kasprintf(GFP_KERNEL, "bq275x0-%d", num);
	if (!name) {
		dev_err(&client->dev, "failed to allocate device name\n");
		retval = -ENOMEM;
		goto batt_failed_1;
	}

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di) {
		dev_err(&client->dev, "failed to allocate device info data\n");
		retval = -ENOMEM;
		goto batt_failed_2;
	}
	di->id = num;

	i2c_set_clientdata(client, di);
	di->dev = &client->dev;
	di->bat.name = name;
	di->client = client;
	di->timeout_jiffies = jiffies;

	wake_lock_init(&di->low_power_lock, WAKE_LOCK_SUSPEND, "Bq275x0_PowerLow");
	
	bq275x0_batt_gauge.gauge_priv = (void*)di;
	retval = get_gpio_num_by_name("BAT_LOW_INT");
	if(retval < 0){
		printk(KERN_ERR "No BAT_LOW_INT is specified!\n");
		goto gpio_config_fail;
		}
	di->gpio_config = retval;
	di->client->irq = MSM_GPIO_TO_INT(di->gpio_config);

	bq275x0_powersupply_init(di);

	g_battery_measure_by_bq275x0_i2c_client = client;
	g_bq275x0_device_info_di = di;

	dev_info(&client->dev, "bq275x0 support ver. %s enabled\n", DRIVER_VERSION);

    bq275x0_hw_config();


	retval = 0;
    retval = gpio_request(g_bq275x0_device_info_di->gpio_config, "gpio_bq275x0_intr");
	if (retval) {
		printk(KERN_ERR"bq275x0_battery_probe: gpio_request failed for intr %d\n", g_bq275x0_device_info_di->gpio_config);
		goto gpio_request_fail;
	}
	retval = gpio_direction_input(g_bq275x0_device_info_di->gpio_config);
	if (retval) {
		printk(KERN_ERR"bq275x0_battery_probe: gpio_direction_input failed for intr %d\n", g_bq275x0_device_info_di->gpio_config);
	}
	retval = request_irq(g_bq275x0_device_info_di->client->irq, soc_irqhandler,
				IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING,
				"bq275x0_IRQ", di);
	if (retval < 0) {
		printk(KERN_ERR "bq275x0_battery_probe: request irq failed\n");
		goto irq_request_fail;
	}
	 else {
	 	if(bq275x0_batt_gauge.gauge_valid)enable_irq_wake(g_bq275x0_device_info_di->client->irq);
		else disable_irq(g_bq275x0_device_info_di->client->irq);
	}

    #ifdef CONFIG_BQ27520_TEST_ENABLE
    INIT_DELAYED_WORK(&di->test_printk, bq275x0_test_printk);
    schedule_delayed_work(&di->test_printk, DELAY_TEST_PRINTK);
    #endif

    INIT_DELAYED_WORK_DEFERRABLE(&di->notifier_work,
				interrupt_notifier_work);
	bq275x0_sleep_config(false);

	version = bq275x0_get_firmware_version_by_i2c(client);

    printk("bq275x0_battery_probe end, coluversion=%x\n",version);
	return 0;

batt_failed_2:
	kfree(name);
batt_failed_1:
	mutex_lock(&bq275x0_battery_mutex);
	idr_remove(&bq275x0_battery_id, num);
	mutex_unlock(&bq275x0_battery_mutex);
batt_failed_0:
	return retval;
gpio_request_fail:
        gpio_free(di->gpio_config);
 irq_request_fail:
 	free_irq(g_bq275x0_device_info_di->client->irq,di);
gpio_config_fail: 
	return 0;
}


#ifdef CONFIG_BQ27520_TEST_ENABLE
static void bq275x0_test_printk(struct work_struct *work)
{
    struct bq275x0_device_info *di=NULL;
    int a=0,b=0,c=0,d=0,e=0,f=0,g=0,h=0,i=0,j=0;
	di = container_of(work, struct bq275x0_device_info, test_printk.work);

	if(NULL == di)
	{
	    printk(KERN_ERR "=====Fail to get struct bq275x0_device_info of bq275x0 \n");
	    return;
	}
    
    a=bq275x0_battery_current(di);
    b=bq275x0_battery_voltage(di);
    c=bq275x0_battery_capacity(di);
    d=bq275x0_battery_temperature(di);
    e=bq275x0_battery_tte(di);
    f=bq275x0_battery_ttf(di);
    g=is_bq275x0_battery_full(di);
    h=is_bq275x0_battery_exist(di);
    i=bq275x0_battery_temperature_original(di);
    j=bq275x0_battery_flags(di);

    printk(KERN_ERR "=====read current result = %d mA\n",a);
    printk(KERN_ERR "=====read voltage result = %d volts\n",b);
    printk(KERN_ERR "=====read capacity result = %d Hundred Percents\n",c);
    printk(KERN_ERR "=====read temperature result = %d Celsius\n",i);
    printk(KERN_ERR "=====The battery Time to Empty = %d minutes\n",e);
    printk(KERN_ERR "=====The battery Time to Full = %d minutes\n",f);
    printk(KERN_ERR "=====The battery charging is Full = %d\n",g);
    printk(KERN_ERR "=====The status of battery Exist = %d\n",h);
    printk(KERN_ERR "=====Read Flags() result = %d\n",j);

    schedule_delayed_work(&di->test_printk, DELAY_TEST_PRINTK);
}
#endif


/*Module stuff*/
static const struct i2c_device_id bq275x0_id[] = {
	{"bq275x0-battery",0},
	{},
};

static struct i2c_driver bq275x0_battery_driver = {
	.driver = {
		.name = "bq275x0-battery",
	},
	.probe = bq275x0_battery_probe,
	.remove = bq275x0_battery_remove,
	.id_table = bq275x0_id,
};

static int __init bq275x0_battery_init(void)
{
	int ret;

	ret = i2c_add_driver(&bq275x0_battery_driver);
	if (ret)
		BQ275x0_ERR("Unable to register BQ275x0 driver\n");
	return ret;
}

module_init(bq275x0_battery_init);

static void __exit bq275x0_battery_exit(void)
{
	i2c_del_driver(&bq275x0_battery_driver);
}
module_exit(bq275x0_battery_exit);

MODULE_AUTHOR("Rodolfo Giometti <giometti@linux.it>");
MODULE_DESCRIPTION("BQ275x0 battery monitor driver");
MODULE_LICENSE("GPL");
