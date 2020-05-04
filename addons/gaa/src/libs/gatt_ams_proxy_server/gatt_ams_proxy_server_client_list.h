/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

#ifndef GATT_AMS_PROXY_SERVER_CLIENT_LIST_H_
#define GATT_AMS_PROXY_SERVER_CLIENT_LIST_H_

#include "gatt_ams_proxy_server_private.h"
#include <gatt_ams_client.h>

void gattAmsProxyAddClient(GAMS_PROXY_SS *ams_proxy, const GAMS *client);
bool gattAmsProxyRemoveClient(const GAMS *client);
const GAMS * gattAmsProxyGetClient(uint16 cid);

#endif
