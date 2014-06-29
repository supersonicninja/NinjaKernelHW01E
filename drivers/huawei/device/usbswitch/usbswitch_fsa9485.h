#ifndef __USBSWITCH_FSA9485__
#define __USBSWITCH_FSA9485__
#include "usbswitch.h"


#define FSA_DEVICEID					0x1		//the address of device id register
#define FSA_CONTROL           				0x2        //the address of control register
#define FSA_INT1               				0x3        // the address of interupt 1 register  
#define FSA_INT2               				0x4        // the address of interupt 2 register
#define FSA_ADC                				0x7        // the address of ADC register
#define FSA_MSW1               				0x13      // the address of manual sw 1 register
#define FSA_RESET                                      0x1b      // the address of reset register
#define FSA_VBUS             				0x1d      // the address of vbus register
#define FSA_OVP_FUSE            			0x21      //the address of ovp fuse register[this is a reserved reg]

#define FSA_CONTROL_INIT_VAL			0x1A	//the init config value of control register

#define FSA_MASK_MANUAL        			0xFB	// the mask of manual manual sw
#define FSA_MASK_INT           				0xFE	// the mask of INT
#define FSA_MASK_PRESS         			0x0 		// the mask  of  press botton 
#define FSA_MASK_RAW           			0xF7       //the mask of RAW data
#define FSA_MASK_ADCCHANGE     			0x4        // the flage of  ADC change
#define FSA_MASK_ATTACH        			0x1        // the flage of accessory attach
#define FSA_MASK_DETACH        			0x2        // the flage  of accessory dettach
#define FSA_MASK_VBUS            			0x2        //the flage of vbus high
#define FSA_MASK_AVCABLE      			0x1        // the flage of a/v cable
#define FSA_MASK_DISABLERAW 			0x8        // the flage of disable RAW
#define FSA_MASK_ENABLERAW   			0xF7      // the flage of enable RAW

#define FSA_MSW1_USB          				0x93       // the writing data of usb accessory in FSA IC
#define FSA_MSW1_HEADPHONE 			0x4a       // the writing data of usb headphone
#define FSA_RESET_DATA                            0x01       // the writing data of RESET

#define FSA_ADC_USBCHARGE_OR_DATA  	0x1f       // the ADC data of usb accessory
#define FSA_ADC_USBHEADPHONE1      		0x18       // the ADC data of double channel earphone
#define FSA_ADC_USBHEADPHONE2      		0x19       // the ADC data of double channel earphone
#define FSA_ADC_USBHEADPHONE3      		0X1e       // the ADC data of single channel earphone
#define FSA_ADC_MHL_OR_CRADLE1      	0x00       // the ADC data of MHL or cradle
#define FSA_ADC_MHL_OR_CRADLE2      	0x01 	// the ADC data of MHL or cradle
#define FSA_ADC_PRESS_UNVAILD               0x1f      // the ADC data of headphone press unvaild

#define FSA_OVP_6_4						0x08	//the OVP value of 6.4V
#define FSA_OVP_6_8						0x18	//the OVP value of 6.8V
#define FSA_OVP_7_2						0x28	//the OVP value of 7.2V
#define FSA_OVP_7_6						0x38	//the OVP value of 7.6V

#define FSA_CLR_ALL_BITS_MASK			0xff	//the mask of clear all bits

#endif
