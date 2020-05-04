/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

#ifndef GATT_ANCS_PROXY_SERVER_CLIENT_LIST_H_
#define GATT_ANCS_PROXY_SERVER_CLIENT_LIST_H_

#include "gatt_ancs_proxy_server_private.h"
#include <gatt_apple_notification_client.h>

void gattAncsProxyAddClient(GANCS_PROXY_SS *ancs_proxy, const GANCS *client);
bool gattAncsProxyRemoveClient(const GANCS *client);
const GANCS * gattAncsProxyGetClient(uint16 cid);

#endif
