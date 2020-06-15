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
#define MULTIPLIERS_CH0						0x00
#define MULTIPLIERS_CH1						0x00
#define MULTIPLIERS_CH2						0x00
#define MULTIPLIERS_CH3						0x00
#define BASE_VAL1							0x33
#define BASE_VAL2							0x33

/*
* Change the Compensation for each channel (0x08 in this order)
* Please note that these values are affected by the Auto ATI routine and should
* only be written in the case of a specific setup.  Alternatively Auto ATI
* should be manually called after writing these settings.
* This is applicable for both Multipliers and Compensation.
*/
#define COMPENSATION_CH0					0x00
#define COMPENSATION_CH1					0x00
#define COMPENSATION_CH2					0x00
#define COMPENSATION_CH3					0x00

/* Change the Prox Settings or setup of the IQS263 (0x09 in this order) */
#define PROXSETTINGS0_VAL					0x22
#define PROXSETTINGS1_VAL					0x2D
#define PROXSETTINGS2_VAL					0x10
#define PROXSETTINGS3_VAL					0x07
#define EVENT_MASK_VAL						0xE0

/* Change the Thresholds for each channel (0x0A in this order) */
#define PROX_THRESHOLD						0xFF
#define TOUCH_THRESHOLD_CH1					0x04
#define TOUCH_THRESHOLD_CH2					0x04
#define TOUCH_THRESHOLD_CH3					0x04
#define MOVEMENT_THRESHOLD					0x03
#define RESEED_BLOCK						0x00
#define HALT_TIME							0xD8
#define I2C_TIMEOUT							0x50

/* Change the Timing settings (0x0B in this order) */
#define LOW_POWER							0x00
#define ATI_TARGET_TOUCH					0x50
#define ATI_TARGET_PROX						0x00
#define TAP_TIMER							0x28 //0x18 // 0x18 (If use tap event)  0x00 (If use PO pin status)
#define FLICK_TIMER							0x30
#define FLICK_THRESHOLD						0x36//0x24

/* Set Active Channels (0x0D) */
#define ACTIVE_CHS							0x07

#endif	/* _IQS263_INIT_H_ */
