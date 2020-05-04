/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_gaa_media_server_msg_handler.c
@brief   Library message handler
*/

#include "gatt_gaa_media_server_msg_handler.h"
#include "gatt_gaa_media_server_access.h"
#include "gatt_gaa_media_server_debug.h"

static void handleGaaMediaClientNotificationCfm(GATT_NOTIFICATION_CFM_T *msg)
{
    UNUSED(msg);
    GATT_GAA_MEDIA_SERVER_DEBUG_INFO(("GAA_MEDIA NOT CFM s %d, %04x, %d\n", msg->status, msg->cid, msg->handle));
}

void GattGaaMediaServerMsgHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
        case GATT_MANAGER_SERVER_ACCESS_IND:
            handleGaaMediaServerAccess((GATT_MANAGER_SERVER_ACCESS_IND_T *) message);
            break;

        case GATT_MANAGER_REMOTE_CLIENT_NOTIFICATION_CFM:
            handleGaaMediaClientNotificationCfm((GATT_NOTIFICATION_CFM_T *) message);
            break;

        default:
            GATT_GAA_MEDIA_SERVER_DEBUG_INFO(("GAA_MEDIA unhandled 0x%04X\n", id));
            break;
    }
}
