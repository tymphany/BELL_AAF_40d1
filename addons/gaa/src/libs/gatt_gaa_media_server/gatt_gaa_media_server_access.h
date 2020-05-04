/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_gaa_media_server_access.h
@brief   Server read/write access
*/

#ifndef GATT_GAA_MEDIA_SERVER_ACCESS_H_
#define GATT_GAA_MEDIA_SERVER_ACCESS_H_

#include "gatt_gaa_media_server.h"

/***************************************************************************
NAME
    handleGaaMediaServerAccess

DESCRIPTION
    Handles the GATT_MANAGER_SERVER_ACCESS_IND message
*/
void handleGaaMediaServerAccess(const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind);

/***************************************************************************
NAME
    sendGaaMediaConfigAccessRsp

DESCRIPTION
    Sends an client configuration access response back to the GATT Manager library.
*/
void sendGaaMediaConfigAccessRsp(uint16 cid, uint16 handle, uint16 client_config);

#endif  /* GATT_GAA_MEDIA_SERVER_ACCESS_H_ */
