/*
* USBSWITCH_CHRDEV Header File <driver>
*
* Copyright (C) 2012 Huawei Technology Inc. 
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
#ifndef __USBSWITCH_CHRDEV__
#define __USBSWITCH_CHRDEV__

/*========================================
* header file include
*========================================*/
#include <linux/kernel.h>
#include <linux/ioctl.h>

/*========================================
* macro definition
*========================================*/
/* #define USBSWITCH_CHRDEV_DLOG */
#ifdef  USBSWITCH_CHRDEV_DLOG
#define ERROR_PRINT( fmt, arg... )  { printk(KERN_ERR  "%s: " fmt "\n", "uswitch_chrdev", ##arg); }
#define INFO_PRINT( fmt, arg... )   { printk(KERN_INFO "%s: " fmt "\n", "uswitch_chrdev", ##arg); }
#define TRACE() INFO_PRINT( "%s( %d )", __FUNCTION__, __LINE__ )
#else
#define ERROR_PRINT( fmt, arg... )
#define INFO_PRINT( fmt, arg... )  
#define TRACE() INFO_PRINT()
#endif

#define USWITCH_IOCTL_MAGIC 'U'
#define USWITCH_IOCTL_GET_CRADLE_STATUS _IOW(USWITCH_IOCTL_MAGIC, 1, int)

#define CRADLE_DETECT_GPIO "CRADLE_ANT_DET"

/*========================================
* function declaration
*========================================*/
int usbswitch_is_cradle_attached(void);

#endif