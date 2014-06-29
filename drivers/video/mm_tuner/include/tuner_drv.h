/******************************************************************************
 *
 *  file name       : tuner_drv.h
 *  brief note      : Driver Common Header
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
 * HISTORY      : 2011/08/25    K.Kitamura(*)
 *                001 new creation
 ******************************************************************************/
/*..+....1....+....2....+....3....+....4....+....5....+....6....+....7....+...*/
#ifndef _TUNER_DRV_H
#define _TUNER_DRV_H

#include <linux/interrupt.h>
#include "../include_share/tuner_drv_config.h"
#include "../include_share/tuner_drv_sys.h"

 
#define IOCTL_VALGET _IOW(TUNER_IOC_MAGIC, 1, struct ioctl_cmd)
#define IOCTL_VALSET _IOR(TUNER_IOC_MAGIC, 2, struct ioctl_cmd)

/* general settings */
#define TUNER_OFF                     0x00       /* Setting OFF               */
#define TUNER_ON                      0x01       /* Setting ON                */

/* enable bit number */
#define TUNER_CHARNUM_MAX             0x08       /* enable bit number of char-type */

/* slave address */
#define TUNER_SLAVE_ADR_M1            0x64       /* Main#1 full-seg           */
#define TUNER_SLAVE_ADR_M2            0x60       /* Main#2 1-seg              */
#define TUNER_SLAVE_ADR_S             0x62       /* Sub Tuner control         */

/* register address */
/* Main#1 */
#define TUNER_ADR_INTDEF1_F           0xD3
#define TUNER_ADR_INTDEF2_F           0xD4
/* Main#2 */
#define TUNER_ADR_INTDEF_S            0xC0

/* INTDEF1_F */
#define SIG_ENAS_INTDEF1_F             (0)       /* INTDEF1_F start bit       */
#define SIG_ENAE_INTDEF1_F             (7)       /* INTDEF1_F end bit         */
#define SIG_ENA_INTDEF1_F           (0xFF)       /* INTDEF1_F enable bit      */
#define SIG_ENAIRQ_INTDEF1_NONE     (0x00)       /* INTDEF1_F irq_enable_none */
/* INTDEF2_F */
#define SIG_ENAS_INTDEF2_F             (0)       /* INTDEF2_F start bit       */
#define SIG_ENAE_INTDEF2_F             (3)       /* INTDEF2_F end bit         */
#define SIG_ENA_INTDEF2_F           (0x0F)       /* INTDEF2_F enable bit      */
#define SIG_ENAIRQ_INTDEF2_NONE     (0x00)       /* INTDEF2_F irq_enable_none */

/* INTDEF_S */
#define SIG_ENAS_IERR_S                (0)
#define SIG_ENAE_SEGERRS               (7)
#define SIG_ENA_SEGERRS             (0x80)
#define SIG_ENA_MGERR_S             (0x40)
#define SIG_ENA_SSEQ_S              (0x20)
#define SIG_ENA_CDFLG_S             (0x10)
#define SIG_ENA_ISYNC_S             (0x08)
#define SIG_ENA_TMCCERR_S           (0x04)
#define SIG_ENA_EMGFLG_S            (0x02)
#define SIG_ENA_IERR_S              (0x01)
#define SIG_ENAIRQ_INTDEF_NONE      (0x00)

/* interrupt register */
#define TUNER_DRV_ADR_INTCND_F        0xDA
#define TUNER_DRV_ADR_INTCND_S        0xC3
#define TUNER_DRV_ENAS_ALL               0
#define TUNER_DRV_ENAE_ALL               7
#define TUNER_DRV_ENA_ALL             0xFF
#define TUNER_DRV_PARAM_RINIT         0x00
#define TUNER_DRV_PARAM_ALL           0xFF

/* number of transmit data */
#define TUNER_R_MSGLEN                0x01       /* length of read data       */
#define TUNER_W_MSGLEN                0x02       /* length of write data      */
#define TUNER_R_MSGNUM                0x02       /* length of read data       */
#define TUNER_W_MSGNUM                0x01       /* length of write data      */

/* write-once settings */
#define TUNER_I2C_WRITE_ALL_NUM     (1000)       /* write-once implementation threshold data */
#define TUNER_I2C_MSG_DATA_NUM         (2)       /* the number of data tables setting I2C    */

/* number of tuner event register */
#define TUNER_EVENT_REGNUM             (3)       /* number of tuner event register */

/* event flag kernel thread */
#define TUNER_KTH_NONE          0x00000000       /* the initial value flag kernel thread */
#define TUNER_KTH_IRQHANDLER    0x00000001       /* handlers interrupt flag              */
#define TUNER_KTH_END           0x80000000       /* kernel thread termination flag       */

/* kernel thread end flag */
#define TUNER_KTH_ENDFLG_OFF          0x00       /* kernel thread running     */
#define TUNER_KTH_ENDFLG_ON           0x01       /* kernel thread terminating */

#define TUNER_RESET    "ISDB_RST" /* reset gpio */

/* DEBUG */
/* #define TUNER_DRV_DLOG */
#ifdef  TUNER_DRV_DLOG
#define ERROR_PRINT( fmt, arg... )  { printk(KERN_ERR  "%s: " fmt "\n", "mmtuner driver", ##arg); }
#define INFO_PRINT( fmt, arg... )   { printk(KERN_INFO "%s: " fmt "\n", "mmtuner driver", ##arg); }
#define DEBUG_PRINT( fmt, arg... )  { printk(KERN_INFO "%s: " fmt "\n", "mmtuner driver", ##arg); }
#define TRACE() DEBUG_PRINT( "%s( %d )", __FUNCTION__, __LINE__ )
#else
#define ERROR_PRINT( fmt, arg... )
#define INFO_PRINT( fmt, arg... )  
#define DEBUG_PRINT( fmt, arg... )
#define TRACE() DEBUG_PRINT()
#endif

extern int tuner_drv_hw_access( unsigned int uCommand,
                                TUNER_DATA_RW *data,
                                unsigned short param_len );
extern int tuner_drv_ctl_power( int data );
extern int tuner_drv_ctl_reset( int data );
extern int tuner_is_lastexit_exception(void);
extern int tuner_drv_set_interrupt( void );
extern void tuner_drv_release_interrupt( void );
extern void tuner_drv_enable_interrupt( void );
extern void tuner_drv_disable_interrupt( void );
extern irqreturn_t tuner_interrupt( int irq, void *dev_id );
/* Tuner Driver ADD -S. */
extern int tuner_pm8921_regulator_l29_setup( struct platform_device* pdev );
extern int tuner_pm8921_regulator_lvs2_setup( struct platform_device* pdev );
extern void tuner_pm8921_regulator_release( void );
/* Tuner Driver ADD -E. */

#endif /* _TUNER_DRV_H */
/*******************************************************************************
 *              Copyright(c) 2011 Panasonc Co., Ltd.
 ******************************************************************************/
