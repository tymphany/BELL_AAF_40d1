/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file		gatt_server_ancs_proxy.h
\brief      Routines to handle messages sent from the GATT ANCS Proxy server.
*/

#ifndef _GATT_SERVER_ANCS_PROXY_H_
#define _GATT_SERVER_ANCS_PROXY_H_

#if defined(INCLUDE_GAA_LE) && !defined(INCLUDE_GAA) 
    #error INCLUDE_GAA_LE must be used in conjunction with INCLUDE_GAA
#endif 


#include <csrtypes.h>
#include <message.h>


bool GattServerAncsProxy_Init(Task init_task);

#endif /* _GATT_SERVER_ANCS_PROXY_H_ */
