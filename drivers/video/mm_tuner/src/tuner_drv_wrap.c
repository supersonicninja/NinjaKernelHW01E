/******************************************************************************
 *
 *  file name       : tuner_drv_wrap.c
 *  brief note      : The Wrapper Layer for Tmm Tuner Driver
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
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/mm.h>
#include <asm/uaccess.h>
#include "./../include/tuner_drv.h"
/* Tuner Driver ADD -S. */
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <hsad/config_interface.h>


/******************************************************************************/
/* external function                                                          */
/******************************************************************************/
extern void gpio_func_config_write( unsigned gpio, uint32_t bits );

#define L22_ENABLE
/* Tuner Driver ADD -E. */
/******************************************************************************/
/* function                                                                   */
/******************************************************************************/
int tuner_drv_ctl_power( int data );
int tuner_drv_ctl_reset( int data );
int tuner_is_lastexit_exception(void);
int tuner_drv_set_interrupt( void );
void tuner_drv_release_interrupt( void );
/* Tuner Driver ADD -S. */
static struct regulator *g_reg_l29  = NULL;
static struct regulator *g_reg_lvs2 = NULL;
#ifdef L22_ENABLE 
static struct regulator *g_reg_l22 = NULL;
#endif
int tuner_pm8921_regulator_l29_setup( struct platform_device* pdev );
int tuner_pm8921_regulator_lvs2_setup( struct platform_device* pdev );
int tuner_pm8921_ctl_power_on( void );
int tuner_pm8921_ctl_power_off( void );
void tuner_pm8921_regulator_l29_release( void );
void tuner_pm8921_regulator_lvs2_release( void );
void tuner_pm8921_regulator_release( void );
/* Tuner Driver ADD -E. */

/******************************************************************************
 * code area
 ******************************************************************************/
/******************************************************************************
 *    function:   tuner_drv_ctl_power
 *    brief   :   power control of a driver
 *    date    :   2011.08.26
 *    author  :   M.Takahashi(*)
 *
 *    return  :    0                   normal exit
 *            :   -1                   error exit
 *    input   :   data                 setting data
 *    output  :   none
 *    note    :   none
 ******************************************************************************/
int tuner_drv_ctl_power( int data )
{
/* Tuner Driver ADD -S. */
    int rc = 0;
/* Tuner Driver ADD -E. */
    /* power ON control */
    if(data == TUNER_DRV_CTL_POWON)
    {
/* Tuner Driver ADD -S. */
        rc = tuner_pm8921_ctl_power_on();
/* Tuner Driver ADD -E. */
    }
    /* power OFF control */
    else
    {
/* Tuner Driver ADD -S. */
        rc = tuner_pm8921_ctl_power_off();
/* Tuner Driver ADD -E. */
    }
/* Tuner Driver ADD -S. */
    return rc;
/* Tuner Driver ADD -E. */
}

int tuner_drv_ctl_reset( int data )
{
    int rc = 0;
    int reset_gpio = 0;

    TRACE();

    reset_gpio = get_gpio_num_by_name(TUNER_RESET);

    /* reset: pull up */
    if(data == TUNER_DRV_CTL_RST_PULLUP)
    {
        DEBUG_PRINT("tuner_drv_ctl_reset pullup");

        rc = gpio_request(reset_gpio, TUNER_RESET);
        if(rc)
        {
            gpio_free(reset_gpio);
            rc = gpio_request(reset_gpio, TUNER_RESET);
            if(rc)
            {
                ERROR_PRINT("tuner_drv_ctl_reset gpio request error");
                return rc;
            }
            else
            {
                gpio_direction_output(reset_gpio, TUNER_DRV_CTL_RST_PULLUP);
            }
        }
        else
        {
            gpio_direction_output(reset_gpio, TUNER_DRV_CTL_RST_PULLUP);
        }
        
    }
    /* reset: pull down */
    else if(data == TUNER_DRV_CTL_RST_PULLDOWN)
    {
        DEBUG_PRINT("tuner_drv_ctl_reset pulldown");
        gpio_direction_output(reset_gpio, TUNER_DRV_CTL_RST_PULLDOWN);
        gpio_free(reset_gpio);
    }

    return rc;
}


int tuner_is_lastexit_exception()
{
    int rc = 0;
    int nRet = 0;
    int reset_gpio = 0;
    
    DEBUG_PRINT("tuner_is_lastexit_exception -E\n");

    reset_gpio = get_gpio_num_by_name(TUNER_RESET);

    rc = gpio_request(reset_gpio, TUNER_RESET);
    if(rc)
    {
        DEBUG_PRINT("tuner_is_lastexit_exception: exception detect; rc=%d\n", rc);
        nRet = 1;
    }
    else
    {
        DEBUG_PRINT("tuner_is_lastexit_exception: no exception\n");
        nRet = 0;
        gpio_free(reset_gpio);
    }

    DEBUG_PRINT("tuner_is_lastexit_exception -X; nRet=%d\n", nRet);
    return nRet;
}

/******************************************************************************
 *    function:   tuner_drv_set_interrupt
 *    brief   :   interruption registration control of a driver
 *    date    :   2011.08.26
 *    author  :   M.Takahashi(*)
 *
 *    return  :    0                   normal exit
 *            :   -1                   error exit
 *    input   :   pdev
 *    output  :   none
 *    note    :   none
 ******************************************************************************/
int tuner_drv_set_interrupt( void )
{
    int ret;                                     /* return value              */

    /* interrupt register */
    ret = request_irq( TUNER_CONFIG_INT,         /* IRQ number                */
                       tuner_interrupt,          /* callbacks                 */
                       IRQF_DISABLED,            /* set interrupt disable function running */
                       "mm_tuner",               /* display name              */
                       NULL );                   /* no device ID              */

    /* failure to register */
    if( ret != 0 )
    {
        return -1;
    }
    /* normal exit */
    return 0;
}

/******************************************************************************
 *    function:   tuner_drv_release_interrupt
 *    brief   :   interruption registration release control of a driver
 *    date    :   2011.08.26
 *    author  :   M.Takahashi(*)
 *
 *    return  :   none
 *    input   :   none
 *    output  :   none
 *    note    :   none
 ******************************************************************************/
void tuner_drv_release_interrupt( void )
{
    /* clear interrupt register */
    free_irq( TUNER_CONFIG_INT, NULL );
}

#ifdef TUNER_CONFIG_IRQ_LEVELTRIGGER
/******************************************************************************
 *    function:   tuner_drv_enable_interrupt
 *    brief   :   interruption registration enable control of a driver
 *    date    :   2011.09.18
 *    author  :   M.Takahashi(*)(*)
 *
 *    return  :   none
 *    input   :   none
 *    output  :   none
 *    note    :   none
 ******************************************************************************/
void tuner_drv_enable_interrupt( void )
{
    /* set interrupt enable */
#if 0 /* Tuner Driver MOD -S. */
    enable_irq( TUNER_INT, NULL );
#else
    enable_irq( TUNER_CONFIG_INT );
#endif /* Tuner Driver MOD -E. */
}

/******************************************************************************
 *    function:   tuner_drv_disable_interrupt
 *    brief   :   interruption registration disable control of a driver
 *    date    :   2011.09.18
 *    author  :   M.Takahashi(*)(*)
 *
 *    return  :   none
 *    input   :   none
 *    output  :   none
 *    note    :   none
 ******************************************************************************/
void tuner_drv_disable_interrupt( void )
{
    /* set interrupt disable */
#if 0 /* Tuner Driver MOD -S. */
    disable_irq( TUNER_INT, NULL );
#else
    disable_irq( TUNER_CONFIG_INT );
#endif /* Tuner Driver MOD -E. */
}
#endif /* TUNER_CONFIG_IRQ_LEVELTRIGGER */

/* Tuner Driver ADD -S. */
/******************************************************************************
 *    function:   tuner_pm8921_regulator_l29_setup
 *    brief   :   setup PM8921_L29 regulator
 *    date    :   2012.01.16
 *    author  :   fsi
 *
 *    return  :   int
 *    input   :   pdev
 *    output  :   none
 *    note    :   none
 ******************************************************************************/
int tuner_pm8921_regulator_l29_setup( struct platform_device* pdev )
{
    const char* reg_name = "tuner_L29";
    int rc = 0;

    /* param check. */
    if(pdev == NULL)
    {
        ERROR_PRINT("[%d][%s] bad param.\n", __LINE__, __func__);
        return -1;
    }

    /* If not acquired regulator */
    if(g_reg_l29 == NULL)
    {
        /* get regulator. */
        g_reg_l29 = regulator_get(&pdev->dev, reg_name);
        /* acquisition failure */
        if(IS_ERR(g_reg_l29))
        {
            rc = PTR_ERR(g_reg_l29);
            ERROR_PRINT("[%d][%s] L29 regulator_get error: %d\n", __LINE__, __func__, rc);
            return rc;
        }

        /* set voltage Low. */
        rc = regulator_set_voltage(g_reg_l29, 0, 1800000);
        /* configuration fails */
        if(rc < 0)
        {
            rc = PTR_ERR(g_reg_l29);
            ERROR_PRINT("[%d][%s] L29 regulator_set_voltage error: %d\n", __LINE__, __func__, rc);
            tuner_pm8921_regulator_l29_release();
            return rc;
        }
    }

#ifdef L22_ENABLE
    if(g_reg_l22 == NULL)
    {
        /* get regulator. */
        g_reg_l22 = regulator_get(&pdev->dev, "tuner_antenna_l22");
        if(IS_ERR(g_reg_l22))
        {
            rc = PTR_ERR(g_reg_l22);
            ERROR_PRINT("[%d][%s] regulator_get error: %d\n", __LINE__, __func__, rc);
            return rc;
        }

        /* set voltage Low. */
        rc = regulator_set_voltage(g_reg_l22, 0, 2750000);
        if(rc < 0)
        {
            rc = PTR_ERR(g_reg_l22);
            ERROR_PRINT("[%d][%s] regulator_set_voltage error: %d\n", __LINE__, __func__, rc);
            return rc;
        }
    }
#endif

    /* normal exit */
    return 0;
}

/******************************************************************************
 *    function:   tuner_pm8921_regulator_lvs2_setup
 *    brief   :   setup PM8921_LVS2 regulator
 *    date    :   2012.01.16
 *    author  :   fsi
 *
 *    return  :   int
 *    input   :   pdev
 *    output  :   none
 *    note    :   none
 ******************************************************************************/
int tuner_pm8921_regulator_lvs2_setup( struct platform_device* pdev )
{
    const char* reg_name = "tuner_LVS2";
    int rc = 0;

    /* param check. */
    if(pdev == NULL)
    {
        ERROR_PRINT("[%d][%s] bad param.\n", __LINE__, __func__);
        tuner_pm8921_regulator_l29_release();
        return -1;
    }

    /* If not acquired regulator */
    if(g_reg_lvs2 == NULL)
    {
        /* get regulator. */
        g_reg_lvs2 = regulator_get(&pdev->dev, reg_name);
        /* acquisition failure */
        if(IS_ERR(g_reg_lvs2))
        {
            rc = PTR_ERR(g_reg_lvs2);
            ERROR_PRINT("[%d][%s] LVS2 regulator_get error: %d\n", __LINE__, __func__, rc);
            tuner_pm8921_regulator_release();
            return rc;
        }
    }
    /* normal exit */
    return 0;
}

/******************************************************************************
 *    function:   tuner_pm8921_ctl_power_on
 *    brief   :   set voltage High ,and enable PM8921_L29 and PM8921_LVS2
 *    date    :   2012.01.16
 *    author  :   fsi
 *
 *    return  :   int
 *    input   :   pdev
 *    output  :   none
 *    note    :   none
 ******************************************************************************/
int tuner_pm8921_ctl_power_on( void )
{
    int rc = 0;

    DEBUG_PRINT("tuner_pm8921_ctl_power_on -E");

    /* <L29> set voltage High. */
    rc = regulator_set_voltage( g_reg_l29, 1800000, 1800000 );
    if( rc < 0 )
    {
        rc = PTR_ERR( g_reg_l29 );
        ERROR_PRINT( "[%d][%s] regulator_set_voltage error: %d\n", __LINE__, __func__, rc );
        tuner_pm8921_regulator_l29_release();
        return rc;
    }
    
    /* <L29> set enable. */
    rc = regulator_enable( g_reg_l29 );
    if( rc < 0 )
    {
        rc = PTR_ERR( g_reg_l29 );
        ERROR_PRINT( "[%d][%s] regulator_enable error: %d\n", __LINE__, __func__, rc );
        tuner_pm8921_regulator_release();
        return rc;
    }

    /* <LVS2> set enable. */
    rc = regulator_enable( g_reg_lvs2 );
    if( rc < 0 )
    {
        rc = PTR_ERR( g_reg_lvs2 );
        ERROR_PRINT( "[%d][%s] regulator_enable error: %d\n", __LINE__, __func__, rc );
        tuner_pm8921_regulator_release();
        return rc;
    }

#ifdef L22_ENABLE
    rc = regulator_set_voltage(g_reg_l22, 2750000, 2750000);
    if(rc < 0)
    {
        rc = PTR_ERR(g_reg_l22);
        ERROR_PRINT("[%d][%s] regulator_set_voltage error: %d\n", __LINE__, __func__, rc);
        return rc;
    }

    /* <L22> set enable. */
    if(regulator_enable(g_reg_l22))
    {
        rc = PTR_ERR(g_reg_l22);
        ERROR_PRINT("[%d][%s] regulator_enable error: %d\n", __LINE__, __func__, rc);
        return rc;
    }
#endif

    DEBUG_PRINT("tuner_pm8921_ctl_power_on -X");
    return rc;
}

/******************************************************************************
 *    function:   tuner_pm8921_ctl_power_off
 *    brief   :   set voltage Low ,and disable PM8921_L29 and PM8921_LVS2
 *    date    :   2012.01.16
 *    author  :   fsi
 *
 *    return  :   int
 *    input   :   pdev
 *    output  :   none
 *    note    :   none
 ******************************************************************************/
int tuner_pm8921_ctl_power_off( void )
{
    int rc = 0;

    DEBUG_PRINT("tuner_pm8921_ctl_power_off -E\n");

    /* <L29> set voltage Low. */
    if( rc < 0 )
    {
        rc = PTR_ERR( g_reg_l29 );
        ERROR_PRINT( "[%d][%s] regulator_set_voltage error: %d\n", __LINE__, __func__, rc );
        tuner_pm8921_regulator_release();
        return rc;
    }

    /* <L29> set disable. */
    rc = regulator_disable( g_reg_l29 );
    if( rc < 0 )
    {
        rc = PTR_ERR( g_reg_l29 );
        ERROR_PRINT( "[%d][%s] regulator_disable error: %d\n", __LINE__, __func__, rc );
        tuner_pm8921_regulator_release();
        return rc;
    }

    /* <LVS2> set disable. */
    rc = regulator_disable( g_reg_lvs2 );
    if( rc < 0 )
    {
        rc = PTR_ERR( g_reg_lvs2 );
        ERROR_PRINT( "[%d][%s] regulator_disable error: %d\n", __LINE__, __func__, rc );
        tuner_pm8921_regulator_release();
        return rc;
    }

#ifdef L22_ENABLE
    /* <L29> set voltage Low. */
    rc = regulator_set_voltage(g_reg_l22, 0, 2750000);
    if(rc < 0)
    {
        rc = PTR_ERR(g_reg_l22);
        ERROR_PRINT("[%d][%s] regulator_set_voltage error: %d\n", __LINE__, __func__, rc);
        return rc;
    }

    /* <L22> set disable. */
    if(regulator_disable(g_reg_l22))
    {
        rc = PTR_ERR(g_reg_l22);
        ERROR_PRINT("[%d][%s] regulator_disable error: %d\n", __LINE__, __func__, rc);
        return rc;
    }
#endif

    DEBUG_PRINT("tuner_pm8921_ctl_power_off -X\n");
    return rc;
}

/******************************************************************************
 *    function:   tuner_pm8921_regulator_L29_release
 *    brief   :   release PM8921_L29 regulator
 *    date    :   2012.01.16
 *    author  :   fsi
 *
 *    return  :   none
 *    input   :   none
 *    output  :   none
 *    note    :   none
 ******************************************************************************/
void tuner_pm8921_regulator_l29_release( void )
{
    /* regulator release. */
    regulator_put(g_reg_l29);
    /* global value init. */
    g_reg_l29 = NULL;

#ifdef L22_ENABLE
    /* regulator release. */
    regulator_put(g_reg_l22);
    /* global value init. */
    g_reg_l22 = NULL;
#endif
}

/******************************************************************************
 *    function:   tuner_pm8921_regulator_lvs2_release
 *    brief   :   release PM8921_LVS2 regulator
 *    date    :   2012.01.16
 *    author  :   fsi
 *
 *    return  :   none
 *    input   :   none
 *    output  :   none
 *    note    :   none
 ******************************************************************************/
void tuner_pm8921_regulator_lvs2_release( void )
{
    /* regulator release. */
    regulator_put(g_reg_lvs2);
    /* global value init. */
    g_reg_lvs2 = NULL;
}

/******************************************************************************
 *    function:   tuner_pm8921_regulator_release
 *    brief   :   release PM8921_L29 and PM8921_LVS2 regulator
 *    date    :   2012.01.16
 *    author  :   fsi
 *
 *    return  :   none
 *    input   :   none
 *    output  :   none
 *    note    :   none
 ******************************************************************************/
void tuner_pm8921_regulator_release( void )
{
    /* release L29 regulator */
    tuner_pm8921_regulator_l29_release();
    /* release LVS2 regulator */
    tuner_pm8921_regulator_lvs2_release();
}
/* Tuner Driver ADD -E. */

/*******************************************************************************
 *              Copyright(c) 2011 Panasonc Co., Ltd.
 ******************************************************************************/
