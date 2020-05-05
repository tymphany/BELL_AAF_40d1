/*
* This file contains all the necessary settings for the IQS263 and this file can
* be changed from the GUI or edited here
* File:   iqs263_init.h
* Author: Azoteq
*/

#ifndef _IQS263_INIT_H_
#define _IQS263_INIT_H_

/* Used to switch Projected mode & set Global Filter Halt (0x01 Byte1) */
#define SYSTEM_FLAGS_VAL					0x00

/* Enable / Disable system events (0x01 Byte2)*/
#define SYSTEM_EVENTS_VAL					0x00

/*
* Change the Multipliers & Base values (0x07 in this order)
* Please note that these values are affected by the Auto ATI routine and should
* only be written in the case of a specific setup.  Alternatively Auto ATI
* should be manually called after writing these settings.
* This is applicable for both Multipliers and Compensation.
*/
#define MULTIPLIERS_CH0						0x1C
#define MULTIPLIERS_CH1						0x24
#define MULTIPLIERS_CH2						0x1B
#define MULTIPLIERS_CH3						0x00
#define BASE_VAL1							0x81
#define BASE_VAL2							0x88

/*
* Change the Compensation for each channel (0x08 in this order)
* Please note that these values are affected by the Auto ATI routine and should
* only be written in the case of a specific setup.  Alternatively Auto ATI
* should be manually called after writing these settings.
* This is applicable for both Multipliers and Compensation.
*/
#define COMPENSATION_CH0					0xA6
#define COMPENSATION_CH1					0x35
#define COMPENSATION_CH2					0x38
#define COMPENSATION_CH3					0x00

/* Change the Prox Settings or setup of the IQS263 (0x09 in this order) */
#define PROXSETTINGS0_VAL					0x02
#define PROXSETTINGS1_VAL					0x15
#define PROXSETTINGS2_VAL					0x00
#define PROXSETTINGS3_VAL					0x05
#define EVENT_MASK_VAL						0xE0

/* Change the Thresholds for each channel (0x0A in this order) */
#define PROX_THRESHOLD						0x0A
#define TOUCH_THRESHOLD_CH1					0x0F
#define TOUCH_THRESHOLD_CH2					0x0F
#define TOUCH_THRESHOLD_CH3					0x0F
#define MOVEMENT_THRESHOLD					0x03
#define RESEED_BLOCK						0x00
#define HALT_TIME							0xD8 /* 22 => 12 sec , 0xD8 -> 219 = 120 second */
#define I2C_TIMEOUT							0x50

/* Change the Timing settings (0x0B in this order) */
#define LOW_POWER							0x00
#define ATI_TARGET_TOUCH					0x64
#define ATI_TARGET_PROX						0x50
#define TAP_TIMER							0x10//0x0A , vendor suggest,20190904
#define FLICK_TIMER							0x3C
#define FLICK_THRESHOLD						0x48

/* Set Active Channels (0x0D) */
#define ACTIVE_CHS							0x07

#endif	/* _IQS263_INIT_H_ */
