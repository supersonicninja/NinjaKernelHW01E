#ifndef __USBSWITCH_TSU6712__
#define __USBSWITCH_TSU6712__
#include "usbswitch.h"

#define TSU_DEVICEID				0x1
#define TSU_CONTROL           			0x2        //the address of control register
#define TSU_INT1               			0x3        // the address of interupt 1 register  
#define TSU_INT2               			0x4        // the address of interupt 2 register
#define TSU_ADC                			0x7        // the address of ADC register
#define TSU_DEVICE_TYPE1			0xA		// the address of device type1 register
#define TSU_DEVICE_TYPE2			0xB		// the address of device type2 register
#define TSU_CARKIT_INT1_MASK		0xE		// the address of carkit int1 mask register
#define TSU_CARKIT_INT1			0xF		// the address of carkit int1  register
#define TSU_CARKIT_INT2			0x10	// the address of carkit int2  register
#define TSU_CARKIT_INT2_MASK		0x12	// the address of carkit int2 mask register
#define TSU_MSW1               			0x13      // the address of manual sw 1 register
#define TSU_VBUS             			0x15      // the address of vbus register

#define TSU_CONTROL_INIT_VAL		0x1A	//the init config value of control register

#define TSU_MASK_MANUAL        		0xFB	// the mask of manual manual sw
#define TSU_MASK_INT           			0xFE	// the mask of INT
#define TSU_MASK_PRESS         		0x0 		// the mask  of  press botton 
#define TSU_MASK_RAW           		0xF7       //the mask of RAW data
#define TSU_MASK_ADCCHANGE     		0x4        // the flage of  ADC change
#define TSU_MASK_ATTACH        		0x1        // the flage of accessory attach
#define TSU_MASK_DETACH        		0x2        // the flage  of accessory dettach
#define TSU_MASK_VBUS            		0x2        //the flage of vbus high
#define TSU_MASK_AVCABLE      		0x1        // the flage of a/v cable
#define TSU_MASK_DISABLERAW 		0x8        // the flage of disable RAW
#define TSU_MASK_ENABLERAW   		0xF7      // the flage of enable RAW
#define TSU_MASK_CARKIT_INT		0xFF	//the mask of disable all carkit int

#define TSU_MSW1_USB          			0x91       // the writing data of usb accessory in TSU IC
#define TSU_MSW1_HEADPHONE 		0x4a       // the writing data of usb headphone

#define TSU_ADC_USBCHARGE_OR_DATA  	0x1f       // the ADC data of usb accessory
#define TSU_ADC_USBHEADPHONE1      		0x18       // the ADC data of double channel earphone
#define TSU_ADC_USBHEADPHONE2      		0x19       // the ADC data of double channel earphone
#define TSU_ADC_USBHEADPHONE3      		0X1e       // the ADC data of single channel earphone
#define TSU_ADC_MHL_OR_CRADLE1      		0x00       // the ADC data of MHL or cradle
#define TSU_ADC_MHL_OR_CRADLE2      		0x01 	// the ADC data of MHL or cradle
#define TSU_ADC_PRESS_UNVAILD                	0x1f      // the ADC data of headphone press unvaild

enum tsu_device_type
{
	TSU_DEVICE_NULL 	= 0x0,
	TSU_DEVICE_USB 	= 0x4,
};

#endif
