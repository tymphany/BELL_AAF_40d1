/*
* This file contains all the necessary settings for the IQS263 and this file can
* be changed from the GUI or edited here
* File:   iqs263.h
* Author: Azoteq
*/

#ifndef _IQS263_H_
#define _IQS263_H_
/*! \brief Types of event that can cause connect rules to run. */
typedef enum iqs263CtrlMsg
{
    iqs263_event,
    iqs263_hold2s,
    iqs263_hold5s,
    iqs263_hold10s,
    iqs263_hold25s,
    iqs263_tap_timeout, //2020-06-12
    iqs263_hold_count, //2020-06-12
} iqs263CtrlMsg_t;


/*! The high level configuration for taking measurement */
struct __touch_config
{
    uint8 hold_interrupt;  //hold pin
    uint8 rdy_interrupt;   //ready pin
    uint8 chan1threshold;
    uint8 holdDuration;        //2020-06-12
    uint8 tapCnt;
    uint8 touchpad;
    bool  holdPio;        //hold pio high/low
    bool  readyPio;       //read pin high/low
    bool  init;
    bool  hold2sTrigger;   //hold2s trigger  
    bool  hold5sTrigger;   //hold5s trigger       
};

/***************************************************************************

**************************************************************************/


/*********************** IQS263 REGISTERS *************************************/

#define DEVICE_INFO				0x00
#define SYS_FLAGS				0x01
#define COORDINATES				0x02
#define TOUCH_BYTES				0x03
#define COUNTS					0x04
#define LTA						0x05
#define DELTAS					0x06
#define MULTIPLIERS				0x07
#define COMPENSATION			0x08
#define PROX_SETTINGS			0x09
#define THRESHOLDS				0x0A
#define TIMINGS_AND_TARGETS		0x0B
#define GESTURE_TIMERS			0x0C
#define ACTIVE_CHANNELS			0x0D


/*--------------------------------------------------------------------------
 * Prototypes
 *--------------------------------------------------------------------------
 */ 
#endif	/* _IQS266_H_ */
