/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       gatt_server_gaa_media.h
\brief      Gaa media communication component.
*/

#ifndef _GATT_SERVER_GAA_MEDIA_H_
#define _GATT_SERVER_GAA_MEDIA_H_

#if defined(INCLUDE_GAA_LE) && !defined(INCLUDE_GAA) 
    #error INCLUDE_GAA_LE must be used in conjunction with INCLUDE_GAA
#endif 


#include <gatt_manager.h>
#include <csrtypes.h>
#include <message.h>

/*! \brief Intializes the GAA Media GATT server.
    \param init_task -Client task to respond (optional).
    \return TRUE on successfully initialzing else FALSE.
*/
bool GattServerGaaMedia_Init(Task init_task);


#endif /* _GATT_SERVER_GAA_MEDIA_H_ */
