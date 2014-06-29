/*
 * SiIxxxx <Firmware or Driver>
 *
 * Copyright (C) 2011 Silicon Image Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed .as is. WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the
 * GNU General Public License for more details.
*/


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/irq.h>
#include <linux/kobject.h>
#include <linux/io.h>
#include <linux/kthread.h>

#include <linux/bug.h>
#include <linux/err.h>
#include <linux/i2c.h>

#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/types.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <mach/gpio.h>

//#include "sii_9244_driver.h"
#include "sii_9244_api.h"
#include "si_mhl_tx_api.h"
#include "si_mhl_defs.h"
//#include "si_timer_cfg.h"
#include "sii_reg_access.h"
#include "si_drv_mhl_tx.h"
#include <linux/i2c/sii_9244.h>

#include <linux/i2c/twl.h>


//interrupt mode or polling mode for 9244 driver
#define SiI9244DRIVER_INTERRUPT_MODE   1

//sbit pinMHLTxVbus_CTRL = P0^4;    // VDD 5V to MHL VBUS switch control

#define    APP_DEMO_RCP_SEND_KEY_CODE 0x41

bool_tt    vbusPowerState = true;        // false: 0 = vbus output on; true: 1 = vbus output off;

static unsigned short Sii9244_key2code[] = {
    KEY_MENU,//139
    //KEY_SELECT,//0x161
    KEY_UP,//103
    KEY_LEFT,//105
    KEY_RIGHT,//106
    KEY_DOWN,//108
    //KEY_EXIT,//174
    KEY_1,   //0x02
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,  //0x10
    KEY_0,
    KEY_DOT,//52
    KEY_ENTER,//28
    KEY_CLEAR,//0x163 //DEL
    KEY_SOUND,//0x213
    KEY_PLAY,//207
    KEY_PAUSE,//119
    KEY_STOP,//128
    KEY_FASTFORWARD,//208
    KEY_REWIND,//168
    KEY_EJECTCD,//161
    KEY_FORWARD,//159
    KEY_BACK,//158
    KEY_PLAYCD,//200
    KEY_PAUSECD,//201
    KEY_STOP,//128
    KEY_F1,//59
    KEY_F2,//60
    KEY_F3,
    KEY_F4,
    KEY_F5,//63
    KEY_CHANNELUP, //0x192
    KEY_CHANNELDOWN, //0x193
    KEY_MUTE, //113
    KEY_PREVIOUS, //0x19c
    KEY_VOLUMEDOWN, //114
    KEY_VOLUMEUP, //115
    KEY_RECORD, // 167
    KEY_REPLY,
    KEY_PLAYPAUSE,
    KEY_PREVIOUSSONG,//w00185212 add
    KEY_NEXTSONG, //c00196901 add 2012-07-16
    KEY_BACKSPACE, //DEL
};
struct Sii9244_data {
    struct i2c_client *client;
    struct input_dev *input;
    unsigned short keycodes[ARRAY_SIZE(Sii9244_key2code)];
};

struct Sii9244_data *Sii9244;

struct regulator *regulator_l12 = NULL;


uint8_t PAGE_0_0X72;
uint8_t PAGE_1_0X7A;
uint8_t PAGE_2_0X92;
uint8_t PAGE_CBUS_0XC8;

static void init_PAGE_values(void)
{
    PAGE_0_0X72 = 0x76;
    PAGE_1_0X7A = 0x7E;
    PAGE_2_0X92 = 0x96;
    PAGE_CBUS_0XC8 = 0xCC;
}

#if (VBUS_POWER_CHK == ENABLE)
///////////////////////////////////////////////////////////////////////////////
//
// AppVbusControl
//
// This function or macro is invoked from MhlTx driver to ask application to
// control the VBUS power. If powerOn is sent as non-zero, one should assume
// peer does not need power so quickly remove VBUS power.
//
// if value of "powerOn" is 0, then application must turn the VBUS power on
// within 50ms of this call to meet MHL specs timing.
//
// Application module must provide this function.
//
void    AppVbusControl( bool_tt powerOn )
{
    if( powerOn )
    {
        //pinMHLTxVbus_CTRL = 1;
        MHLSinkOrDonglePowerStatusCheck();
        TX_API_PRINT(("[MHL]App: Peer's POW bit is set. Turn the VBUS power OFF here.\n"));
    }
    else
    {
        //pinMHLTxVbus_CTRL = 0;
        TX_API_PRINT(("[MHL]App: Peer's POW bit is cleared. Turn the VBUS power ON here.\n"));
    }
}
#endif



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
////////////////////////// Linux platform related //////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//Debug test
#undef dev_info
#define dev_info dev_err
#define MHL_DRIVER_NAME "sii9244drv"

/***** public type definitions ***********************************************/

typedef struct {
    struct task_struct    *pTaskStruct;
    uint8_t                pendingEvent;        // event data wait for retrieval
    uint8_t                pendingEventData;    // by user mode application

} MHL_DRIVER_CONTEXT_T, *PMHL_DRIVER_CONTEXT_T;


/***** global variables ********************************************/

MHL_DRIVER_CONTEXT_T gDriverContext;

struct i2c_client *mhl_Sii9244_page0 = NULL;
struct i2c_client *mhl_Sii9244_page1 = NULL;
struct i2c_client *mhl_Sii9244_page2 = NULL;
struct i2c_client *mhl_Sii9244_cbus = NULL;

struct platform_data {
    void (*reset) (void);
};
static struct platform_data *Sii9244_plat_data;

//------------------------------------------------------------------------------
// Array of timer values
//------------------------------------------------------------------------------


uint16_t Int_count=0;

static bool_tt match_id(const struct i2c_device_id *id, const struct i2c_client *client)
{
    if (strcmp(client->name, id->name) == 0)
        return true;

    return false;
}

static bool_tt Sii9244_mhl_reset(void)
{
    Sii9244_plat_data = mhl_Sii9244_page0->dev.platform_data;
    if (Sii9244_plat_data->reset){
        Sii9244_plat_data->reset();
        return true;
    }
    return false;
}


/*****************************************************************************/
/**
 * @brief Wait for the specified number of milliseconds to elapse.
 *
 *****************************************************************************/
void HalTimerWait(uint16_t m_sec)
{
    unsigned long    time_usec = m_sec * 1000;

    usleep_range(time_usec, time_usec);
    //mdelay(m_sec);
}


// I2C functions used by the driver.
//
//------------------------------------------------------------------------------
uint8_t I2C_ReadByte(uint8_t SlaveAddr, uint8_t RegAddr)    //oscar
{
    uint8_t ReadData = 0;
#if 0
    switch (SlaveAddr)
    {
        case PAGE_0_0X72:
            ReadData = i2c_smbus_read_byte_data(mhl_Sii9244_page0, RegAddr);
            break;
        case PAGE_1_0X7A:
            ReadData = i2c_smbus_read_byte_data(mhl_Sii9244_page1, RegAddr);
            break;
        case PAGE_2_0X92:
            ReadData = i2c_smbus_read_byte_data(mhl_Sii9244_page2, RegAddr);
            break;
        case PAGE_CBUS_0XC8:
            ReadData = i2c_smbus_read_byte_data(mhl_Sii9244_cbus, RegAddr);
            break;
    }
    #endif
    if (SlaveAddr == PAGE_0_0X72)
            ReadData = i2c_smbus_read_byte_data(mhl_Sii9244_page0, RegAddr);
    else if (SlaveAddr == PAGE_1_0X7A)
            ReadData = i2c_smbus_read_byte_data(mhl_Sii9244_page1, RegAddr);
    else if (SlaveAddr == PAGE_2_0X92)
            ReadData = i2c_smbus_read_byte_data(mhl_Sii9244_page2, RegAddr);
    else if (SlaveAddr == PAGE_CBUS_0XC8)
            ReadData = i2c_smbus_read_byte_data(mhl_Sii9244_cbus, RegAddr);

    return ReadData;
}

//------------------------------------------------------------------------------
void I2C_WriteByte(uint8_t SlaveAddr, uint8_t RegAddr, uint8_t Data)
{
#if 0
    switch (SlaveAddr)
    {
        case PAGE_0_0X72:
            i2c_smbus_write_byte_data(mhl_Sii9244_page0, RegAddr, Data);
            break;
        case PAGE_1_0X7A:
            i2c_smbus_write_byte_data(mhl_Sii9244_page1, RegAddr, Data);
            break;
        case PAGE_2_0X92:
            i2c_smbus_write_byte_data(mhl_Sii9244_page2, RegAddr, Data);
            break;
        case PAGE_CBUS_0XC8:
            i2c_smbus_write_byte_data(mhl_Sii9244_cbus, RegAddr, Data);
            break;
    }
#endif
    if (SlaveAddr == PAGE_0_0X72)
            i2c_smbus_write_byte_data(mhl_Sii9244_page0, RegAddr, Data);
    else if (SlaveAddr == PAGE_1_0X7A)
            i2c_smbus_write_byte_data(mhl_Sii9244_page1, RegAddr, Data);
    else if (SlaveAddr == PAGE_2_0X92)
            i2c_smbus_write_byte_data(mhl_Sii9244_page2, RegAddr, Data);
    else if (SlaveAddr == PAGE_CBUS_0XC8)
            i2c_smbus_write_byte_data(mhl_Sii9244_cbus, RegAddr, Data);
}
#ifdef SiI9244DRIVER_INTERRUPT_MODE

//------------------------------------------------------------------------------
void input_keycode(unsigned short key_code)
{    
    struct input_dev *input = NULL;

    TX_API_PRINT(("sii_9244_api.c input_keycode key_code =%d\n", key_code));

    input = (struct input_dev *)Sii9244->input;

    input_report_key(input, key_code, 1);
    input_sync(input);
    msleep(200);
    
    if(key_code == KEY_FASTFORWARD || key_code == KEY_REWIND)
    {
        TX_API_PRINT(("sii_9244_api.c input_keycode case1\n"));
        msleep(3000);
        input_report_key(input, key_code, 0);
        input_sync(input);
    }
    else
    {
        TX_API_PRINT(("sii_9244_api.c input_keycode case2\n"));
        input_report_key(input, key_code, 0);
        input_sync(input);
    }

    return;
}

static irqreturn_t Sii9244_mhl_interrupt(int irq, void *dev_id)
{
    //disable_irq_nosync(irq);

    //schedule_work(sii9244work);

    uint8_t Int_count=0;
    extern uint8_t    fwPowerState;

    //enable_irq(mhl_Sii9244_page0->irq);
    for(Int_count=0;Int_count<10;Int_count++){
        SiiMhlTxDeviceIsr();
        msleep(10);
        printk("Int_count=%d::::::::Sii9244 interrupt happened\n",Int_count);
        //SiiMhlTxGetEvents( &event, &eventParameter );
        #if 0
        if( MHL_TX_EVENT_NONE != event )
        {
            AppRcpDemo( event, eventParameter);
        }
        #endif
        if(POWER_STATE_D3 == fwPowerState)
                break;
    }
    //enable_irq(mhl_Sii9244_page0->irq);

    //printk("The sii9244 interrupt's top_half has been done and bottom_half will be processed..\n");
    //spin_unlock_irqrestore(&sii9244_lock, lock_flags);
    return IRQ_HANDLED;
}
#else
static int SiI9244_mhl_loop(void *nothing)
{
    //uint8_t    event;
    //uint8_t    eventParameter;

    printk("%s EventThread starting up\n", MHL_DRIVER_NAME);

       while (true)
        {
        /*
            Event loop
        */
        //
        // Look for any events that might have occurred.
        //
        //SiiMhlTxGetEvents( &event, &eventParameter );
        SiiMhlTxDeviceIsr();
        #if 0
        if( MHL_TX_EVENT_NONE != event )
        {
            AppRcpDemo( event, eventParameter);
        }
        #endif
        msleep(20);
        }
       return 0;
}


/*****************************************************************************/
/**
 * @brief Start driver's event monitoring thread.
 *
 *****************************************************************************/
void StartEventThread(void)
{
    gDriverContext.pTaskStruct = kthread_run(SiI9244_mhl_loop,
                                             &gDriverContext,
                                             MHL_DRIVER_NAME);
}


/*****************************************************************************/
/**
 * @brief Stop driver's event monitoring thread.
 *
 *****************************************************************************/
void  StopEventThread(void)
{
    kthread_stop(gDriverContext.pTaskStruct);

}
#endif

static struct i2c_device_id mhl_Sii9244_idtable[] = {
    {"mhl_Sii9244_page0", 0},
    {"mhl_Sii9244_page1", 0},
    {"mhl_Sii9244_page2", 0},
    {"mhl_Sii9244_cbus", 0},
};


/*
 * i2c client ftn.
 */
static int __devinit mhl_Sii9244_probe(struct i2c_client *client,
            const struct i2c_device_id *dev_id)
{
    int ret = 0;
    uint8_t     pollIntervalMs;
    //add
    int error,i;
    struct input_dev *input;

    TX_API_PRINT((KERN_INFO "%s:%d:\n", __func__,__LINE__));
    init_PAGE_values();

/*
    init_timer(&g_mhl_1ms_timer);
    g_mhl_1ms_timer.function = TimerTickHandler;
    g_mhl_1ms_timer.expires = jiffies + 10*HZ;
    add_timer(&g_mhl_1ms_timer);
*/
    if(match_id(&mhl_Sii9244_idtable[0], client))
    {
        mhl_Sii9244_page0 = client;
        dev_info(&client->adapter->dev, "attached %s "
            "into i2c adapter successfully\n", dev_id->name);
    }
    else if(match_id(&mhl_Sii9244_idtable[1], client))
    {
        mhl_Sii9244_page1 = client;
        dev_info(&client->adapter->dev, "attached %s "
            "into i2c adapter successfully \n", dev_id->name);
    }
    else if(match_id(&mhl_Sii9244_idtable[2], client))
    {
        mhl_Sii9244_page2 = client;
        dev_info(&client->adapter->dev, "attached %s "
            "into i2c adapter successfully \n", dev_id->name);
    }
    else if(match_id(&mhl_Sii9244_idtable[3], client))
    {
        mhl_Sii9244_cbus = client;
        dev_info(&client->adapter->dev, "attached %s "
            "into i2c adapter successfully\n", dev_id->name);

    }
    else
    {
        dev_info(&client->adapter->dev, "invalid i2c adapter: can not found dev_id matched\n");
        return -EIO;
    }

    regulator_l12 = regulator_get(NULL, "8921_l12");
    if (!IS_ERR(regulator_l12))
    {
        regulator_set_voltage(regulator_l12, 1200000, 1200000);
        regulator_enable(regulator_l12);
        pr_info("get pm8921_chg_reg_switch\n");
    }
    else
    {
        pr_info("Unable to get pm8921_chg_reg_switch\n");
        return -EIO;
    }

    if(mhl_Sii9244_page0 != NULL
        && mhl_Sii9244_page1 != NULL
        && mhl_Sii9244_page2 != NULL
        && mhl_Sii9244_cbus != NULL)
    {
        // Announce on RS232c port.
        //
        printk("\n============================================\n");
        printk("SiI9244 Linux Driver V1.22 \n");
        printk("============================================\n");

        Sii9244 = kzalloc(sizeof(struct Sii9244_data), GFP_KERNEL);
        input = input_allocate_device();
        if (!Sii9244 || !input) {
            dev_err(&client->dev, "insufficient memory\n");
		    error = -ENOMEM;
		    goto err_free_mem;
        }
        Sii9244->client = mhl_Sii9244_page0;
        Sii9244->input = input;
        dev_set_drvdata(&input->dev, Sii9244);
        input->name = "Sii9244";
        input->id.bustype = BUS_I2C;

        input->keycode = Sii9244->keycodes;
        input->keycodesize = sizeof(Sii9244->keycodes[0]);
        input->keycodemax = ARRAY_SIZE(Sii9244_key2code);

        __set_bit(EV_KEY, input->evbit);
        __clear_bit(EV_REP, input->evbit);

        for (i = 0; i < ARRAY_SIZE(Sii9244_key2code); i++) {
                Sii9244->keycodes[i] = Sii9244_key2code[i];
                input_set_capability(Sii9244->input, EV_KEY,
                            Sii9244->keycodes[i]);
        }
        __clear_bit(KEY_RESERVED, input->keybit);
        
        //
        // Initialize the registers as required. Setup firmware vars.
        //
        if(false == Sii9244_mhl_reset())
            return -EIO;

        //HalTimerInit ( );
        //HalTimerSet (TIMER_POLLING, MONITORING_PERIOD);


        SiiMhlTxInitialize( pollIntervalMs = MONITORING_PERIOD);

        #ifdef SiI9244DRIVER_INTERRUPT_MODE
        ret = request_threaded_irq(mhl_Sii9244_page0->irq, NULL, Sii9244_mhl_interrupt,IRQF_TRIGGER_LOW | IRQF_ONESHOT,//IRQ_TYPE_EDGE_FALLING,//IRQF_TRIGGER_LOW | IRQF_ONESHOT,//IRQ_TYPE_LEVEL_LOW,  //IRQ_TYPE_EDGE_FALLING,
                      mhl_Sii9244_page0->name, mhl_Sii9244_page0);
        if (ret){
            printk(KERN_INFO "%s:%d:Sii9244 interrupt failed\n", __func__,__LINE__);
            free_irq(mhl_Sii9244_page0->irq, mhl_Sii9244_page0->name);
            }

        else{
            enable_irq_wake(mhl_Sii9244_page0->irq);
            //disable_irq_nosync(mhl_Sii9244_page0->irq);
            printk(KERN_INFO "%s:%d:Sii9244 interrupt is sucessful\n", __func__,__LINE__);
            }

        #else
        StartEventThread();        /* begin monitoring for events */
        #endif

        error = input_register_device(Sii9244->input);
	  if (error) 
        {
            dev_err(&client->dev,"Failed to register input device\n");
            goto err;
	  }
    }
    return ret;
err:
	input_free_device(input);
err_free_mem:
	kfree(Sii9244);
	return error;
}

static int mhl_Sii9244_remove(struct i2c_client *client)
{
    dev_info(&client->adapter->dev, "detached s5p_mhl "
        "from i2c adapter successfully\n");

    if(regulator_l12)
    {
        regulator_disable(regulator_l12);
        regulator_put(regulator_l12);
    }
    
    return 0;
}

static int mhl_Sii9244_suspend(struct i2c_client *cl, pm_message_t mesg)
{
#if 0 
     printk("^_^-mhl_Sii9244_suspend\n");
     disable_irq(mhl_Sii9244_page0->irq);
     //disable_irq_nosync(mhl_Sii9244_page0->irq);
     msleep(300);
     twl_i2c_write_u8(TWL6030_MODULE_ID0, 0x00, 0xAD);
     twl_i2c_write_u8(TWL6030_MODULE_ID0, 0x00, 0xAF);
     printk("^_^-mhl_Sii9244_suspend\n");
    //twl_i2c_write_u8(TWL6030_MODULE_ID0, 0x00, 0x64);
    //twl_i2c_write_u8(TWL6030_MODULE_ID0, 0x00, 0x66);
#endif
    return 0;
};

static int mhl_Sii9244_resume(struct i2c_client *cl)
{
#if 0
    printk("^_^-mhl_Sii9244_resume^_^\n");
    uint8_t     pollIntervalMs;
    twl_i2c_write_u8(TWL6030_MODULE_ID0, 0x07, 0xAD);//3.3V
    twl_i2c_write_u8(TWL6030_MODULE_ID0, 0x21, 0xAF);
    //twl_i2c_write_u8(TWL6030_MODULE_ID0, 0x01, 0x64);
    //twl_i2c_write_u8(TWL6030_MODULE_ID0, 0x21, 0x66);
    msleep(300);
    printk("^_^-mhl_Sii9244_resume^_^\n");
    SiiMhlTxInitialize( pollIntervalMs = MONITORING_PERIOD);
    enable_irq(mhl_Sii9244_page0->irq);
#endif
    return 0;
};


MODULE_DEVICE_TABLE(i2c, mhl_Sii9244_idtable);

static struct i2c_driver mhl_Sii9244_driver = {
    .driver = {
        .name = "Sii9244_Driver",
    },
    .id_table     = mhl_Sii9244_idtable,
    .probe         = mhl_Sii9244_probe,
    .remove     = __devexit_p(mhl_Sii9244_remove),

    .suspend    = mhl_Sii9244_suspend,
    .resume     = mhl_Sii9244_resume,
};

static int __init mhl_Sii9244_init(void)
{
	if(!is_mhl_exist())
		return -ENODEV;
    return i2c_add_driver(&mhl_Sii9244_driver);
}

static void __exit mhl_Sii9244_exit(void)
{
    i2c_del_driver(&mhl_Sii9244_driver);
}


late_initcall(mhl_Sii9244_init);
module_exit(mhl_Sii9244_exit);

MODULE_VERSION("1.22");
MODULE_AUTHOR("gary <qiang.yuan@siliconimage.com>, Silicon image SZ office, Inc.");
MODULE_DESCRIPTION("sii9244 transmitter Linux driver");
MODULE_ALIAS("platform:MHL_sii9244");
