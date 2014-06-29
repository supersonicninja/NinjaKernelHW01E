/*
 * LCD Tuning ioctl macros
 *
 */

#ifndef _LINUX_LCD_IOCTL_H
#define _LINUX_LCD_IOCTL_H

#include <linux/ioctl.h>
#include <linux/types.h>


#define LCD_IOR(num, dtype)	_IOR('L', num, dtype)

#define LCD_TUNING_DGAMMA	LCD_IOR(30, unsigned int)
/*reason: modified for ACL fouction */
#define LCD_TUNING_ACL 	LCD_IOR(31, unsigned int)
#define LCD_TUNING_CABC LCD_IOR(32, unsigned int)
#endif
