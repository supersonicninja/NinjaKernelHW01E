/******************************************************************************
 *
 *  file name       : tuner_drv.c
 *  brief note      : The Control Layer of Tmm Tuner Driver
 *
 *  creation data   : 2011.07.25
 *  author          : K.Kitamura(*)
 *  special affairs : none
 *
 *  $Rev:: 322                        $ Revision of Last commit
 *  $Date:: 2011-10-26 13:33:02 +0900#$ Date of last commit
 *
 *              Copyright (C) 2011 by Panasonic Co., Ltd.
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
 *
 ******************************************************************************
 * HISTORY      : 2011/07/25    K.Kitamura(*)
 *                001 new creation
 ******************************************************************************/
/*..+....1....+....2....+....3....+....4....+....5....+....6....+....7....+...*/

/******************************************************************************
 * include
 ******************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/poll.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include "./../include/tuner_drv.h"
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/kthread.h>
#include <linux/version.h>
#include <linux/mutex.h>
/******************************************************************************
 * data
 ******************************************************************************/
/* for holding the address of the memory map */
void *mem_p;

/* poll control table */
wait_queue_head_t g_tuner_poll_wait_queue;       /* poll wait queue           */
spinlock_t        g_tuner_lock;                  /* spinlock resources        */
unsigned long     g_tuner_wakeup_flag;           /* poll wakeup flag          */

/* interrupt triger table */
unsigned char g_tuner_intcnd_f;                  /* INTCNDD_F register value  */
unsigned char g_tuner_intcnd_s;                  /* INTCNDD_S register value  */

/* kernel_thread */
struct task_struct *g_tuner_kthread_id;          /* kernel thread id          */
u32                 g_tuner_kthread_flag;        /* kernel thread wakeup flag */
wait_queue_head_t   g_tuner_kthread_wait_queue;  /* kernel thread wait queue  */

/* mutex */
struct mutex g_tuner_mutex;                      /* exclusion control         */
/******************************************************************************
 * function
 ******************************************************************************/
static ssize_t tuner_module_entry_read( struct file* FIle,
                                        char* Buffer,
                                        size_t Count,
                                        loff_t* OffsetPosition );
static ssize_t tuner_module_entry_write( struct file* FIle,
                                         const char* Buffer,
                                         size_t Count,
                                         loff_t* OffsetPosition );
static unsigned int tuner_module_entry_poll( struct file *file,
                                             struct poll_table_struct *poll_tbl );

static long tuner_module_entry_ioctl( struct file *file,
                                      unsigned int uCommand,
                                      unsigned long uArgument );
static int tuner_module_entry_open( struct inode* Inode,
                                    struct file* FIle );
static int tuner_module_entry_close( struct inode* Inode,
                                     struct file* FIle );
static int tuner_probe( struct platform_device *pdev );
static int tuner_remove( struct platform_device *pdev );
static int  __init tuner_drv_start( void );
static void __exit tuner_drv_end( void );

/* entry point */
static struct file_operations TunerFileOperations =
{
   .owner   = THIS_MODULE,
   .read    = tuner_module_entry_read,
   .write   = tuner_module_entry_write,
   .poll    = tuner_module_entry_poll,
   .unlocked_ioctl = tuner_module_entry_ioctl,
   .open    = tuner_module_entry_open,
   .release = tuner_module_entry_close
};

static struct platform_driver mmtuner_driver = {
    .probe  = tuner_probe,
    .remove = tuner_remove,
    .driver = { .name = "mmtuner",
                .owner = THIS_MODULE,
              }
};

static struct platform_device *mmtuner_device;
static struct class *device_class;

/* OPEN counter           */
unsigned long open_cnt;

irqreturn_t tuner_interrupt( int irq, void *dev_id );

/******************************************************************************
 * code area
 ******************************************************************************/

/******************************************************************************
 *    function:   tuner_probe
 *    brief   :   probe control of a driver
 *    date    :   2011.08.02
 *    author  :   K.Kitamura(*)
 *
 *    return  :    0                   normal exit
 *            :   -1                   error exit
 *    input   :   pdev
 *    output  :   none
 *    note    :   none
 ******************************************************************************/
static int tuner_probe(struct platform_device *pdev)
{
/* Tuner Driver ADD -S. */
    int ret = 0;
/* Tuner Driver ADD -E. */
    INFO_PRINT("mmtuner_probe: Called.\n");
#if 0 /* Tuner Driver DEL -S. */
    /* driver registration */
    if (register_chrdev(TUNER_CONFIG_DRV_MAJOR, TUNER_CONFIG_DRIVER_NAME, &TunerFileOperations))
    {
        ERROR_PRINT("mmtuner_probe: register_chrdev()\
                     Failed Major:%d.\n", TUNER_CONFIG_DRV_MAJOR);
        return -1;
    }
#endif /* Tuner Driver DEL -E. */

/* Tuner Driver ADD -S. */
    /* acquisition of pm8921 regulator(L29) */
    ret = tuner_pm8921_regulator_l29_setup(pdev);
    if(ret < 0)
    {
        ERROR_PRINT("[%d][%s] regulator_l29_setup error: %d\n", __LINE__, __func__, ret);
        return -1;
    }

    /* acquisition of pm8921 regulator(LVS2) */
    ret = tuner_pm8921_regulator_lvs2_setup(pdev);
    if(ret < 0)
    {
        ERROR_PRINT("[%d][%s] regulator_lvs2_setup error: %d\n", __LINE__, __func__, ret);
        return -1;
    }
/* Tuner Driver ADD -E. */
    /* initialization of external variables */
    init_waitqueue_head( &g_tuner_poll_wait_queue );
    spin_lock_init( &g_tuner_lock );
    g_tuner_wakeup_flag = TUNER_OFF;
    g_tuner_intcnd_f = 0x00;
    g_tuner_intcnd_s = 0x00;
    open_cnt         = 0;
    mutex_init(&g_tuner_mutex);

    INFO_PRINT("tuner_probe: END.\n");
    /* normal exit */
    return 0;
}

/******************************************************************************
 *    function:   tuner_remove
 *    brief   :   remove control of a driver
 *    date    :   2011.08.02
 *    author  :   K.Kitamura(*)
 *
 *    return  :    0                   normal exit
 *            :   -1                   error exit
 *    input   :   pdev
 *    output  :   none
 *    note    :   none
 ******************************************************************************/
static int tuner_remove(struct platform_device *pdev)
{
    INFO_PRINT("tuner_remove: Called.\n");
    TRACE();

    /* release interrupt */
    tuner_drv_release_interrupt();
#if 0 /* Tuner Driver DEL -S. */
    /* unregistration the character device */
    unregister_chrdev(TUNER_CONFIG_DRV_MAJOR, TUNER_CONFIG_DRIVER_NAME);
#endif /* Tuner Driver DEL -E. */
/* Tuner Driver ADD -S. */
    /* regulator release. */
    tuner_pm8921_regulator_release();
/* Tuner Driver ADD -E. */
    INFO_PRINT("tuner_remove: END.\n");
    /* normal exit */
    return 0;
}


/******************************************************************************
 *    function:   tuner_kernel_thread
 *    brief   :   kernel_thread of mmtuner driver
 *    date    :   2011.09.16
 *    author  :   K.Kitamura(*)
 *
 *    return  :    0                   normal exit
 *            :   -1                   error exit
 *    input   :   none
 *    output  :   none
 *    note    :   none
 ******************************************************************************/
int tuner_kernel_thread( void * arg )
{
    int ret = 0;
    unsigned long flags;
    unsigned long ktread_flg;
    TUNER_DATA_RW rw_data[ 2 ];                  /* setting data */
    mm_segment_t    oldfs;
    struct sched_param  param; 

    INFO_PRINT("tuner_kernel_thread: START.\n");

    /* initialize internal variables */
    ret = 0;
    flags = 0;
    ktread_flg = 0;
    param.sched_priority = TUNER_CONFIG_KTH_PRI;

    daemonize( "tuner_kthread" );
    oldfs = get_fs();
    set_fs( KERNEL_DS );
    ret = sched_setscheduler( g_tuner_kthread_id, SCHED_FIFO, &param );
    set_fs( oldfs );

    while(1)
    {
        DEBUG_PRINT("tuner_kernel_thread waiting... ");
        wait_event_interruptible( g_tuner_kthread_wait_queue, g_tuner_kthread_flag );

        spin_lock_irqsave( &g_tuner_lock, flags );
        ktread_flg = g_tuner_kthread_flag;
        spin_unlock_irqrestore( &g_tuner_lock, flags);

        /* read request interrupt factor */
        if ( ( ktread_flg & TUNER_KTH_IRQHANDLER ) == TUNER_KTH_IRQHANDLER )
        {
            DEBUG_PRINT("tuner_kernel_thread IRQHANDLER start ");

            /* read data */
            /* INTCND_F */
            rw_data[ 0 ].slave_adr = TUNER_SLAVE_ADR_M1;
            rw_data[ 0 ].adr       = TUNER_DRV_ADR_INTCND_F;
            rw_data[ 0 ].sbit      = TUNER_DRV_ENAS_ALL;
            rw_data[ 0 ].ebit      = TUNER_DRV_ENAE_ALL;
            rw_data[ 0 ].enabit    = TUNER_DRV_ENA_ALL;
            rw_data[ 0 ].param     = TUNER_DRV_PARAM_RINIT;
            /* INTCND_S */
            rw_data[ 1 ].slave_adr = TUNER_SLAVE_ADR_M2;
            rw_data[ 1 ].adr       = TUNER_DRV_ADR_INTCND_S;
            rw_data[ 1 ].sbit      = TUNER_DRV_ENAS_ALL;
            rw_data[ 1 ].ebit      = TUNER_DRV_ENAE_ALL;
            rw_data[ 1 ].enabit    = TUNER_DRV_ENA_ALL;
            rw_data[ 1 ].param     = TUNER_DRV_PARAM_RINIT;

            /* read interrupt factor */
            ret = tuner_drv_hw_access( TUNER_IOCTL_VALGET,
                                       rw_data,
                                       2 );
            /* Error checking */
            if( ret != 0 )
            {
                DEBUG_PRINT( " tuner_kernel_thread i2c_read Err \n" );
                /* If the result is NG interrupt factor reading,
                   clearing all bits */
                rw_data[ 0 ].param = TUNER_DRV_PARAM_ALL;
                rw_data[ 1 ].param = TUNER_DRV_PARAM_ALL;
            }
            else
            {
                /* If results are normal reading of interrupt factor 
                   to save the interrupt factor */
                /* To store the interrupt factor */
                g_tuner_intcnd_f |= ( unsigned char )rw_data[ 0 ].param;
                g_tuner_intcnd_s |= ( unsigned char )rw_data[ 1 ].param;
            }

            DEBUG_PRINT( "!!! g_tuner_intcnd_f : 0x%x !!!\n"
                                         ,g_tuner_intcnd_f );
            DEBUG_PRINT( "!!! g_tuner_intcnd_s : 0x%x !!!\n"
                                         ,g_tuner_intcnd_s );

            /* interrupt factor is cleared */
            ret = tuner_drv_hw_access( TUNER_IOCTL_VALSET,
                                       rw_data,
                                       2 );
            /* Error checking */
            if( ret != 0 )
            {
                break;
            }

            /* poll wait released */
            g_tuner_wakeup_flag = TUNER_ON;
            wake_up( &g_tuner_poll_wait_queue );

            DEBUG_PRINT("tuner_interrupt end ");

            spin_lock_irqsave( &g_tuner_lock, flags );
            g_tuner_kthread_flag &= ~TUNER_KTH_IRQHANDLER;
            spin_unlock_irqrestore( &g_tuner_lock, flags );

#ifdef TUNER_CONFIG_IRQ_LEVELTRIGGER
            /* If the interrupt level, re-enable interrupts */
            tuner_drv_enable_interrupt();
#endif /* TUNER_CONFIG_IRQ_LEVELTRIGGER */
        }

        /* kernel thread termination request */
        if ( ( ktread_flg & TUNER_KTH_END ) == TUNER_KTH_END )
        {
            DEBUG_PRINT("tuner_kernel_thread KTH_END start ");
            spin_lock_irqsave( &g_tuner_lock, flags );
            g_tuner_kthread_flag &= ~TUNER_KTH_END;
            spin_unlock_irqrestore( &g_tuner_lock, flags );
            break;
        }
    }

    INFO_PRINT("tuner_kernel_thread: END.\n");
    /* normal exit */
    return 0;
}


/******************************************************************************
 *    function:   tuner_drv_start
 *    brief   :   initialization control of a driver
 *    date    :   2011.08.02
 *    author  :   K.Kitamura(*)
 *
 *    return  :    0                   normal exit
 *            :   -1                   error exit
 *    input   :   none
 *    output  :   none
 *    note    :   none
 ******************************************************************************/
static int __init tuner_drv_start(void)
{
    int ret =0;
    struct device *dev = NULL;

    INFO_PRINT("mmtuner_tuner_drv_start: Called\n");

    /* driver registration */
    ret = platform_driver_register(&mmtuner_driver);

    /* Error checking */
    if( ret != 0 )
    {
        ERROR_PRINT("init_module: Error:\
                     failed in platform_driver_register.\n");
        return ret;
    }

    /* allocate memory */
    mmtuner_device = platform_device_alloc("mmtuner", -1);

    if (!mmtuner_device)
    {
        ERROR_PRINT("init_module: Error: failed in platform_device_alloc.\n");
        platform_driver_unregister(&mmtuner_driver);
        return -ENOMEM;
    }

    /* add device */
    ret = platform_device_add(mmtuner_device);
    if ( ret )
    {
        ERROR_PRINT("init_module: Error: failed in platform_device_add.\n");
        platform_device_put(mmtuner_device);
        platform_driver_unregister(&mmtuner_driver);
        return ret;
    }
    /* device node (class) create */
    device_class = class_create(THIS_MODULE, "mmtuner");
    if (IS_ERR(device_class)) 
    {
        ERROR_PRINT("init_module: Error: failed in class_create.\n");
        platform_device_put(mmtuner_device);
        platform_driver_unregister(&mmtuner_driver);
        return PTR_ERR(device_class);
    }

#if 1 /* Tuner Driver ADD -S. */
    /* character device registration */
    ret = register_chrdev(TUNER_CONFIG_DRV_MAJOR, TUNER_CONFIG_DRIVER_NAME, &TunerFileOperations);
    if (ret < 0)
    {
        ERROR_PRINT("init_module: Error: failed in register_chrdev.\n");
        platform_device_put(mmtuner_device);
        platform_driver_unregister(&mmtuner_driver);
        return ret;
    }
#endif /* Tuner Driver ADD -E. */

    /* device create */
    dev = device_create (device_class, NULL, MKDEV(TUNER_CONFIG_DRV_MAJOR, TUNER_CONFIG_DRV_MINOR), NULL, "mmtuner");

    if(IS_ERR(dev))
    {
        ERROR_PRINT("init_module: Error: failed in device_create.\n");
        platform_device_put(mmtuner_device);
        platform_driver_unregister(&mmtuner_driver);
        return PTR_ERR(dev);
    }
   
    /* generation process kernel thread */
    g_tuner_kthread_flag = TUNER_KTH_NONE;

    init_waitqueue_head( &g_tuner_kthread_wait_queue );

    g_tuner_kthread_id = kthread_create( tuner_kernel_thread,
                                         NULL,
                                         "tuner_kthread" );
    if( IS_ERR( g_tuner_kthread_id ) )
    {
        g_tuner_kthread_id = NULL;
        platform_device_put(mmtuner_device);
        platform_driver_unregister(&mmtuner_driver);
        return -EIO;
    }
    /* wake up*/
    wake_up_process( g_tuner_kthread_id );

    INFO_PRINT("mmtuner_tuner_drv_start: END\n");
    /* normal exit */
    return 0;
}

/******************************************************************************
 *    function:   tuner_drv_end
 *    brief   :   exit control of a driver
 *    date    :   2011.08.02
 *    author  :   K.Kitamura(*)
 *
 *    return  :   none
 *    input   :   none
 *    output  :   none
 *    note    :   none
 ******************************************************************************/
static void __exit tuner_drv_end(void)
{
    INFO_PRINT("mmtuner_tuner_drv_end: Called\n");

    /* to terminate the kernel thread */
    g_tuner_kthread_flag |= TUNER_KTH_END;
    if( waitqueue_active( &g_tuner_kthread_wait_queue ))
    {
        wake_up( &g_tuner_kthread_wait_queue );
    }

    /* to unregister the kernel thread */
    if( g_tuner_kthread_id )
    {
        kthread_stop( g_tuner_kthread_id );
    }

    /* remove device */
    device_destroy(device_class, MKDEV(TUNER_CONFIG_DRV_MAJOR, TUNER_CONFIG_DRV_MINOR));
    /* remove class */
    class_destroy(device_class);
#if 1 /* Tuner Driver ADD -S. */
    /* unregistration the character device */
    unregister_chrdev(TUNER_CONFIG_DRV_MAJOR, TUNER_CONFIG_DRIVER_NAME);
#endif /* Tuner Driver ADD -E. */
    /* unregistration the device */
    platform_device_unregister(mmtuner_device);
    /* unregistration the driver */
    platform_driver_unregister(&mmtuner_driver);

    INFO_PRINT("mmtuner_tuner_drv_end: END\n");
}

/******************************************************************************
 *    function:   tuner_module_entry_open
 *    brief   :   open control of a driver
 *    date    :   2011.08.02
 *    author  :   K.Kitamura(*)
 *
 *    return  :    0                   normal exit
 *            :   -1                   error exit
 *    input   :   Inode
 *            :   FIle
 *    output  :   none
 *    note    :   none
 ******************************************************************************/
static int tuner_module_entry_open(struct inode* Inode, struct file* FIle)
{

    INFO_PRINT("tuner_module_entry_open: Called\n");

#ifdef  TUNER_CONFIG_DRV_MULTI      /* enable multiple open  */
    open_cnt++;
#else  /* TUNER_CONFIG_DRV_MULTI */ /* disable multiple open */
    /* If it is already an open */
    if( open_cnt > 0 )
    {
        INFO_PRINT("tuner_module_entry_open: open error\n");
        return -1;
    }
    /* If the new open */
    else
    {
        INFO_PRINT("tuner_module_entry_open: open_cnt = 1\n");
        open_cnt++;
    }
#endif /* TUNER_CONFIG_DRV_MULTI */

    /* normal exit */
    return 0;
}

/******************************************************************************
 *    function:   tuner_module_entry_close
 *    brief   :   close control of a driver
 *    date    :   2011.08.02
 *    author  :   K.Kitamura(*)
 *
 *    return  :    0                   normal exit
 *            :   -1                   error exit
 *    input   :   Inode
 *            :   FIle
 *    output  :   none
 *    note    :   none
 ******************************************************************************/
static int tuner_module_entry_close(struct inode* Inode, struct file* FIle)
{
    struct devone_data *dev;

    INFO_PRINT("tuner_module_entry_close: Called\n");

    /* If zero or less open_cnt */
    if( open_cnt <= 0 )
    {
        INFO_PRINT("tuner_module_entry_close: close error\n");
        return -1;
    }
    else
    {
        open_cnt--;
    }

    /* If you close all the open */
    if( open_cnt == 0 )
    {
        /* release interrupt */
        tuner_drv_release_interrupt();

        if( FIle == NULL )
        {
            return -1;
        }

        dev = FIle->private_data;

        if( dev )
        {
            kfree( dev );
        }
    }

    /* normal exit */
    return 0;

}

/******************************************************************************
 *    function:   tuner_module_entry_read
 *    brief   :   read control of a driver
 *    date    :   2011.08.02
 *    author  :   K.Kitamura(*)
 *
 *    return  :    0                   normal exit
 *            :   -1                   error exit
 *    input   :   FIle
 *            :   Buffer
 *            :   Count
 *            :   OffsetPosition
 *    output  :   none
 *    note    :   none
 ******************************************************************************/
static ssize_t tuner_module_entry_read(struct file * FIle, char * Buffer,
                                 size_t Count, loff_t * OffsetPosition)
{
    /* No operation */
    /* normal exit */
    return 0;

}

/******************************************************************************
 *    function:   tuner_module_entry_write
 *    brief   :   write control of a driver
 *    date    :   2011.08.02
 *    author  :   K.Kitamura(*)
 *
 *    return  :    0                   normal exit
 *            :   -1                   error exit
 *    input   :   FIle
 *            :   Buffer
 *            :   Count
 *            :   OffsetPosition
 *    output  :   none
 *    note    :   none
 ******************************************************************************/
static ssize_t tuner_module_entry_write(struct file* FIle, const char* Buffer,
                                  size_t Count, loff_t* OffsetPosition)
{
    int           ret;
    unsigned long copy_ret;
    TUNER_DATA_RW *write_arg;            /* pointer to a data storage area    */

    /* destination area to ensure */
    write_arg  = ( TUNER_DATA_RW * )vmalloc( Count );
    if( write_arg == NULL )
    {
        return -EINVAL;
    }

    /* get value from userspace */
    copy_ret = copy_from_user( write_arg,
                               Buffer,
                               Count );
    /* Error checking */
    if( copy_ret != 0 )
    {
        /* to free up space destination */
        vfree( write_arg );
        return -EINVAL;
    }

    ret = tuner_drv_hw_access(
              TUNER_IOCTL_VALSET, 
              write_arg,
              ( unsigned short )( Count / sizeof( TUNER_DATA_RW )));

    /* to free up space destination */
    vfree( write_arg );

    return ret;
}

/******************************************************************************
 *    function:   tuner_module_entry_ioctl
 *    brief   :   ioctl control of a driver
 *    date    :   2011.08.02
 *    author  :   K.Kitamura(*)
 *
 *    return  :    0                   normal exit
 *            :   -1                   error exit
 *    input   :   Inode
 *            :   FIle
 *            :   uCommand
 *            :   uArgument
 *    output  :   none
 *    note    :   none
 ******************************************************************************/
static long tuner_module_entry_ioctl(struct file *file,
                              unsigned int uCommand, unsigned long uArgument)
{
    int ret=0;
    TUNER_DATA_RW         data;
    unsigned long         copy_ret;
    int                   param;
    TUNER_DATA_RW         event_status[ TUNER_EVENT_REGNUM ];

    INFO_PRINT("tuner_module_entry_ioctl:START\n");

#if 0 /* Tuner Driver DEL -S. */
    /* exclusive control: start rock */
    mutex_lock(&g_tuner_mutex);
#endif /* Tuner Driver DEL -E. */

    /* argument checking */
    if( uArgument == 0 )
    {
        TRACE();
#if 0 /* Tuner Driver DEL -S. */
        /* exclusive control: unlocking */
        mutex_unlock(&g_tuner_mutex);
#endif /* Tuner Driver DEL -E. */ 
        return -EINVAL;
    }

/* Tuner Driver ADD -S. */
    /* exclusive control: start rock */
    mutex_lock(&g_tuner_mutex);
/* Tuner Driver ADD -E. */

    switch( uCommand )
    {
        /* retrieve data from the HW */
        case TUNER_IOCTL_VALGET:
            /* get value from userspace */
            copy_ret = copy_from_user( &data,
                                       &( *(TUNER_DATA_RW *)uArgument ),
                                       sizeof( TUNER_DATA_RW ));
            /* Error checking */
            if( copy_ret != 0 )
            {
                TRACE();
                /* exclusive control: unlocking */
                mutex_unlock(&g_tuner_mutex);
                return -EINVAL;
            }

            ret = tuner_drv_hw_access( uCommand, &data, 1 );

            if( ret == 0 )
            {
                /* read values returned to userspace */
                copy_ret = copy_to_user( &( *(TUNER_DATA_RW *)uArgument ),
                                         &data,
                                         sizeof( TUNER_DATA_RW ));
                /* Error checking */
                if( copy_ret != 0 )
                {
                    TRACE();
                    /* exclusive control: unlocking */
                    mutex_unlock(&g_tuner_mutex);
                    return -EINVAL;
                }
            }

            break;


        /* data set to HW */
        case TUNER_IOCTL_VALSET:
            /* get value from userspace */
            copy_ret = copy_from_user( &data,
                                       &( *(TUNER_DATA_RW *)uArgument ),
                                       sizeof( TUNER_DATA_RW ));
            /* Error checking */
            if( copy_ret != 0 )
            {
                TRACE();
                /* exclusive control: unlocking */
                mutex_unlock(&g_tuner_mutex);
                return -EINVAL;
            }

            ret = tuner_drv_hw_access( uCommand, &data, 1 );
            break;

        case TUNER_IOCTL_VALGET_EVENT:
            /* INTCNT_F */
            /* to userspace */
            copy_ret = copy_to_user( &( *( unsigned char *)uArgument ),
                                     &g_tuner_intcnd_f,
                                     sizeof( unsigned char ));
            /* Error checking */
            if( copy_ret != 0 )
            {
                TRACE();
                /* exclusive control: unlocking */
                mutex_unlock(&g_tuner_mutex);
                return -EINVAL;
            }

            /* INTCNT_S */
            /* to userspace */
            copy_ret = copy_to_user( &( *( unsigned char *)( uArgument + 1 )),
                                     &g_tuner_intcnd_s,
                                     sizeof( unsigned char ));
            /* Error checking */
            if( copy_ret != 0 )
            {
                TRACE();
                /* exclusive control: unlocking */
                mutex_unlock(&g_tuner_mutex);
                return -EINVAL;
            }

            DEBUG_PRINT("!!! IOCTL g_tuner_intcnd_f : 0x%x !!!\n"
                                               ,g_tuner_intcnd_f );
            DEBUG_PRINT("!!! IOCTL g_tuner_intcnd_s : 0x%x !!!\n"
                                               ,g_tuner_intcnd_s );
    
            /* factors initialization */
            g_tuner_intcnd_f = 0x00;
            g_tuner_intcnd_s = 0x00;

            ret = copy_ret;

            break;
        /* event set */
        case TUNER_IOCTL_VALSET_EVENT:
            /* get value from userspace */
            copy_ret = copy_from_user( &data,
                                       &( *(TUNER_DATA_RW *)uArgument ),
                                       sizeof( TUNER_DATA_RW ));
            /* Error checking */
            if( copy_ret != 0 )
            {
                TRACE();
                /* exclusive control: unlocking */
                mutex_unlock(&g_tuner_mutex);
                return -EINVAL;
            }

            /* If the first event settings and enable interrupts */
            event_status[0].slave_adr = TUNER_SLAVE_ADR_M1;  /* slave address                */
            event_status[0].adr       = TUNER_ADR_INTDEF1_F; /* register address             */
            event_status[0].sbit      = SIG_ENAS_INTDEF1_F;  /* specify the start bit        */
            event_status[0].ebit      = SIG_ENAE_INTDEF1_F;  /* specify the end bit          */
            event_status[0].param     = 0x00;                /* keeping the writing, reading */
            event_status[0].enabit    = SIG_ENA_INTDEF1_F;   /* specify the enable bit       */
            event_status[1].slave_adr = TUNER_SLAVE_ADR_M1;  /* slave address                */
            event_status[1].adr       = TUNER_ADR_INTDEF2_F; /* register address             */
            event_status[1].sbit      = SIG_ENAS_INTDEF2_F;  /* specify the start bit        */
            event_status[1].ebit      = SIG_ENAE_INTDEF2_F;  /* specify the end bit          */
            event_status[1].param     = 0x00;                /* keeping the writing, reading */
            event_status[1].enabit    = SIG_ENA_INTDEF2_F;   /* specify the enable bit       */
            event_status[2].slave_adr = TUNER_SLAVE_ADR_M2;  /* slave address                */
            event_status[2].adr       = TUNER_ADR_INTDEF_S;  /* register address             */
            event_status[2].sbit      = SIG_ENAS_IERR_S;     /* specify the start bit        */
            event_status[2].ebit      = SIG_ENAE_SEGERRS;    /* specify the end bit          */
            event_status[2].param     = 0x00;                /* keeping the writing, reading */
            event_status[2].enabit    = ( SIG_ENA_IERR_S 
                                         |SIG_ENA_EMGFLG_S 
                                         |SIG_ENA_TMCCERR_S
                                         |SIG_ENA_ISYNC_S 
                                         |SIG_ENA_CDFLG_S 
                                         |SIG_ENA_SSEQ_S 
                                         |SIG_ENA_MGERR_S 
                                         |SIG_ENA_SEGERRS ) ; /* specify the enable bit       */

            ret = tuner_drv_hw_access( TUNER_IOCTL_VALGET, event_status, TUNER_EVENT_REGNUM );

            /* Error checking */
            if( ret != 0 )
            {
                TRACE();
                /* exclusive control: unlocking */
                mutex_unlock(&g_tuner_mutex);
                return -EINVAL;  
            }

            if( ( (event_status[0].param&event_status[0].enabit) == SIG_ENAIRQ_INTDEF1_NONE ) &&
                ( (event_status[1].param&event_status[1].enabit) == SIG_ENAIRQ_INTDEF2_NONE ) && 
                ( (event_status[2].param&event_status[2].enabit) == SIG_ENAIRQ_INTDEF_NONE ) )
            {
                ret = tuner_drv_set_interrupt();

                /* Error checking */
                if( ret != 0 )
                {
                    TRACE();
                /* exclusive control: unlocking */
                mutex_unlock(&g_tuner_mutex);
                return -EINVAL;  
                }
            }

            ret = tuner_drv_hw_access( TUNER_IOCTL_VALSET, &data, 1 );

            break;
        /* clear event */
        case TUNER_IOCTL_VALREL_EVENT:
            /* get value from userspace */
            copy_ret = copy_from_user( &data,
                                       &( *(TUNER_DATA_RW *)uArgument ),
                                       sizeof( TUNER_DATA_RW ));
            /* Error checking */
            if( copy_ret != 0 )
            {
                TRACE();
                /* exclusive control: unlocking */
                mutex_unlock(&g_tuner_mutex);
                return -EINVAL;
            }

            ret = tuner_drv_hw_access( TUNER_IOCTL_VALSET, &data, 1 );

            /* Error checking */
            if( ret != 0 )
            {
                TRACE();
                /* exclusive control: unlocking */
                mutex_unlock(&g_tuner_mutex);
                return -EINVAL;
            }

            /* If the first event settings and disble interrupts */
            /* event_status[0] */
            event_status[0].slave_adr = TUNER_SLAVE_ADR_M1;  /* slave address                */
            event_status[0].adr       = TUNER_ADR_INTDEF1_F; /* register address             */
            event_status[0].sbit      = SIG_ENAS_INTDEF1_F;  /* specify the start bit        */
            event_status[0].ebit      = SIG_ENAE_INTDEF1_F;  /* specify the end bit          */
            event_status[0].param     = 0x00;                /* keeping the writing, reading */
            event_status[0].enabit    = SIG_ENA_INTDEF1_F;   /* specify the enable bit       */

            /* event_status[1] */
            event_status[1].slave_adr = TUNER_SLAVE_ADR_M1;  /* slave address                */
            event_status[1].adr       = TUNER_ADR_INTDEF2_F; /* register address             */
            event_status[1].sbit      = SIG_ENAS_INTDEF2_F;  /* specify the start bit        */
            event_status[1].ebit      = SIG_ENAE_INTDEF2_F;  /* specify the end bit          */
            event_status[1].param     = 0x00;                /* keeping the writing, reading */
            event_status[1].enabit    = SIG_ENA_INTDEF2_F;   /* specify the enable bit       */

            /* event_status[2] */
            event_status[2].slave_adr = TUNER_SLAVE_ADR_M2;  /* slave address                */
            event_status[2].adr       = TUNER_ADR_INTDEF_S;  /* register address             */
            event_status[2].sbit      = SIG_ENAS_IERR_S;     /* specify the start bit        */
            event_status[2].ebit      = SIG_ENAE_SEGERRS;    /* specify the end bit          */
            event_status[2].param     = 0x00;                /* keeping the writing, reading */
            event_status[2].enabit    = ( SIG_ENA_IERR_S 
                                         |SIG_ENA_EMGFLG_S 
                                         |SIG_ENA_TMCCERR_S
                                         |SIG_ENA_ISYNC_S 
                                         |SIG_ENA_CDFLG_S 
                                         |SIG_ENA_SSEQ_S 
                                         |SIG_ENA_MGERR_S 
                                         |SIG_ENA_SEGERRS ) ; /* specify the enable bit       */

            ret = tuner_drv_hw_access( TUNER_IOCTL_VALGET, event_status, TUNER_EVENT_REGNUM );

            if( ( (event_status[0].param&event_status[0].enabit) == SIG_ENAIRQ_INTDEF1_NONE ) &&
                ( (event_status[1].param&event_status[1].enabit) == SIG_ENAIRQ_INTDEF2_NONE ) && 
                ( (event_status[2].param&event_status[2].enabit) == SIG_ENAIRQ_INTDEF_NONE ) )
            {
                tuner_drv_release_interrupt();
            }

            break;
        case TUNER_IOCTL_VALSET_POWER:
            /* get value from userspace */
            copy_ret = copy_from_user( &param,
                                       &( *( int * )uArgument ),
                                       sizeof( int ));
            /* Error checking */
            if( copy_ret != 0 )
            {
                TRACE();
                /* exclusive control: unlocking */
                mutex_unlock(&g_tuner_mutex);
                return -EINVAL;
            }
            ret = tuner_drv_ctl_power( param );
            break;

        case TUNER_IOCTL_VALSET_RESET:
            DEBUG_PRINT("tuner_module_entry_ioctl command: TUNER_IOCTL_VALSET_RESET");
            copy_ret = copy_from_user( &param,
                                       &( *( int * )uArgument ),
                                       sizeof( int ));
            if( copy_ret != 0 )
            {
                TRACE();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
                /* 排他制御:ロック解�?*/
                mutex_unlock(&g_tuner_mutex);
#endif  
                return -EINVAL;
            }
            ret = tuner_drv_ctl_reset( param );
            
            break;

        case TUNER_IOCTL_VALGET_RESET:
            DEBUG_PRINT("tuner_module_entry_ioctl command: TUNER_IOCTL_VALGET_RESET");
            param = tuner_is_lastexit_exception();
            DEBUG_PRINT("tuner_module_entry_ioctl command: TUNER_IOCTL_VALGET_RESET tag1");

            copy_ret = copy_to_user(&(*(int *)uArgument), &param, sizeof(int));
            if(copy_ret != 0)
            {
                TRACE();
                #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
                mutex_unlock(&g_tuner_mutex);
                #endif  
                
                return -EINVAL;
            }

            break;
            
        default:
            TRACE();
            ret = -EINVAL;
            break;
    }

    /* exclusive control: unlocking */
    mutex_unlock(&g_tuner_mutex);
    INFO_PRINT("tuner_module_entry_ioctl:END\n");
    return ret;
}

/******************************************************************************
 *    function:   tuner_module_entry_poll
 *    brief   :   poll control of a driver
 *    date    :   2011.08.23
 *    author  :   M.Takahashi(*)
 *
 *    return  :    0                   normal exit
 *            :   -1                   error exit
 *    input   :   file
 *            :   poll_tbl
 *    output  :   none
 *    note    :   none
 ******************************************************************************/
static unsigned int tuner_module_entry_poll(
                        struct file *file,
                        struct poll_table_struct *poll_tbl )
{
    unsigned long tuner_flags;
    unsigned int  tuner_mask;


    /* initialization */
    tuner_mask = 0;

    /* wait state transition */
    poll_wait( file, &g_tuner_poll_wait_queue, poll_tbl );

    /* interrupt disabled */
    spin_lock_irqsave( &g_tuner_lock, tuner_flags );

    /* If the wait release conditions */
    if( g_tuner_wakeup_flag == TUNER_ON )
    {
       /* POLLIN    : Can be read without blocking */
       /* POLLRDNORM: Normal data can be read */
        tuner_mask = ( POLLIN | POLLRDNORM );
    }
    
    g_tuner_wakeup_flag = TUNER_OFF;

    /* interrupt enabled */
    spin_unlock_irqrestore( &g_tuner_lock, tuner_flags );

    return tuner_mask;
}

/******************************************************************************
 *    function:   tuner_interrupt
 *    brief   :   interrpu control of a driver
 *    date    :   2011.08.23
 *    author  :   M.Takahashi(*)
 *
 *    return  :    0                   normal exit
 *            :   -1                   error exit
 *    input   :   irq
 *            :   dev_id
 *    output  :   none
 *    note    :   none
 ******************************************************************************/
irqreturn_t tuner_interrupt( int irq, void *dev_id )
{
    DEBUG_PRINT("tuner_interrupt start ");

    /* the processing performed by kernel thread */
    g_tuner_kthread_flag |= TUNER_KTH_IRQHANDLER;
    if( waitqueue_active( &g_tuner_kthread_wait_queue ))
    {
#ifdef TUNER_CONFIG_IRQ_LEVELTRIGGER
        /* if the interrupt level, re-disble interrupts */
        tuner_drv_disable_interrupt();
#endif /* TUNER_CONFIG_IRQ_LEVELTRIGGER */
        wake_up( &g_tuner_kthread_wait_queue );
    }
    else
    {
        DEBUG_PRINT("tuner_interrupt waitqueue_active err!!! \n");
        /* if the wakeup is NG, the kernel thread, to stop the interruption */
        /* failsafe for processing interrupts continue to occur */
        tuner_drv_release_interrupt();
    }

    DEBUG_PRINT("tuner_interrupt end ");

    return IRQ_HANDLED;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Panasonic Co., Ltd.");
MODULE_DESCRIPTION("MM Tuner Driver");

module_init(tuner_drv_start);
module_exit(tuner_drv_end);
/*******************************************************************************
 *              Copyright(c) 2011 Panasonc Co., Ltd.
 ******************************************************************************/
