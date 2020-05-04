/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_gaa_media_server.h
@brief   Header file for the GATT GAA Media Server library
*/

#ifndef GATT_GAA_MEDIA_SERVER_H_
#define GATT_GAA_MEDIA_SERVER_H_

#include "gatt_manager.h"

#include <csrtypes.h>
#include <message.h>
#include <library.h>

typedef enum
{
    gaa_media_active_app_char,
    gaa_media_command_char,
    gaa_media_status_char,
    gaa_media_broadcast_char,
    gaa_media_char_last
} gatt_gaa_media_char_id_t;

/*!
    @brief Initialises the Apple Proxy Service Library in the Server role.

    @param start_handle start handle
    @param end_handle end handle

    @return TRUE if successful, FALSE otherwise
 */
bool GattGaaMediaServerInit(uint16 start_handle, uint16 end_handle);

/*!
    @brief Associates a connection ID with the GAA Media Service.
    @param cid Connection ID
    @return TRUE if successful, FALSE otherwise
 */
bool GattGaaMediaServerConnect(uint16 cid);

/*!
    @brief Disassociates a connection ID from the GAA Media Service.
    @param cid Connection ID
    @return TRUE if successful, FALSE otherwise
 */
bool GattGaaMediaServerDisconnect(uint16 cid);

#endif /* GATT_GAA_MEDIA_SERVER_H_ */
