/*!
\copyright  Copyright (c) 2005 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   key_sync TYM SYNC
\ingroup    bt_domain
\brief      Key synchronisation component.
*/

#ifndef TYM_SYNC_H
#define TYM_SYNC_H

#include <domain_message.h>

#include <message.h>
#include "earbud_tym_sync_defs.h"

#define TYM_LEFT_EARBUD     0x10
#define TYM_RIGHT_EARBUD    0x20

typedef enum btSyncCmd_e{
    btStatusCmd,
    noCasePairCmd,
    ambientLevelCmd,
    speechLevelCmd,    
    sleepStandbyModeCmd,     
}btSyncCmd_t;

/*\{*/
typedef enum btStatus_e{
    btConnectable,
    btPairing,
    btPairingSuccessful,
    btConnected,
    btDisconnect,
    restoreDefault,
    startOTA,
    OTAFinish,
    happenErr,        
}btStatus_t;

/*!< Task information for proximity sensor */
extern tym_sync_app_configuration_t app_config_setting;
/*! Get pointer to the proximity sensor data structure */
#define TymGet_AppSetting()   (&app_config_setting)

/*! \brief Initialise the key sync component. */
bool TymSync_Init(Task init_task);

void tymSyncdata(uint8 command,uint8 data);
void tymSyncAppConfiguration(tym_sync_app_configuration_t *config);
/*\}*/

#endif /* KEY_SYNC_H */
