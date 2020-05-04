/* Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd. */

#ifndef GATT_AMS_PROXY_SERVER_ACCESS_H_
#define GATT_AMS_PROXY_SERVER_ACCESS_H_

#include "gatt_ams_proxy_server_private.h"
#include <gatt_manager.h>

/*
    Handles the GATT_MANAGER_SERVER_ACCESS_IND message.
*/
void gattAmsProxyHandleServerAccess(GAMS_PROXY_SS *ams_proxy, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind);

void gattAmsProxyServerSendAccessResponse(GAMS_PROXY_SS *ams_proxy, uint16 cid, uint16 handle, gatt_status_t status, uint16 value_size, const uint8 *value);
void gattAmsProxySendClientConfigReadResponse(GAMS_PROXY_SS *ams_proxy, uint16 cid, uint16 handle, uint16 client_config);

#endif
