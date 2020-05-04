/* Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd. */

#include "gatt_ams_proxy_server_notify.h"
#include <string.h>

#define NOTIFICATION_BUFFER_SIZE 40

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

static void sendNotificationToGattManager(GAMS_PROXY_SS *ams_proxy, uint16 cid, uint16 handle, uint16 length, const uint8 *data)
{
    uint16 tx_mtu_size = GattGetMaxTxDataLength(cid);

    /* The connection id must be invalid at this point. Ignore the notification request. */
    if (tx_mtu_size == 0)
    {
        PRINT(("AMS PROXY: Invalid MTU, drop notification\n"));
        releaseNotificationLock();
        return;
    }

    while(length)
    {
        uint16 bytes_to_tx = MIN(length, tx_mtu_size);

        GattManagerRemoteClientNotify(&ams_proxy->lib_task, cid, handle, bytes_to_tx, data);

        data += bytes_to_tx;
        length -= bytes_to_tx;
    }
}

void gattAmsProxySendNotification(GAMS_PROXY_SS *ams_proxy, uint16 cid, uint16 handle, uint16 length, const uint8 *data)
{
    MAKE_GATT_AMS_PROXY_SERVER_MESSAGE_WITH_LEN(GATT_AMS_PROXY_SERVER_SEND_NOTIFICATION, length);
    message->cid = cid;
    message->handle = handle;
    message->length = length;
    memcpy(message->value, data, length);

    if (notification.is_notification_pending)
    {
        PRINT(("AMS PROXY: Queue notification\n"));
        MessageSendConditionally(&ams_proxy->lib_task, GATT_AMS_PROXY_SERVER_SEND_NOTIFICATION, message, &notification.is_notification_pending);
    }
    else
    {
        PRINT(("AMS PROXY: No notifications pending\n"));
        gattAmsProxyHandleSendNotification(ams_proxy, message);
        free(message);
    }
}

void gattAmsProxyHandleSendNotification(GAMS_PROXY_SS *ams_proxy, const GATT_AMS_PROXY_SERVER_SEND_NOTIFICATION_T *not)
{
    if (not->length > NOTIFICATION_BUFFER_SIZE)
        PANIC(("AMS PROXY: Notification buffer not large enough\n"));

    lockNotifications();
    notification.cid = not->cid;
    notification.handle = not->handle;
    notification.content_length = not->length;
    memcpy(notification.content, not->value, not->length);

    sendNotificationToGattManager(ams_proxy, not->cid, not->handle, not->length, not->value);
}

void gattAmsProxyHandleNotificationCfm(GAMS_PROXY_SS *ams_proxy, const GATT_MANAGER_REMOTE_CLIENT_NOTIFICATION_CFM_T *cfm)
{
    if (cfm->status == gatt_status_success)
        releaseNotificationLock();
    else
    {
        /* the failure is gatt_status_failure = 257 */
        PRINT(("AMS PROXY: Failed to send notification, handle [0x%04x], status [%d]\n", cfm->handle, cfm->status));

        #define CONNECTION_EVENT_INTERVAL 30
        MessageSendLater(&ams_proxy->lib_task, GATT_AMS_PROXY_SERVER_SEND_NOTIFICATION_LATER, NULL, CONNECTION_EVENT_INTERVAL);
    }
}

void gattAmsProxyHandleSendNotificationLater(GAMS_PROXY_SS *ams_proxy)
{
   sendNotificationToGattManager(ams_proxy, notification.cid, notification.handle, notification.content_length, notification.content);
}
