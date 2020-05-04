/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_ancs_proxy_server.h
@brief   Header file for the GATT ANCS proxy server library
*/

#ifndef GATT_ANCS_PROXY_SERVER_H_
#define GATT_ANCS_PROXY_SERVER_H_

#include <csrtypes.h>
#include <library.h>
#include <message.h>

/*!
    @brief Enumeration of messages an application task can receive from the library.
*/
typedef enum
{
    /* Must be handled by the application */
    GATT_ANCS_PROXY_SERVER_READ_READY_C_CFG_IND = GATT_ANCS_PROXY_SERVER_MESSAGE_BASE,
    GATT_ANCS_PROXY_SERVER_WRITE_READY_C_CFG_IND,
    GATT_ANCS_PROXY_SERVER_READY_CHAR_UPDATE_IND,

    GATT_ANCS_PROXY_SERVER_MESSAGE_TOP
} gatt_ancs_proxy_server_message_id_t;

typedef struct
{
    uint16 cid;
} GATT_ANCS_PROXY_SERVER_READ_READY_C_CFG_IND_T;

typedef struct
{
    uint16 cid;
    uint16 ready_c_cfg;
} GATT_ANCS_PROXY_SERVER_WRITE_READY_C_CFG_IND_T;

typedef struct
{
    uint16 cid;
    uint16 ready_char;
} GATT_ANCS_PROXY_SERVER_READY_CHAR_UPDATE_IND_T;

/*!
    @brief Initialises the ANCS proxy server library.

    @param app_task The application task to receive the messages sent from this library.
    @param start_handle start handle.
    @param end_handle end handle.

    @return TRUE if successful, FALSE otherwise
*/
bool GattAncsProxyServerInit(Task app_task, uint16 start_handle, uint16 end_handle);

/*!
    @brief Sends a read ready characteristic configuration response to the remote client with this connection ID.
    Must be called by the application in response to GATT_ANCS_PROXY_SERVER_READ_READY_C_CFG_IND.

    @param cid Connection ID included in the GATT_ANCS_PROXY_SERVER_READ_READY_C_CFG_IND message.
    @param ready_c_cfg The ready characteristic configuration for the client.
*/
void GattAncsProxyServerReadReadyClientConfigResponse(uint16 cid, uint16 ready_c_cfg);

/*!
    @brief Sends a notification for the ready characteristic to the remote client with this connection ID.
    Must only be called by the application in response to GATT_ANCS_PROXY_SERVER_READY_CHAR_UPDATE_IND.
    The application must determine if it is required to call this API or not (based on the ready characteristic configuration).

    @param cid Connection ID included in the GATT_ANCS_PROXY_SERVER_READY_CHAR_UPDATE_IND message.
*/
void GattAncsProxyServerSendReadyCharNotification(uint16 cid);

#endif
