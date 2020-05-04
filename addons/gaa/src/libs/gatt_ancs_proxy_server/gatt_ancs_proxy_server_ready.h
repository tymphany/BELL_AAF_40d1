/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

#ifndef GATT_ANCS_PROXY_SERVER_READY_H_
#define GATT_ANCS_PROXY_SERVER_READY_H_

#include "gatt_ancs_proxy_server_private.h"
#include <gatt_apple_notification_client.h>

uint16 gattAncsProxyGetReadyCharacteristic(uint16 cid);
void gattAncsProxyClientIsReady(GANCS_PROXY_SS *ancs_proxy, const GANCS *client);
void gattAncsProxyClientIsDestroyed(GANCS_PROXY_SS *ancs_proxy, const GANCS *client);

#endif
