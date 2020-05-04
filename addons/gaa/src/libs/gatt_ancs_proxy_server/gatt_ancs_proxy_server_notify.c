/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_ancs_proxy_server_notify.c
@brief   Handle notifications to the server
*/

#include "gatt_ancs_proxy_server_notify.h"
#include <string.h>

#define CONNECTION_EVENT_INTERVAL (5)
#define NOTIFICATION_BUFFER_SIZE  (40)

typedef struct
{
    uint16 is_notification_pending;
    uint16 cid;
    uint16 handle;
    uint16 content_length;
    uint8  content[NOTIFICATION_BUFFER_SIZE];
} notification_t;

static notification_t notification;

static void releaseNotificationLock(void)
{
    notification.is_notification_pending = FALSE;
}

static void lockNotifications(void)
{
    notification.is_notification_pending = TRUE;
}

static void sendNotificationToGattManager(GANCS_PROXY_SS *ancs_proxy, uint16 cid, uint16 handle, uint16 length, const uint8 *data)
{
    uint16 tx_mtu_size = GattGetMaxTxDataLength(cid);

    /* The connection id must be invalid at this point. Ignore the notification request. */
    if (tx_mtu_size == 0)
    {
        PRINT(("ANCS PROXY: Invalid MTU, drop notification\n"));
        releaseNotificationLock();
        return;
    }

    while(length)
    {
        uint16 bytes_to_tx = MIN(length, tx_mtu_size);

        GattManagerRemoteClientNotify(&ancs_proxy->lib_task, cid, handle, bytes_to_tx, data);

        data += bytes_to_tx;
        length -= bytes_to_tx;
    }
}

void gattAncsProxySendNotification(GANCS_PROXY_SS *ancs_proxy, uint16 cid, uint16 handle, uint16 length, const uint8 *data)
{
    MAKE_GATT_ANCS_PROXY_SERVER_MESSAGE_WITH_LEN(GATT_ANCS_PROXY_SERVER_SEND_NOTIFICATION, length);
    message->cid = cid;
    message->handle = handle;
    message->length = length;
    memcpy(message->value, data, length);

    if (notification.is_notification_pending)
    {
        PRINT(("ANCS PROXY: Queue notification\n"));
        MessageSendConditionally(&ancs_proxy->lib_task, GATT_ANCS_PROXY_SERVER_SEND_NOTIFICATION, message, &notification.is_notification_pending);
    }
    else
    {
        PRINT(("ANCS PROXY: No notifications pending\n"));
        gattAncsProxyHandleSendNotification(ancs_proxy, message);
        free(message);
    }
}

void gattAncsProxyHandleSendNotification(GANCS_PROXY_SS *ancs_proxy, const GATT_ANCS_PROXY_SERVER_SEND_NOTIFICATION_T *not)
{
    if (not->length > NOTIFICATION_BUFFER_SIZE)
        PANIC(("ANCS PROXY: Notification buffer not large enough\n"));

    lockNotifications();
    notification.cid = not->cid;
    notification.handle = not->handle;
    notification.content_length = not->length;
    memcpy(notification.content, not->value, not->length);

    sendNotificationToGattManager(ancs_proxy, not->cid, not->handle, not->length, not->value);
}

void gattAncsProxyHandleNotificationCfm(GANCS_PROXY_SS *ancs_proxy, const GATT_MANAGER_REMOTE_CLIENT_NOTIFICATION_CFM_T *cfm)
{
    if (cfm->status == gatt_status_success)
        releaseNotificationLock();
    else
    {
        PRINT(("ANCS PROXY: Failed to send notification, handle [0x%04x], status [%d]\n", cfm->handle, cfm->status));
        MessageSendLater(&ancs_proxy->lib_task, GATT_ANCS_PROXY_SERVER_SEND_NOTIFICATION_LATER, NULL, CONNECTION_EVENT_INTERVAL);
    }
}

void gattAncsProxyHandleSendNotificationLater(GANCS_PROXY_SS *ancs_proxy)
{
    sendNotificationToGattManager(ancs_proxy, notification.cid, notification.handle, notification.content_length, notification.content);
}
