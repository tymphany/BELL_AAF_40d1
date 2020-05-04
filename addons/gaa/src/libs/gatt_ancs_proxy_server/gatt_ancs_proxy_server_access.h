/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_ancs_proxy_server_access.h
@brief   Server read/write access
*/

#ifndef GATT_ANCS_PROXY_SERVER_ACCESS_H_
#define GATT_ANCS_PROXY_SERVER_ACCESS_H_

#include "gatt_ancs_proxy_server_private.h"
#include <gatt_manager.h>

/*
    Handles the GATT_MANAGER_SERVER_ACCESS_IND message.
*/
void gattAncsProxyHandleServerAccess(GANCS_PROXY_SS *ancs_proxy, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind);

void gattAncsProxyServerSendAccessResponse(GANCS_PROXY_SS *ancs_proxy, uint16 cid, uint16 handle, gatt_status_t status, uint16 value_size, const uint8 *value);
void gattAncsProxySendClientConfigReadResponse(GANCS_PROXY_SS *ancs_proxy, uint16 cid, uint16 handle, uint16 client_config);

#endif
