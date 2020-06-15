/*!
\copyright  Copyright (c) 2017 - 2019 TYMPHANY Technologies International, Ltd.
            All Rights Reserved.
            TYMPHANY Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       tym_anc.h
\brief      Support for the anc interface
*/
#ifndef __TYM_ANC_H__
#define __TYM_ANC_H__

#include <task_list.h>
#include "domain_message.h"

/*! Forward declaration of a config structure (type dependent) */
struct __anc_config;
/*! Proximity config incomplete type */
typedef struct __anc_config ancConfig;

typedef enum{
    amcinvalid,
    ancoff,        /*anc off*/
    ancon,         /*anc on*/   
    speech,      /*talk through ,voice on*/
    ambient,        /*environment on*/
    ancmax,
}ancState_e;

/*! @brief tym anc module state. */
typedef struct
{
    /*! List of registered client tasks */
    task_list_t *clients;
    /*! The config */
    ancConfig *config;
    uint8      prevAncMode;
    uint8      curAncMode;    
    uint8      ambientLevel;
    uint8      speechLevel;  
    uint8      onceAnc;
} tymAncTaskData;

/*!< Task information for proximity sensor */
extern tymAncTaskData app_tymanc;
/*! Get pointer to the proximity sensor data structure */
#define TymAncGetTaskData()   (&app_tymanc)

bool appAncClientRegister(Task task);
void appAncClientUnregister(Task task);
void dumpANCWriteToPSKey(void);
void stanc3_ancoff(void);
void stanc3_ancon(void);
uint8 stanc3_ancvol(void);
void stanc3_audiomute(bool enable);
void stanc3_volumedown(bool enable);
void disable_i2c_for_cal(bool enable);
bool getExtAncEnableStatus(void);
void appAncPowerOff(void);
void appAncPowerOn(void);
bool checkANCVerifyValue(void);
void stanc3_change_register(uint8 reg,uint8 data);
#endif/*__EARBUD_TYM_ANC_H__*/
