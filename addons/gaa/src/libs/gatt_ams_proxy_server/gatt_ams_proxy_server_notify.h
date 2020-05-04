/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

#ifndef GATT_AMS_PROXY_SERVER_NOTIFY_H_
#define GATT_AMS_PROXY_SERVER_NOTIFY_H_

#include "gatt_ams_proxy_server_private.h"
#include <gatt_manager.h>

void gattAmsProxySendNotification(GAMS_PROXY_SS *ams_proxy, uint16 cid, uint16 handle, uint16 length, const uint8 *data);
void gattAmsProxyHandleSendNotification(GAMS_PROXY_SS *ams_proxy, const GATT_AMS_PROXY_SERVER_SEND_NOTIFICATION_T *not);
void gattAmsProxyHandleNotificationCfm(GAMS_PROXY_SS *ams_proxy, const GATT_MANAGER_REMOTE_CLIENT_NOTIFICATION_CFM_T *cfm);
void gattAmsProxyHandleSendNotificationLater(GAMS_PROXY_SS *ams_proxy);

#endif
