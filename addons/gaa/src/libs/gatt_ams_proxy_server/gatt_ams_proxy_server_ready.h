/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

#ifndef GATT_AMS_PROXY_SERVER_READY_H_
#define GATT_AMS_PROXY_SERVER_READY_H_

#include "gatt_ams_proxy_server_private.h"
#include <gatt_ams_client.h>

uint16 gattAmsProxyGetReadyCharacteristic(uint16 cid);
void gattAmsProxyClientIsReady(GAMS_PROXY_SS *ams_proxy, const GAMS *client);
void gattAmsProxyClientIsDestroyed(GAMS_PROXY_SS *ams_proxy, const GAMS *client);

#endif
