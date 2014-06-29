/*
 *  apds990x.c - Linux kernel modules for ambient light + proximity sensor
 *
 *  Copyright (C) 2010 Lee Kai Koon <kai-koon.lee@avagotech.com>
 *  Copyright (C) 2010 Avago Technologies
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/input.h>
#include "apds990x.h"
#include "../sensors.h"
#include <hsad/config_interface.h>
/* Change History
 *
 * 1.0.1	Functions apds990x_show_rev(), apds990x_show_id() and apds990x_show_status()
 *			have missing CMD_BYTE in the i2c_smbus_read_byte_data(). APDS-990x needs
 *			CMD_BYTE for i2c write/read byte transaction.
 *
 *
 * 1.0.2	Include PS switching threshold level when interrupt occurred
 *
 *
 * 1.0.3	Implemented ISR and delay_work, correct PS threshold storing
 *
 * 1.0.4	Added Input Report Event
 */

/*
 * Management functions
 */
#define  MAX_ADC_PROX_VALUE  1023
 #define  PS_FIRST_VALUE      550
#include <linux/fs.h>
#include <linux/syscalls.h>
#define  I2C_TRY_TIME    10
/*for print luxvaule wenjuan 00182148 20120214 end*/

 #define  DELAY_FOR_DATA_RADY            300
 
static struct apds990x_platform_data *platform_data;

static int lux_old = 300;
static int min_proximity_value;
static int apds_990x_pwindows_value = 450;
static int apds_990x_pwave_value = 250;
static int ps_h, ps_l = 0;
static int flag_work_enable;
static int ps_is_docom = 0;
static int IC_old = 50;
static int pcount = 0;
static int apds990x_set_command(struct i2c_client *client, int command)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int ret;
	int clearInt;

	if (command == 0)
		clearInt = CMD_CLR_PS_INT;
	else if (command == 1)
		clearInt = CMD_CLR_ALS_INT;
	else
		clearInt = CMD_CLR_PS_ALS_INT;

	mutex_lock(&data->update_lock);
	ret = i2c_smbus_write_byte(client, clearInt);
	mutex_unlock(&data->update_lock);

	return ret;
}

static int apds990x_set_enable(struct i2c_client *client, int enable)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&data->update_lock);
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS990x_ENABLE_REG, enable);
	mutex_unlock(&data->update_lock);

	data->enable = enable;

	return ret;
}

static int apds990x_set_atime(struct i2c_client *client, int atime)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&data->update_lock);
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS990x_ATIME_REG, atime);
	mutex_unlock(&data->update_lock);

	data->atime = atime;

	return ret;
}

static int apds990x_set_ptime(struct i2c_client *client, int ptime)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&data->update_lock);
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS990x_PTIME_REG, ptime);
	mutex_unlock(&data->update_lock);

	data->ptime = ptime;

	return ret;
}

static int apds990x_set_wtime(struct i2c_client *client, int wtime)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&data->update_lock);
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS990x_WTIME_REG, wtime);
	mutex_unlock(&data->update_lock);

	data->wtime = wtime;

	return ret;
}

static int apds990x_set_ailt(struct i2c_client *client, int threshold)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&data->update_lock);
	ret = i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_AILTL_REG, threshold);
	mutex_unlock(&data->update_lock);

	data->ailt = threshold;

	return ret;
}

static int apds990x_set_aiht(struct i2c_client *client, int threshold)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&data->update_lock);
	ret = i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_AIHTL_REG, threshold);
	mutex_unlock(&data->update_lock);

	data->aiht = threshold;

	return ret;
}

static int apds990x_set_pilt(struct i2c_client *client, int threshold)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&data->update_lock);
	ret = i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_PILTL_REG, threshold);
	mutex_unlock(&data->update_lock);

	data->pilt = threshold;

	return ret;
}

static int apds990x_set_piht(struct i2c_client *client, int threshold)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&data->update_lock);
	ret = i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_PIHTL_REG, threshold);
	mutex_unlock(&data->update_lock);

	data->piht = threshold;

	return ret;
}

static int apds990x_set_pers(struct i2c_client *client, int pers)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&data->update_lock);
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS990x_PERS_REG, pers);
	mutex_unlock(&data->update_lock);

	data->pers = pers;

	return ret;
}

static int apds990x_set_config(struct i2c_client *client, int config)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&data->update_lock);
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS990x_CONFIG_REG, config);
	mutex_unlock(&data->update_lock);

	data->config = config;

	return ret;
}

static int apds990x_set_ppcount(struct i2c_client *client, int ppcount)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&data->update_lock);
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS990x_PPCOUNT_REG, ppcount);
	mutex_unlock(&data->update_lock);

	data->ppcount = ppcount;

	return ret;
}

static int apds990x_set_control(struct i2c_client *client, int control)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&data->update_lock);
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS990x_CONTROL_REG, control);
	mutex_unlock(&data->update_lock);

	data->control = control;

	/* obtain ALS gain value */
	if ((data->control&0x03) == 0x00) /* 1X Gain */
		data->als_gain = 1;
	else if ((data->control&0x03) == 0x01) /* 8X Gain */
		data->als_gain = 8;
	else if ((data->control&0x03) == 0x02) /* 16X Gain */
		data->als_gain = 16;
	else  /* 120X Gain */
		data->als_gain = 120;

	return ret;
}

static int LuxCalculation(struct i2c_client *client, int cdata, int irdata)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int luxValue=0;

	int IAC1=0;
	int IAC2=0;
	int IAC=0;
	int GA;
	int COE_B;
	int COE_C;
	int COE_D;
	int DF = 52;

    if( ps_is_docom != 0 )
    {
        GA = 4000;
        COE_B = 2117;
        COE_C = 76;
        COE_D = 78;
    }
    else
    {
        GA = 6300;
        COE_B = 2117;
        COE_C = 76;
        COE_D = 78;
    }
	IAC1 = (cdata - (COE_B*irdata)/1000);	// re-adjust COE_B to avoid 2 decimal point
	IAC2 = ((COE_C*cdata)/1000 - (COE_D*irdata)/1000); // re-adjust COE_C and COE_D to void 2 decimal point

	if (IAC1 > IAC2)
		IAC = IAC1;
	else IAC = IAC2;

	if(IAC < 0) 
        IAC = IC_old;
	else 
        IC_old = IAC;

	luxValue = ((IAC*GA*DF)/1000)/(((272*(256-data->atime))/100) *data->als_gain);
	if(data->enable_ps_sensor)
	{
		if(cdata > 16383)
			luxValue = 10000;
	}
	else if(cdata > 65534)
		luxValue = 10000;
	return luxValue;
}

static void apds990x_change_ps_threshold(struct i2c_client *client)
{
	struct apds990x_data *data = i2c_get_clientdata(client);

	int pthreshold_h = 0, pthreshold_l = 0;

	data->ps_data =	i2c_smbus_read_word_data(client, CMD_WORD|APDS990x_PDATAL_REG);
        if ((data->ps_data + apds_990x_pwave_value) < min_proximity_value && (data->ps_data > 0))
        {
            min_proximity_value = data->ps_data + apds_990x_pwave_value;

            i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_PILTL_REG, min_proximity_value);
            i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_PIHTL_REG, (min_proximity_value + apds_990x_pwindows_value));
        }

        pthreshold_h = i2c_smbus_read_word_data(client,CMD_WORD| APDS990x_PIHTL_REG);
        pthreshold_l = i2c_smbus_read_word_data(client, CMD_WORD|APDS990x_PILTL_REG);
	/*get value of proximity*/
        // proximity_data_value = data->ps_data;

        /* if more than the value of  proximity high threshold we set*/

        if (data->ps_data >= pthreshold_h)
        {
            data->ps_detection = 1;
            i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_PILTL_REG, min_proximity_value);
			i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_PIHTL_REG, 1023);
            input_report_abs(data->input_dev_ps, ABS_DISTANCE, 0);/* FAR-to-NEAR detection */
	     input_sync(data->input_dev_ps);
        }
        /* if less than the value of  proximity low threshold we set*/
        /* the condition of pdata==pthreshold_l is valid */
        else if (data->ps_data <=  pthreshold_l)
        {
            data->ps_detection = 0;
            //i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_PILTL_REG, 0);
            i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_PIHTL_REG, min_proximity_value + apds_990x_pwindows_value);
            input_report_abs(data->input_dev_ps, ABS_DISTANCE, 1);/* NEAR-to-FAR detection */
	    input_sync(data->input_dev_ps);
        }
        /*on 27a platform ,bug info is a lot*/
        else
        {
            printk("Wrong status!\n");
            i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_PILTL_REG, min_proximity_value);
        }

		pthreshold_h = i2c_smbus_read_word_data(client,CMD_WORD| APDS990x_PIHTL_REG);
        pthreshold_l = i2c_smbus_read_word_data(client, CMD_WORD|APDS990x_PILTL_REG);
        ps_h = pthreshold_h;
        ps_l = pthreshold_l;


}

static void apds990x_change_als_threshold(struct i2c_client *client)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int cdata, irdata;
	int luxValue=0;

	cdata = i2c_smbus_read_word_data(client, CMD_WORD|APDS990x_CDATAL_REG);
	irdata = i2c_smbus_read_word_data(client, CMD_WORD|APDS990x_IRDATAL_REG);

	luxValue = LuxCalculation(client, cdata, irdata);

	luxValue = luxValue>0 ? luxValue : 0;
	luxValue = luxValue<10000 ? luxValue : 10000;

	 // check PS under sunlight
	if ( (data->ps_detection == 1) && (irdata >= 5000))	// PS was previously in far-to-near condition
	{
		// need to inform input event as there will be no interrupt from the PS
		//input_report_abs(data->input_dev_ps, ABS_DISTANCE, 0);/* NEAR-to-FAR detection */
        input_report_abs(data->input_dev_ps, ABS_DISTANCE, 1);/* NEAR-to-FAR detection */
		input_sync(data->input_dev_ps);

		//i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_PILTL_REG, 0);
		i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_PIHTL_REG, min_proximity_value + apds_990x_pwindows_value);

		data->pilt = 0;
		data->piht = data->ps_threshold;

		data->ps_detection = 0;	/* near-to-far detected */

		dev_dbg(&client->dev,"apds_990x_proximity_handler = FAR\n");
	}


	input_report_abs(data->input_dev_als, ABS_MISC, luxValue); // report the lux level
	input_sync(data->input_dev_als);

	data->als_data = cdata;

	data->als_threshold_l = (data->als_data * (100-APDS990x_ALS_THRESHOLD_HSYTERESIS) ) /100;
	data->als_threshold_h = (data->als_data * (100+APDS990x_ALS_THRESHOLD_HSYTERESIS) ) /100;

	if (data->als_threshold_h >= 65535) data->als_threshold_h = 65535;

	i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_AILTL_REG, data->als_threshold_l);

	i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_AIHTL_REG, data->als_threshold_h);
}

static void apds990x_reschedule_work(struct apds990x_data *data,
					  unsigned long delay)
{
	//unsigned long flags;
    //printk("----------------apds990x_reschedule_work enter------------\n");
	//spin_lock_irqsave(&data->update_lock, flags);

	/*
	 * If work is already scheduled then subsequent schedules will not
	 * change the scheduled time that's why we have to cancel it first.
	 */
	__cancel_delayed_work(&data->dwork);
	schedule_delayed_work(&data->dwork, delay);

	//spin_unlock_irqrestore(&data->update_lock, flags);
}

/* ALS polling routine */
static void apds990x_als_polling_work_handler(struct work_struct *work)
{
	struct apds990x_data *data = container_of(work, struct apds990x_data, als_dwork.work);
	struct i2c_client *client=data->client;
	int cdata, irdata, pdata;
	int luxValue=0;

    if(!flag_work_enable) return;
	cdata = i2c_smbus_read_word_data(client, CMD_WORD|APDS990x_CDATAL_REG);
	irdata = i2c_smbus_read_word_data(client, CMD_WORD|APDS990x_IRDATAL_REG);
	pdata = i2c_smbus_read_word_data(client, CMD_WORD|APDS990x_PDATAL_REG);
       if(irdata < 0 ||cdata < 0 || pdata < 0)
       {
           dev_err(&client->dev, "apds990x: als read i2c read fail!cdata(%d),irdata(%d),pdata(%d)\n", cdata, irdata, pdata);
       }
	luxValue = LuxCalculation(client, cdata, irdata);
       if(luxValue < 0)
       {
           input_report_abs(data->input_dev_als, ABS_MISC, lux_old); // report the lux level
           input_sync(data->input_dev_als);
           dev_err(&client->dev,"apds990x:luxvaule = %d!\n", luxValue);
           return;
       }
	luxValue = luxValue<10000 ? luxValue : 10000;
    lux_old = luxValue;
	//printk("%s: lux = %d cdata = %d  irdata = %d pdata = %d \n", __func__, luxValue, cdata, irdata, pdata);

	// check PS under sunlight
	if ( (data->ps_detection == 1) && (irdata >= 5000))	// PS was previously in far-to-near condition
	{
		// need to inform input event as there will be no interrupt from the PS
		//input_report_abs(data->input_dev_ps, ABS_DISTANCE, 0);/* NEAR-to-FAR detection */
		input_report_abs(data->input_dev_ps, ABS_DISTANCE, 1);/* NEAR-to-FAR detection */
		input_sync(data->input_dev_ps);

		//i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_PILTL_REG, 0);
		i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_PIHTL_REG, min_proximity_value + apds_990x_pwindows_value);

		data->pilt = 0;
		data->piht = data->ps_threshold;

		data->ps_detection = 0;	/* near-to-far detected */

		dev_dbg(&client->dev,"apds_990x_proximity_handler = FAR\n");
	}

    input_report_abs(data->input_dev_als, ABS_MISC, luxValue);
    input_sync(data->input_dev_als);


	schedule_delayed_work(&data->als_dwork, msecs_to_jiffies(data->als_poll_delay));	// restart timer
}

/* PS interrupt routine */
static void apds990x_work_handler(struct work_struct *work)
{
	struct apds990x_data *data = container_of(work, struct apds990x_data, dwork.work);
	struct i2c_client *client=data->client;
	int	status;
	int cdata, irdata;
	int try = 0;
    // printk("--------------------apds990x_work_handler enter---------------\n");
//	mutex_lock(&data->suspend_lock); 

	status = i2c_smbus_read_byte_data(client, CMD_BYTE|APDS990x_STATUS_REG);
	while((try < I2C_TRY_TIME)&&(status < 0))
	{
         msleep(5);
	     status = i2c_smbus_read_byte_data(client, CMD_BYTE|APDS990x_STATUS_REG);
	     try++;
	}
	if(try >= I2C_TRY_TIME)
	{
           printk("[%s]i2c timeout \n",__func__);
           goto exit;
	}
	irdata = i2c_smbus_read_word_data(client, CMD_WORD|APDS990x_IRDATAL_REG);
	i2c_smbus_write_byte_data(client, CMD_BYTE|APDS990x_ENABLE_REG, 1);	/* disable 990x's ADC first */

	dev_dbg(&client->dev,"status = %x\n", status);

	if ((status & data->enable & 0x30) == 0x30) {
		/* both PS and ALS are interrupted */
		apds990x_change_als_threshold(client);

		cdata = i2c_smbus_read_word_data(client, CMD_WORD|APDS990x_CDATAL_REG);
		if (irdata < 5000)
			apds990x_change_ps_threshold(client);
		else {
			if (data->ps_detection == 1) {
				apds990x_change_ps_threshold(client);
			}
			else {
				dev_dbg(&client->dev,"Triggered by background ambient noise\n");
			}
		}

		apds990x_set_command(client, 2);	/* 2 = CMD_CLR_PS_ALS_INT */
	}
	else if ((status & data->enable & 0x20) == 0x20) {
		/* only PS is interrupted */

		/* check if this is triggered by background ambient noise */
		cdata = i2c_smbus_read_word_data(client, CMD_WORD|APDS990x_CDATAL_REG);
		if (irdata < 5000)
			apds990x_change_ps_threshold(client);
		else {
			if (data->ps_detection == 1) {
				apds990x_change_ps_threshold(client);
			}
			else {
				dev_dbg(&client->dev,"Triggered by background ambient noise\n");
			}
		}

		apds990x_set_command(client, 0);	/* 0 = CMD_CLR_PS_INT */
	}
	else if ((status & data->enable & 0x10) == 0x10) {
		/* only ALS is interrupted */
		apds990x_change_als_threshold(client);

		apds990x_set_command(client, 1);	/* 1 = CMD_CLR_ALS_INT */
	}

	i2c_smbus_write_byte_data(client, CMD_BYTE|APDS990x_ENABLE_REG, data->enable);
//	mutex_unlock(&data->suspend_lock);
exit:
    enable_irq(client->irq);
}

/* assume this is ISR */
static irqreturn_t apds990x_interrupt(int vec, void *info)
{
	struct i2c_client *client=(struct i2c_client *)info;
	struct apds990x_data *data = i2c_get_clientdata(client);

	//printk("==> apds990x_interrupt\n");
    disable_irq_nosync(client->irq);
	apds990x_reschedule_work(data, 0);

	return IRQ_HANDLED;
}

/*
 * SysFS support
 */

static ssize_t apds990x_show_enable_ps_sensor(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds990x_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%d\n", data->enable_ps_sensor);
}

static ssize_t apds990x_store_enable_ps_sensor(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds990x_data *data = i2c_get_clientdata(client);
	unsigned long val = simple_strtoul(buf, NULL, 10);
 	unsigned long flags;

	dev_dbg(dev,"%s: enable ps senosr ( %ld)\n", __func__, val);

	if ((val != 0) && (val != 1)) {
		dev_dbg(dev,"%s:store unvalid value=%ld\n", __func__, val);
		return count;
	}

	if(val == 1) {
		//turn on p sensor
		//first status is FAR
		input_report_abs(data->input_dev_ps, ABS_DISTANCE, 1);
		input_sync(data->input_dev_ps);
		if (data->enable_ps_sensor==0) {

			data->enable_ps_sensor= 1;
            spin_lock_irqsave(&data->update_lock.wait_lock, flags);
            __cancel_delayed_work(&data->als_dwork);
            spin_unlock_irqrestore(&data->update_lock.wait_lock, flags);
			apds990x_set_enable(client,0); /* Power Off */
			apds990x_set_atime(client, 0xf0); /* 27.2ms */
			apds990x_set_ptime(client, 0xff); /* 2.72ms */

			//apds990x_set_ppcount(client, 8); /* 8-pulse */
			apds990x_set_ppcount(client, pcount); /* 5-pulse */
            //modify FAE
            //apds990x_set_control(client, 0x20); /* 100mA, IR-diode, 1X PGAIN, 1X AGAIN */
		    apds990x_set_control(client, 0x20); /* 100mA, IR-diode, 1X PGAIN, 1X AGAIN */
			apds990x_set_pilt(client, 0);		// init threshold for proximity
			apds990x_set_piht(client, PS_FIRST_VALUE);

			apds990x_set_ailt( client, 0);
			apds990x_set_aiht( client, 0xffff);

			apds990x_set_pers(client, 0x33); /* 3 persistence */
			apds990x_set_enable(client, 0x27);	 /* only enable PS interrupt */

			spin_lock_irqsave(&data->update_lock.wait_lock, flags);
			schedule_delayed_work(&data->als_dwork, msecs_to_jiffies(DELAY_FOR_DATA_RADY));
			spin_unlock_irqrestore(&data->update_lock.wait_lock, flags);
		}
	}
	else {
		//turn off p sensor - kk 25 Apr 2011 we can't turn off the entire sensor, the light sensor may be needed by HAL
		data->enable_ps_sensor = 0;
		if (data->enable_als_sensor) {
            spin_lock_irqsave(&data->update_lock.wait_lock, flags);
            __cancel_delayed_work(&data->als_dwork);
			spin_unlock_irqrestore(&data->update_lock.wait_lock, flags);
			// reconfigute light sensor setting
			apds990x_set_enable(client,0); /* Power Off */

			apds990x_set_atime(client, 0xc0);  /* previous als poll delay */

			apds990x_set_ailt( client, 0);
			apds990x_set_aiht( client, 0xffff);

			//modify FAE
			//apds990x_set_control(client, 0x20); /* 100mA, IR-diode, 1X PGAIN, 1X AGAIN */
            apds990x_set_control(client, 0x20); /* 100mA, IR-diode, 1X PGAIN, 16X AGAIN */
            apds990x_set_pers(client, 0x33); /* 3 persistence */

			apds990x_set_enable(client, 0x3);	 /* only enable light sensor */

			spin_lock_irqsave(&data->update_lock.wait_lock, flags);
			schedule_delayed_work(&data->als_dwork, msecs_to_jiffies(DELAY_FOR_DATA_RADY));
			spin_unlock_irqrestore(&data->update_lock.wait_lock, flags);

		}
		else {
			apds990x_set_enable(client, 0);

			spin_lock_irqsave(&data->update_lock.wait_lock, flags);

			/*
			 * If work is already scheduled then subsequent schedules will not
			 * change the scheduled time that's why we have to cancel it first.
			 */
			__cancel_delayed_work(&data->als_dwork);

			spin_unlock_irqrestore(&data->update_lock.wait_lock, flags);
		}
	}


	return count;
}

static DEVICE_ATTR(enable_ps_sensor, S_IWUSR | S_IRUGO,
				   apds990x_show_enable_ps_sensor, apds990x_store_enable_ps_sensor);

static ssize_t apds990x_show_enable_als_sensor(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds990x_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%d\n", data->enable_als_sensor);
}

static ssize_t apds990x_store_enable_als_sensor(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds990x_data *data = i2c_get_clientdata(client);
	unsigned long val = simple_strtoul(buf, NULL, 10);
 	unsigned long flags;

	dev_dbg(dev,"%s: enable als sensor ( %ld)\n", __func__, val);

	if ((val != 0) && (val != 1))
	{
		dev_dbg(dev,"%s: enable als sensor=%ld\n", __func__, val);
		return count;
	}

	if(val == 1) {
		//turn on light  sensor
		if (data->enable_als_sensor==0) {

			data->enable_als_sensor = 1;
                     flag_work_enable = 1;
			apds990x_set_enable(client,0); /* Power Off */

			//apds990x_set_atime(client, data->als_atime);  /* 100.64ms */

			apds990x_set_ailt( client, 0);
			apds990x_set_aiht( client, 0xffff);
		//modify 1020
			apds990x_set_pers(client, 0x33); /* 3 persistence */

			if (data->enable_ps_sensor) {
				apds990x_set_atime(client, 0xf0);
				apds990x_set_ptime(client, 0xff); /* 2.72ms */
                apds990x_set_control(client, 0x20);
			//	apds990x_set_ppcount(client, 8); /* 8-pulse */
                apds990x_set_ppcount(client, pcount); /* 5-pulse */
                apds990x_set_enable(client, 0x27);	 /* if prox sensor was activated previously */
			}
			else {
				apds990x_set_atime(client, 0xc0);
				apds990x_set_enable(client, 0x3);	 /* only enable light sensor */
                apds990x_set_control(client, 0x20); /* 100mA, IR-diode, 1X PGAIN, 16X AGAIN */
			}
			spin_lock_irqsave(&data->update_lock.wait_lock, flags);

			/*
			 * If work is already scheduled then subsequent schedules will not
			 * change the scheduled time that's why we have to cancel it first.
			 */
			__cancel_delayed_work(&data->als_dwork);
			schedule_delayed_work(&data->als_dwork, msecs_to_jiffies(DELAY_FOR_DATA_RADY));
			spin_unlock_irqrestore(&data->update_lock.wait_lock, flags);
		}
	}
	else {
		//turn off light sensor
		// what if the p sensor is active?
		data->enable_als_sensor = 0;
		if (data->enable_ps_sensor) {
			apds990x_set_enable(client,0); /* Power Off */
			apds990x_set_atime(client, 0xf0);  /* 27.2ms */
			apds990x_set_ptime(client, 0xff); /* 2.72ms */
			//apds990x_set_ppcount(client, 8); /* 8-pulse */
			apds990x_set_ppcount(client, pcount); /* 5-pulse */
            //modify
			//apds990x_set_control(client, 0x20); /* 100mA, IR-diode, 1X PGAIN, 1X AGAIN */
            apds990x_set_control(client, 0x20); /* 100mA, IR-diode, 1X PGAIN, 16X AGAIN */

			apds990x_set_pilt(client, ps_l);
			apds990x_set_piht(client, ps_h);

			apds990x_set_ailt( client, 0);
			apds990x_set_aiht( client, 0xffff);

			apds990x_set_pers(client, 0x33); /* 3 persistence */
			apds990x_set_enable(client, 0x27);	 /* only enable prox sensor with interrupt */
		}
		else {
			flag_work_enable = 0;
			apds990x_set_enable(client, 0);
		}


		spin_lock_irqsave(&data->update_lock.wait_lock, flags);

		/*
		 * If work is already scheduled then subsequent schedules will not
		 * change the scheduled time that's why we have to cancel it first.
		 */
		__cancel_delayed_work(&data->als_dwork);

		spin_unlock_irqrestore(&data->update_lock.wait_lock, flags);
	}

	return count;
}
static DEVICE_ATTR(enable_als_sensor, S_IWUSR|S_IWGRP | S_IRUGO,
				   apds990x_show_enable_als_sensor, apds990x_store_enable_als_sensor);

static ssize_t apds990x_show_read_hl_threshold(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	//struct apds990x_data *data = i2c_get_clientdata(client);
	int pthreshold_h=0, pthreshold_l;

	pthreshold_h = i2c_smbus_read_word_data(client,CMD_WORD| APDS990x_PIHTL_REG);
        pthreshold_l = i2c_smbus_read_word_data(client, CMD_WORD|APDS990x_PILTL_REG);

	return sprintf(buf, "%d:%d\n", pthreshold_h,pthreshold_l);	// return in micro-second
}

static DEVICE_ATTR(read_hl_threshold, S_IWUSR|S_IWGRP | S_IRUGO,
				   apds990x_show_read_hl_threshold, NULL);
static ssize_t apds990x_show_als_poll_delay(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds990x_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%d\n", data->als_poll_delay*1000);	// return in micro-second
}

static ssize_t apds990x_store_als_poll_delay(struct device *dev,
					struct device_attribute *attr, const char *buf, size_t count)
{

    struct i2c_client *client = to_i2c_client(dev);
	struct apds990x_data *data = i2c_get_clientdata(client);
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int poll_delay=0;
    unsigned long flags;
	if (val<5000)
    {
		//val = 200000;	// minimum 200ms
		val = 5000;	// minimum 5ms
    }
	data->als_poll_delay = val/1000;	// convert us => ms
	poll_delay = 256 - (val/2720);	// the minimum is 2.72ms = 2720 us, maximum is 696.32ms
	if (poll_delay >= 256)
		data->als_atime = 255;
	else if (poll_delay < 0)
		data->als_atime = 0;
	else
		data->als_atime = poll_delay;
	/* we need this polling timer routine for sunlight canellation */
	spin_lock_irqsave(&data->update_lock.wait_lock, flags);

	/*
	 * If work is already scheduled then subsequent schedules will not
	 * change the scheduled time that's why we have to cancel it first.
	 */
	__cancel_delayed_work(&data->als_dwork);
	if(data->enable_als_sensor)
		schedule_delayed_work(&data->als_dwork, msecs_to_jiffies(data->als_poll_delay));	// 100ms
	spin_unlock_irqrestore(&data->update_lock.wait_lock, flags);

	return count;

}

static DEVICE_ATTR(als_poll_delay, S_IWUSR | S_IRUGO,
				   apds990x_show_als_poll_delay, apds990x_store_als_poll_delay);

static struct attribute *apds990x_attributes[] = {
	&dev_attr_enable_ps_sensor.attr,
	&dev_attr_enable_als_sensor.attr,
	&dev_attr_als_poll_delay.attr,
	&dev_attr_read_hl_threshold.attr,
	NULL
};


static const struct attribute_group apds990x_attr_group = {
	.attrs = apds990x_attributes,
};

/*
 * Initialization function
 */

static int apds990x_init_client(struct i2c_client *client)
{
	int err;
	int id;

	err = apds990x_set_enable(client, 0);

	if (err < 0)
		return err;

	id = i2c_smbus_read_byte_data(client, CMD_BYTE|APDS990x_ID_REG);
	if (id == 0x20) {
		dev_dbg(&client->dev,"APDS-9901\n");
	}
	else if (id == 0x29) {
		dev_dbg(&client->dev,"APDS-990x\n");
	}
	else {
		dev_dbg(&client->dev,"Neither APDS-9901 nor APDS-9901\n");
		return -EIO;
	}
	apds990x_set_atime(client, 0xf0);	// 100.64ms ALS integration time
	apds990x_set_ptime(client, 0xFF);	// 2.72ms Prox integration time
	apds990x_set_wtime(client, 0xFF);	// 2.72ms Wait time

    apds990x_set_ppcount(client, pcount);	// 5-Pulse for proximity

	//apds990x_set_ppcount(client, 0x08);	// 8-Pulse for proximity
	apds990x_set_config(client, 0);		// no long wait
	//apds990x_set_control(client, 0x20);	// 100mA, IR-diode, 1X PGAIN, 1X AGAIN
	apds990x_set_control(client, 0x20);	// 100mA, IR-diode, 1X PGAIN, 16X AGAIN

    ps_l = 0;
	ps_h = PS_FIRST_VALUE;
	apds990x_set_pilt(client, ps_l);		// init threshold for proximity
	apds990x_set_piht(client, ps_h);

	apds990x_set_ailt(client, 0);		// init threshold for als
	apds990x_set_aiht(client, 0xFFFF);

	apds990x_set_pers(client, 0x22);	// 2 consecutive Interrupt persistence

	// sensor is in disabled mode but all the configurations are preset

	return 0;
}

/*
 * I2C init/probing/exit functions
 */

static struct i2c_driver apds990x_driver;
static int __devinit apds990x_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
	char ls_ptype[LSENSOR_PNAME_LEN] = {0};
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct apds990x_data *data;
	int err = 0;
    flag_work_enable = 1;

	get_lightsensor_type(ls_ptype,LSENSOR_PNAME_LEN);
    if(!strncmp(ls_ptype,"product_docomo",LSENSOR_PNAME_LEN))
	{
	    dev_info(&client->dev,"product_docomo.\n");
        apds_990x_pwindows_value = 450;
        apds_990x_pwave_value = 250;
		pcount = 5;
		ps_is_docom = 1;
	}
	else if(!strncmp(ls_ptype,"product_Cricket",LSENSOR_PNAME_LEN))
	{
		dev_info(&client->dev,"product_Cricket.\n");
		apds_990x_pwindows_value = 450;
		apds_990x_pwave_value = 200;
		pcount = 12;
		ps_is_docom = 0;
	}
	else
	{
	    dev_info(&client->dev,"product_SoftBank.\n");
        apds_990x_pwindows_value = 300;
        apds_990x_pwave_value = 200;
		pcount = 5;
		ps_is_docom = 0;
	}
    min_proximity_value = MAX_ADC_PROX_VALUE - apds_990x_pwindows_value;
    //printk("--------------------apds990x_probe enter---------------\n");
        platform_data = client->dev.platform_data;
        if(platform_data->power_on)
	{
	    err = platform_data->power_on(&client->dev);
	    if(err < 0){
	    	dev_err(&client->dev,"apds990x_probe: get power fail!\n");
		goto exit;
	    }
	}
	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE)) {
		err = -EIO;
		goto exit_put_power;
	}

	data = kzalloc(sizeof(struct apds990x_data), GFP_KERNEL);
	if (!data) {
		err = -ENOMEM;
		goto exit_put_power;
	}
	data->client = client;
	i2c_set_clientdata(client, data);

	data->enable = 0;	/* default mode is standard */
	data->ps_threshold = 0;
	data->ps_hysteresis_threshold = 0;
	data->ps_detection = 0;	/* default to no detection */
	data->enable_als_sensor = 0;	// default to 0
	data->enable_ps_sensor = 0;	// default to 0
	//data->als_poll_delay = 100;	// default to 100ms
	data->als_poll_delay = 500;	// default to 500ms
	data->als_atime	= 0xdb;			// work in conjuction with als_poll_delay

	dev_dbg(&client->dev,"enable = %x\n", data->enable);

	mutex_init(&data->update_lock);
//    mutex_init(&data->suspend_lock);
    if(platform_data->gpio_config_interrupt)
    {
    	client->irq = platform_data->gpio_config_interrupt(1);
        if(client->irq<0)
        {
            dev_err(&client->dev, "irq gpio reguest failed\n");
            goto exit_kfree;
        }
    }
    else
    {
        dev_err(&client->dev, "no irq config func\n");
        goto exit_kfree;
    }

    enable_irq_wake(client->irq);
	if (request_irq(client->irq, apds990x_interrupt,IRQF_DISABLED|IRQ_TYPE_EDGE_FALLING,
		APDS990x_DRV_NAME, (void *)client)) {

		dev_dbg(&client->dev,"%s Could not allocate APDS990x_INT !\n", __func__);

		goto exit_free_gpio_irq;
	}

	INIT_DELAYED_WORK(&data->dwork, apds990x_work_handler);
	INIT_DELAYED_WORK(&data->als_dwork, apds990x_als_polling_work_handler);

	dev_dbg(&client->dev,"%s interrupt is hooked\n", __func__);

	/* Initialize the APDS990x chip */
	err = apds990x_init_client(client);
	if (err)
		goto exit_free_irq;

	/* Register to Input Device */
	data->input_dev_als = input_allocate_device();
	if (!data->input_dev_als) {
		err = -ENOMEM;
		dev_err(&client->dev,"Failed to allocate input device als\n");
		goto exit_free_irq;
	}

	data->input_dev_ps = input_allocate_device();
	if (!data->input_dev_ps) {
		err = -ENOMEM;
		dev_dbg(&client->dev,"Failed to allocate input device ps\n");
		goto exit_free_dev_als;
	}

    data->input_dev_als->name = "light sensor";
    data->input_dev_als->id.bustype = BUS_I2C;
	data->input_dev_als->dev.parent = &data->client->dev;
    input_set_drvdata(data->input_dev_als, data);
    data->input_dev_ps->name = "proximity sensor";
    data->input_dev_ps->id.bustype = BUS_I2C;
	data->input_dev_ps->dev.parent = &data->client->dev;
    input_set_drvdata(data->input_dev_ps, data);
	set_bit(EV_ABS, data->input_dev_als->evbit);
	set_bit(EV_ABS, data->input_dev_ps->evbit);

	input_set_abs_params(data->input_dev_als, ABS_MISC, 0, 10000, 0, 0);
	input_set_abs_params(data->input_dev_ps, ABS_DISTANCE, 0, 1, 0, 0);

	err = input_register_device(data->input_dev_als);
	if (err) {
		err = -ENOMEM;
		dev_dbg(&client->dev,"Unable to register input device als: %s\n",
		       data->input_dev_als->name);
		goto exit_free_dev_ps;
	}

	err = input_register_device(data->input_dev_ps);
	if (err) {
		err = -ENOMEM;
		dev_dbg(&client->dev,"Unable to register input device ps: %s\n",
		       data->input_dev_ps->name);
		goto exit_unregister_dev_als;
	}

	/* Register sysfs hooks */
	err = sysfs_create_group(&client->dev.kobj, &apds990x_attr_group);
	if (err)
		goto exit_unregister_dev_ps;

	err = set_sensor_input(ALS, data->input_dev_als->dev.kobj.name);
	if (err) {
		sysfs_remove_group(&client->dev.kobj, &apds990x_attr_group);
		dev_err(&client->dev, "%s, set_sensor_input als error: %s\n",
		       __func__, data->input_dev_als->name);
		goto exit_unregister_dev_ps;
	}

	err = set_sensor_input(PS, data->input_dev_ps->dev.kobj.name);
	if (err) {
		sysfs_remove_group(&client->dev.kobj, &apds990x_attr_group);
		dev_err(&client->dev, "%s,set_sensor_input ps error: %s\n",
		       __func__, data->input_dev_ps->name);
		goto exit_unregister_dev_ps;
	}

	printk("%s support ver. %s enabled\n", __func__, DRIVER_VERSION);

	return 0;

exit_unregister_dev_ps:
	input_unregister_device(data->input_dev_ps);
exit_unregister_dev_als:
	input_unregister_device(data->input_dev_als);
exit_free_dev_ps:
	input_free_device(data->input_dev_ps);
exit_free_dev_als:
	input_free_device(data->input_dev_als);
exit_free_irq:
	//free_irq(APDS990x_INT, client);
	free_irq(client->irq,client);
exit_free_gpio_irq:
    if(platform_data->gpio_config_interrupt)
    {
        platform_data->gpio_config_interrupt(0);
    }
exit_kfree:
	kfree(data);
exit_put_power:
    if(platform_data->power_off)
	{
	    platform_data->power_off();
	}
exit:
	return err;
}

static int __devexit apds990x_remove(struct i2c_client *client)
{
	struct apds990x_data *data = i2c_get_clientdata(client);

	input_unregister_device(data->input_dev_als);
	input_unregister_device(data->input_dev_ps);

	input_free_device(data->input_dev_als);
	input_free_device(data->input_dev_ps);

	//free_irq(APDS990x_INT, client);
    free_irq(client->irq,client);


	sysfs_remove_group(&client->dev.kobj, &apds990x_attr_group);

	/* Power down the device */
	apds990x_set_enable(client, 0);
	if(platform_data->power_off)
	{
	    platform_data->power_off();
	}
	kfree(data);

	return 0;
}

#ifdef CONFIG_PM
static int apds990x_suspend(struct i2c_client *client, pm_message_t mesg)
{
    struct apds990x_data *data = i2c_get_clientdata(client);
    unsigned long flags;
    dev_dbg(&client->dev,"%s: apds9900 try to suspend\n",__func__);
	if (!data->enable_ps_sensor && data->enable_als_sensor)
    {
       flag_work_enable =0;
	spin_lock_irqsave(&data->update_lock.wait_lock, flags);
       __cancel_delayed_work(&data->als_dwork);
       spin_unlock_irqrestore(&data->update_lock.wait_lock, flags);
       i2c_smbus_write_byte_data(client, CMD_BYTE|APDS990x_ENABLE_REG, 0);
    }

   return 0;
}

static int apds990x_resume(struct i2c_client *client)
{
    struct apds990x_data *data = i2c_get_clientdata(client);
    unsigned long flags;
    flag_work_enable =1;
    dev_dbg(&client->dev,"%s: apds9900 try to resume\n",__func__);
    i2c_smbus_write_byte_data(client, CMD_BYTE|APDS990x_ENABLE_REG, data->enable);
    if (!data->enable_ps_sensor && data->enable_als_sensor) 
    {
        spin_lock_irqsave(&data->update_lock.wait_lock, flags);

        __cancel_delayed_work(&data->als_dwork);
		schedule_delayed_work(&data->als_dwork,
			msecs_to_jiffies(data->als_poll_delay));

        spin_unlock_irqrestore(&data->update_lock.wait_lock, flags);
    }

    return 0;
}

#else

#define apds990x_suspend	NULL
#define apds990x_resume		NULL

#endif /* CONFIG_PM */

static const struct i2c_device_id apds990x_id[] = {
	{ "apds990x", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, apds990x_id);

static struct i2c_driver apds990x_driver = {
	.driver = {
		.name	= APDS990x_DRV_NAME,
		.owner	= THIS_MODULE,
	},
	.suspend = apds990x_suspend,
	.resume	= apds990x_resume,
	.probe	= apds990x_probe,
	.remove	= __devexit_p(apds990x_remove),
	.id_table = apds990x_id,
};

static int __init apds990x_init(void)
{
	return i2c_add_driver(&apds990x_driver);
}

static void __exit apds990x_exit(void)
{
	i2c_del_driver(&apds990x_driver);
}

MODULE_AUTHOR("Lee Kai Koon <kai-koon.lee@avagotech.com>");
MODULE_DESCRIPTION("APDS990x ambient light + proximity sensor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);

module_init(apds990x_init);
module_exit(apds990x_exit);

