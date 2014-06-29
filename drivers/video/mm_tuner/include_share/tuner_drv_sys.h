/******************************************************************************
 *
 *  file name       : tuner_drv_sys.h
 *  brief note      : The Header for Driver Public Presentation
 *
 *  creation data   : 2011.08.01
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
#ifndef _TUNER_DRV_SYS_H
#define _TUNER_DRV_SYS_H

/******************************************************************************
 * define
 ******************************************************************************/
/* configuration parameters for the IOCTL */
#define TUNER_IOC_MAGIC 'd'
#define TUNER_IOCTL_VALGET        _IOW(TUNER_IOC_MAGIC, 1, struct ioctl_cmd)
#define TUNER_IOCTL_VALSET        _IOR(TUNER_IOC_MAGIC, 2, struct ioctl_cmd)
#define TUNER_IOCTL_VALGET_EVENT  _IOR(TUNER_IOC_MAGIC, 3, struct ioctl_cmd)
#define TUNER_IOCTL_VALSET_POWER  _IOR(TUNER_IOC_MAGIC, 4, struct ioctl_cmd)
#define TUNER_IOCTL_VALSET_EVENT  _IOW(TUNER_IOC_MAGIC, 5, struct ioctl_cmd)
#define TUNER_IOCTL_VALREL_EVENT  _IOW(TUNER_IOC_MAGIC, 6, struct ioctl_cmd)
#define TUNER_IOCTL_VALSET_RESET  _IOW(TUNER_IOC_MAGIC, 7, struct ioctl_cmd)
#define TUNER_IOCTL_VALGET_RESET  _IOW(TUNER_IOC_MAGIC, 8, struct ioctl_cmd)

/* parameters for power control */
#define TUNER_DRV_CTL_POWON              0       /* power ON                  */
#define TUNER_DRV_CTL_POWOFF             1       /* power OFF                 */

#define TUNER_DRV_CTL_RST_PULLDOWN        0       /* reset: pull down */
#define TUNER_DRV_CTL_RST_PULLUP             1       /* reset: pull up */

/* enabit enable setting */
#define TUNER_SET_ENADATA             0xFF       /* to enable enabit          */

/******************************************************************************
 * data
 ******************************************************************************/
/* Structure for register read/write */
typedef struct _tuner_data_rw {
    unsigned short slave_adr;                     /* Slave address           */
    unsigned short adr;                           /* Register address        */
    unsigned short sbit;                          /* Start bit               */
    unsigned short ebit;                          /* End bit                 */
    unsigned short param;                         /* Write/Read value hold   */
    unsigned short enabit;                        /* Enable bit              */
} TUNER_DATA_RW ;


/* IOCTL data structure for the device */
struct ioctl_cmd {
    unsigned int reg;                            /* register value            */
    unsigned int offset;                         /* offset                    */
    unsigned int val;                            /* parameter                 */
};

#endif/* _TUNER_DRV_SYS_H */
/*******************************************************************************
 *              Copyright(c) 2011 Panasonc Co., Ltd.
 ******************************************************************************/
