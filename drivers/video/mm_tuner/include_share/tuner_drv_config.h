/******************************************************************************
 *
 *  file name       : tuner_drv_config.h
 *  brief note      : Driver Config Header
 *
 *  creation data   : 2011.08.28
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

#ifndef _TUNER_DRV_CONFIG_H
#define _TUNER_DRV_CONFIG_H
/* Tuner Driver ADD -S. */
#include <mach/irqs.h>
/* Tuner Driver ADD -E. */
/******************************************************************************
 * data
 ******************************************************************************/
#define TUNER_SET_ON                     1       /* to enable                 */
#define TUNER_SET_OFF                    0       /* to disable                */

/* driver name */
#if 0 /* Tuner Driver MOD -S. */
#define TUNER_CONFIG_DRIVER_NAME     "mmtuner_drv"
#else
#define TUNER_CONFIG_DRIVER_NAME     "mmtuner"
#endif /* Tuner Driver MOD -E. */

/* configuring the device files */
#if 0 /* Tuner Driver MOD -S. */
#define TUNER_CONFIG_DRV_MAJOR         240       /* MAJOR Number             */
#else /* Tuner Driver MOD. */
#define TUNER_CONFIG_DRV_MAJOR         233       /* MAJOR Number.            */
#endif /* Tuner Driver MOD -E. */
#define TUNER_CONFIG_DRV_MINOR         200       /* MINOR Number.            */

/* interrupt number */
#if 0 /* Tuner Driver MOD -S. */
#define TUNER_CONFIG_INT              0x07       /* IRQNumber                */
#else /* Tuner Driver MOD. */
#define TUNER_CONFIG_INT              MSM_GPIO_TO_INT(92)    /* IRQ Number(288+92). */
#endif /* Tuner Driver MOD -E. */

/* I2C bus number */
#if 0 /* Tuner Driver MOD -S. */
#define TUNER_CONFIG_I2C_BUSNUM       0x05       /* I2C Bus Line.             */
#else /* Tuner Driver MOD. */
#define TUNER_CONFIG_I2C_BUSNUM       0x09       /* I2C Bus Line.             */
#endif /* Tuner Driver MOD -E. */

/* priority kernel thread  */
#define TUNER_CONFIG_KTH_PRI            95       /* exclusive control is enabled            */

/* exclusive control of the driver */
/* #define TUNER_CONFIG_DRV_MULTI              *//* exclusive control is enabled */

/* interrupt trigger configuration */
#define TUNER_CONFIG_IRQ_LEVEL  TUNER_SET_ON     /* Interrupt trigger configuration = Level */

/* Tuner Driver ADD -S. */
/* compile switch for level sence */
#define TUNER_CONFIG_IRQ_LEVELTRIGGER
/* Tuner Driver ADD -E. */

#endif/* _TUNER_DRV_CONFIG_H */
/*******************************************************************************
 *              Copyright(c) 2011 Panasonc Co., Ltd.
 ******************************************************************************/
