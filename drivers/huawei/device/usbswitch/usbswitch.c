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
/*========================================
  * header file include
  *=======================================*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/irq.h>
#include <linux/kobject.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <linux/kthread.h>
#include <linux/bug.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <mach/gpio.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/hrtimer.h>
#include <sound/jack.h>
#include <hsad/config_interface.h>
#include "usbswitch.h"

/*========================================
  * function declaration
  *=======================================*/
static int usbswitch_i2c_probe(struct i2c_client *client, const struct i2c_device_id *dev_id);
static int usbswitch_probe(struct platform_device *);
static int usbswitch_remove(struct i2c_client *);
static int usbswitch_suspend(struct i2c_client *, pm_message_t);
static int usbswitch_resume(struct i2c_client *);
static bool match_id(const struct i2c_device_id *, const struct i2c_client *);

/*========================================
  * global data definition
  *=======================================*/
static enum usbswitchstatus g_usbswitch_state = DEVICE_DISCONNECT;		//usbswitch state machine status

static bool need_config = false;						//usbswitch need re-config flags

static enum usbswitchbotton switchbotton;			//1-line headset press status	

static struct usbswitch_raw_staus usbswitch_raw_st ;	//usbswitch raw mode status

static struct task_struct * usbswitch_task;			//usbswitch thread task struct

struct i2c_client *i2cClient_usbswitch_main = NULL;

struct usbswitch_gpio_src usbswitch_gpio;	//usbswitch gpio source lists

struct usbswitch_struct* usbswitch_func_ptr;

extern struct usbswitch_struct fsa9485_func_ptr;
extern struct usbswitch_struct tsu6712_func_ptr;

struct platform_driver usbswitch_plat_driver = {
	.probe = usbswitch_probe,
	.driver = {
		.name = "usbswitch",
		.owner = THIS_MODULE,
	},
};

static struct i2c_device_id usbswitch_idtable[] = {
    {"usbswitch_main", 0},
};

static struct i2c_driver usbswitch_i2c_driver = {
	.driver = {
		.name = "usbswitch_driver",
	},
	.id_table 	= usbswitch_idtable,
	.probe     = usbswitch_i2c_probe,
	.remove 	= __devexit_p(usbswitch_remove),

	.suspend	= usbswitch_suspend,
	.resume 	= usbswitch_resume,
};


/*========================================
  * function definition
  *=======================================*/
static int usbswitch_remove(struct i2c_client *client)
{
    printk("detached usbswitch driver from i2c adapter\n");
    return 0;
}

/*
 * description: chip suspend
 */
static int usbswitch_suspend(struct i2c_client *client, pm_message_t mesg)
{
    //@add codes here

    return 0;
}

/*
 * description: chip resume
 */
static int usbswitch_resume(struct i2c_client *client)
{
    //@add codes here
    
    return 0;
}

/*
 * description: I2C client id match
 */
static bool match_id(const struct i2c_device_id *id, const struct i2c_client *client)
{
    if (strcmp(client->name, id->name) == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static int usbswitch_probe(struct platform_device *pdev)
{
    int ret;
	
    ret = i2c_add_driver(&usbswitch_i2c_driver);
    if (ret != 0)
		pr_err("Failed to register usbswitch I2C driver: %d\n", ret);
	return ret;
}


static void usbswitch_enable_raw_mode(void)
{
	usbswitch_func_ptr->enable_raw();
	usbswitch_raw_st.raw_enable_time = jiffies;
	usbswitch_raw_st.raw_enable = true;
}

static void usbswitch_disable_raw_mode(void)
{
	if(usbswitch_raw_st.raw_enable){
		usbswitch_func_ptr->disable_raw();
		usbswitch_raw_st.raw_enable = false;
	}
}

static bool usbswitch_raw_mode_is_on(void)
{
	return ((usbswitch_raw_st.raw_enable == true)?true:false);
}

static bool usbswitch_raw_mode_is_timeout(unsigned long now)
{
	return ((time_after(now, usbswitch_raw_st.raw_enable_time + 3*HZ))?true:false); //raw mode timeout time is 3sec
}


static void usbswitch_report_headset_insert(void)
{
	switchbotton = BOTTON_PRESS;
	tabla_hs_jack_report(SND_JACK_HEADPHONE_MONO,TABLA_JACK_MASK);
}
 
static void usbswitch_report_headset_press(void)
{
	switchbotton = BOTTON_RELEASE;
	tabla_hs_button_report(SND_JACK_BTN_0,SND_JACK_BTN_0);
}

static void usbswitch_report_headset_release(void)
{
	switchbotton = BOTTON_PRESS;
	tabla_hs_button_report(0,SND_JACK_BTN_0);
}

static bool usbswitch_assign_state(enum usbswitchstatus new_st)
{
	bool rtn = true;
	
	printk("usbswitch previou state: %d new state: %d\n", g_usbswitch_state, new_st);
	
	if(g_usbswitch_state != new_st){
		g_usbswitch_state = new_st;
		need_config = true;
	}
	else{
		rtn = false;
	}
	return rtn;	
}

bool usbswitch_config_main(enum usbswitchstatus usbswitch_state)
{
	bool rtn = true;
	
	switch(usbswitch_state)
	{
		case DEVICE_DISCONNECT:
			usbswitch_disable_raw_mode();
			rtn =usbswitch_func_ptr->config_default();
			break;
		case USBCHARGE_OR_DATA:
			rtn =usbswitch_func_ptr->config_usb_charge();
			break;
		case USBHEADPHONE:
			rtn =usbswitch_func_ptr->config_headphone();
			break;
		case USBHEADPHONE_2LINE:
			rtn =usbswitch_func_ptr->config_headphone();
			break;
		case MHL_OR_CRADLE:
			rtn =usbswitch_func_ptr->config_mhl_cradle();
			break;
		default:
			usbswitch_disable_raw_mode();
			rtn =usbswitch_func_ptr->config_default();
			break;
	}
	return rtn;
}

void usbswitch_state_process(void)
{
	struct usbswitch_int_val int_value ;
	enum usbswitchstatus tmp_status;
	
	if(false == usbswitch_func_ptr->int_detect(&int_value))
		goto end_process;
	
		do{
			switch(g_usbswitch_state)
			{
				case DEVICE_DISCONNECT://process disconnect state event: 1-exchange to other state
					if(need_config){
						usbswitch_config_main(DEVICE_DISCONNECT);
						need_config = false;
					}
					else{
						tmp_status = usbswitch_func_ptr->state_detect();
						usbswitch_assign_state(tmp_status);
					}
					break;
					
				case USBCHARGE_OR_DATA: //process mhl state event: 1-detach event; 2-exchange mhl mode
					if(need_config){ 
						usbswitch_config_main(USBCHARGE_OR_DATA);
						usbswitch_enable_raw_mode();
						need_config = false;
					}
					else{
							if(usbswitch_raw_mode_is_on()&&usbswitch_raw_mode_is_timeout(jiffies)){
								usbswitch_disable_raw_mode();
							}
							
							tmp_status=usbswitch_func_ptr->state_detect();

							if(USBCHARGE_OR_DATA !=	tmp_status){
								usbswitch_disable_raw_mode();
								usbswitch_assign_state(tmp_status);
							}
					}
					break;
					
				case USBHEADPHONE: //process headphone state event: 1-detach event;  2-headphone press;
					if(need_config){ 
						usbswitch_config_main(USBHEADPHONE);
						usbswitch_enable_raw_mode();
						usbswitch_report_headset_insert();
						need_config = false;
					}
					else{
						if((int_value.int2&INT2_MASK_ADCCHANGE)&&(usbswitch_func_ptr ->keypress_valid())){
							switch(switchbotton)
							{
								case BOTTON_PRESS:
									usbswitch_report_headset_press();
									break;
								case BOTTON_RELEASE:
									usbswitch_report_headset_release();
									break;
							}
						}
						if(int_value.int1&INT1_MASK_DETACH){
							usbswitch_assign_state(DEVICE_DISCONNECT);
							}

					}
					break;
					
				case USBHEADPHONE_2LINE: //process headphone_2line state event: 1-detach event
					if(need_config){ 
						usbswitch_config_main(USBHEADPHONE_2LINE);
						need_config = false;
					}
					else{
						if(int_value.int1&INT1_MASK_DETACH){
							usbswitch_assign_state(DEVICE_DISCONNECT);
							}
					}
					break;
					
				case MHL_OR_CRADLE://process mhl state event: 1-detach event
					if(need_config){ 
						usbswitch_config_main(MHL_OR_CRADLE);
						need_config = false;
					}
					else{
						if(!usbswitch_func_ptr->vbus_status()){
							usbswitch_assign_state(DEVICE_DISCONNECT);
							}
					}
					break;
					
				default://process error state
					usbswitch_assign_state(DEVICE_DISCONNECT);
					break;
			}	
		}while(need_config);
		
end_process:
	return;
}


static irqreturn_t usbswitch_interrupt(int irq, void *dev_id)
{    
	disable_irq_nosync(irq);
       wake_up_process(usbswitch_task);
	return IRQ_HANDLED;
}


bool usbswitch_detect_ic_model(void)
{
	bool rtn = true;
	s32 ControlData = 0;
	ControlData = i2c_smbus_read_byte_data(i2cClient_usbswitch_main, USBSWITCH_DEVICEID); 

	if(unlikely(ControlData<0))
	{
		rtn = false;
		printk("usbswitch read ic model failed\n");
		goto end_detect;
	}
	
	switch(ControlData&0xff)
	{
		case IC_MODEL_TSU6712:
			usbswitch_func_ptr = &tsu6712_func_ptr;
			printk("usbswitch ic model is TSU6712\n");
			break;
		case IC_MODEL_FSA9485:
			usbswitch_func_ptr = &fsa9485_func_ptr;
			printk("usbswitch ic model is FSA9485\n");
			break;
		default:
			rtn = false;
			printk("usbswitch unknown ic model\n");
			break;
	}
end_detect:
	return rtn;
}

static bool usbswitch_chip_init(void)
{
	bool rtn = false;
	enum usbswitchstatus usbswitch_init_status;
	
	// request gpio for usbswitch chip
	usbswitch_gpio.sel1_gpio = get_gpio_num_by_name("USB_SW_SEL1");
	if (gpio_request(usbswitch_gpio.sel1_gpio, "usbswitch_mhlswitch1")) 
	{
		printk("GPIO request for GPIO_CON_MHLSWITCH1 failed\n");
		goto chip_init_fail;
	}
	usbswitch_gpio.sel2_gpio = get_gpio_num_by_name("USB_SW_SEL2");
	if (gpio_request(usbswitch_gpio.sel2_gpio, "usbswitch_mhlswitch2")) 
	{
		printk("GPIO request for GPIO_CON_MHLSWITCH2 failed\n");
		gpio_free(usbswitch_gpio.sel1_gpio);
		goto chip_init_fail;
	}

	usbswitch_gpio.id_gpio = get_gpio_num_by_name("USB_ID_SW");
	if (gpio_request(usbswitch_gpio.id_gpio, "usbswitch_idswitch")) 
	{
		printk("GPIO request for GPIO_CON_IDSWITCH failed\n");
		gpio_free(usbswitch_gpio.sel1_gpio);
		gpio_free(usbswitch_gpio.sel2_gpio);
		goto chip_init_fail;
	}

	usbswitch_gpio.mic_gpio = get_gpio_num_by_name("USB_MIC_SW");
	if (gpio_request(usbswitch_gpio.mic_gpio, "usbswitch_micswitch")) 
	{
		printk("GPIO request for GPIO_CON_MICSWITCH failed\n");
		gpio_free(usbswitch_gpio.sel1_gpio);
		gpio_free(usbswitch_gpio.sel2_gpio);
		gpio_free(usbswitch_gpio.id_gpio);
		goto chip_init_fail;
	}

	usbswitch_func_ptr->init_chip();

	usbswitch_init_status = usbswitch_func_ptr->state_detect();
	if(false == usbswitch_config_main(usbswitch_init_status))
		goto chip_init_fail;

	g_usbswitch_state = usbswitch_init_status;
	printk("usbswitch init state is %d\n",usbswitch_init_status);

	if(USBHEADPHONE == usbswitch_init_status)
	{
		usbswitch_report_headset_insert();
	}

	rtn = true;
	
chip_init_fail:
	return rtn;
}


static bool usbswitch_init(void)
{
	bool rtn = false;
	int nRet = 0;
	
	/* read the id of chip */
	if(false == usbswitch_detect_ic_model()){
		goto error_process;
	}
	
	if(false == usbswitch_chip_init()){
		goto error_process;
	}

	nRet = request_irq(i2cClient_usbswitch_main->irq, usbswitch_interrupt, IRQ_TYPE_LEVEL_LOW,
						i2cClient_usbswitch_main->name, i2cClient_usbswitch_main);

	disable_irq_nosync(i2cClient_usbswitch_main->irq); //usbswitch thread can't process event now
	
	if (nRet)
	{
		printk( "usbswitch interrupt request failed\n");	
		free_irq(i2cClient_usbswitch_main->irq, i2cClient_usbswitch_main->name);
	}
	else
	{
		enable_irq_wake(i2cClient_usbswitch_main->irq);	
		rtn = true;
	}
	
error_process:
	return rtn;
}


static int usbswitch_thread(void * ptr)
{
	int rtn = 0;
	
	if(unlikely(false == usbswitch_init()))
	{
		printk("usbswitch thread init fail \n");
		return rtn;
	}
	
	while(1){
		__set_current_state(TASK_INTERRUPTIBLE);
		enable_irq(i2cClient_usbswitch_main->irq);
		schedule();
		usbswitch_state_process();
	}
}

static int usbswitch_i2c_probe(struct i2c_client *client, const struct i2c_device_id *dev_id)
{
	int nRet = 0;
	
	if(match_id(&usbswitch_idtable[0], client))
	{
	    i2cClient_usbswitch_main = client;
	}
	else
	{
	    return -EIO;
	}
	
	usbswitch_task = kthread_create_on_node(usbswitch_thread, NULL, cpu_to_node(0), "usbswitch");
	if (IS_ERR(usbswitch_task)) {
		printk("usbswitch thread creat failed\n");
		return -EIO;
	}
	wake_up_process(usbswitch_task);

	return nRet;
}


static int __init usbswitch_init_module(void)
{	
	if(!is_usbswitch_exist())
    {
        return -ENODEV;
    }
	return platform_driver_register(&usbswitch_plat_driver);
	
}

static void __exit usbswitch_exit_module(void)
{
    i2c_del_driver(&usbswitch_i2c_driver);
    platform_driver_unregister(&usbswitch_plat_driver);
    return;
}

/*========================================
  * module entrance
  *=======================================*/
module_init(usbswitch_init_module);
module_exit(usbswitch_exit_module);
MODULE_DESCRIPTION("usbswitch driver");
MODULE_LICENSE("GPL v2");
