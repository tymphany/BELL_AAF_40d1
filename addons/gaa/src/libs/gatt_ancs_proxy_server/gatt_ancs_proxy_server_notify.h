/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

#ifndef GATT_ANCS_PROXY_SERVER_NOTIFY_H_
#define GATT_ANCS_PROXY_SERVER_NOTIFY_H_

#include "gatt_ancs_proxy_server_private.h"
#include <gatt_manager.h>

void gattAncsProxySendNotification(GANCS_PROXY_SS *ancs_proxy, uint16 cid, uint16 handle, uint16 length, const uint8 *data);
void gattAncsProxyHandleSendNotification(GANCS_PROXY_SS *ancs_proxy, const GATT_ANCS_PROXY_SERVER_SEND_NOTIFICATION_T *not);
void gattAncsProxyHandleNotificationCfm(GANCS_PROXY_SS *ancs_proxy, const GATT_MANAGER_REMOTE_CLIENT_NOTIFICATION_CFM_T *cfm);
void gattAncsProxyHandleSendNotificationLater(GANCS_PROXY_SS *ancs_proxy);

#endif
