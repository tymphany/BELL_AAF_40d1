/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_ancs_proxy_server_msg_handler.c
@brief   Library message handler
*/

#include "gatt_ancs_proxy_server_msg_handler.h"
#include "gatt_ancs_proxy_server_private.h"
#include "gatt_ancs_proxy_server_access.h"
#include "gatt_ancs_proxy_server_db.h"
#include "gatt_ancs_proxy_server_ready.h"
#include "gatt_ancs_proxy_server_notify.h"
#include <gatt_apple_notification_client.h>

static void handleAncsClientWriteControlPointCfmMsg(GANCS_PROXY_SS *ancs_proxy, const GATT_ANCS_WRITE_CP_CFM_T *cfm)
{
    PRINT(("ANCS PROXY: Write Control Point response, cid [0x%04x], status [0x%04x]\n", cfm->cid, cfm->gatt_status));
    gattAncsProxyServerSendAccessResponse(ancs_proxy, cfm->cid, HANDLE_ANCS_PROXY_CONTROL_POINT_CHAR, cfm->gatt_status, 0, NULL);
}

static void handleAncsClientSetNotificationSourceNotificationCfmMsg(GANCS_PROXY_SS *ancs_proxy, const GATT_ANCS_SET_NS_NOTIFICATION_CFM_T *cfm)
{
    PRINT(("ANCS PROXY: Set Notification Source notification response, cid [0x%04x], status [0x%04x]\n", cfm->cid, cfm->gatt_status));
    gattAncsProxyServerSendAccessResponse(ancs_proxy, cfm->cid, HANDLE_ANCS_PROXY_NOTIFICATION_SOURCE_CHAR_CLIENT_C_CFG, cfm->gatt_status, 0, NULL);
}

static void handleAncsClientSetDataSourceNotificationCfmMsg(GANCS_PROXY_SS *ancs_proxy, const GATT_ANCS_SET_DS_NOTIFICATION_CFM_T *cfm)
{
    PRINT(("ANCS PROXY: Set Data Source notification response, cid [0x%04x], status [0x%04x]\n", cfm->cid, cfm->gatt_status));
    gattAncsProxyServerSendAccessResponse(ancs_proxy, cfm->cid, HANDLE_ANCS_PROXY_DATA_SOURCE_CHAR_CLIENT_C_CFG, cfm->gatt_status, 0, NULL);
}

static void handleGattManagerMessages(GANCS_PROXY_SS *ancs_proxy, MessageId id, Message msg)
{
    switch (id)
    {
        case GATT_MANAGER_SERVER_ACCESS_IND:
            gattAncsProxyHandleServerAccess(ancs_proxy, (GATT_MANAGER_SERVER_ACCESS_IND_T *) msg);
        break;

        case GATT_MANAGER_REMOTE_CLIENT_NOTIFICATION_CFM:
            gattAncsProxyHandleNotificationCfm(ancs_proxy, (GATT_MANAGER_REMOTE_CLIENT_NOTIFICATION_CFM_T *) msg);
        break;

        default:
            DEBUG_PANIC(("ANCS PROXY: GATT manager msg not handled [0x%04x]\n", id));
        break;
    }
}

static void handleAncsClientMessages(GANCS_PROXY_SS *ancs_proxy, MessageId id, Message msg)
{
    switch (id)
    {
        case GATT_ANCS_WRITE_CP_CFM:
            handleAncsClientWriteControlPointCfmMsg(ancs_proxy, (GATT_ANCS_WRITE_CP_CFM_T *) msg);
        break;

        case GATT_ANCS_SET_NS_NOTIFICATION_CFM:
            handleAncsClientSetNotificationSourceNotificationCfmMsg(ancs_proxy, (GATT_ANCS_SET_NS_NOTIFICATION_CFM_T *) msg);
        break;

        case GATT_ANCS_SET_DS_NOTIFICATION_CFM:
            handleAncsClientSetDataSourceNotificationCfmMsg(ancs_proxy, (GATT_ANCS_SET_DS_NOTIFICATION_CFM_T *) msg);
        break;

        default:
            DEBUG_PANIC(("ANCS PROXY: GATT ANCS client msg not handled [0x%04x]\n", id));
        break;
    }
}

static void handleInternalMessages(GANCS_PROXY_SS *ancs_proxy, MessageId id, Message msg)
{
    switch (id)
    {
        case GATT_ANCS_PROXY_SERVER_SEND_NOTIFICATION:
            gattAncsProxyHandleSendNotification(ancs_proxy, (GATT_ANCS_PROXY_SERVER_SEND_NOTIFICATION_T *) msg);
        break;

        case GATT_ANCS_PROXY_SERVER_SEND_NOTIFICATION_LATER:
            gattAncsProxyHandleSendNotificationLater(ancs_proxy);
        break;

        default:
            DEBUG_PANIC(("ANCS PROXY: Internal msg not handled [0x%04x]\n", id));
        break;
    }
}

void gattAncsProxyServerMsgHandler(Task task, MessageId id, Message msg)
{
    GANCS_PROXY_SS *ancs_proxy = (GANCS_PROXY_SS *) task;

    if ((id >= GATT_MANAGER_MESSAGE_BASE) && (id < GATT_MANAGER_MESSAGE_TOP))
        handleGattManagerMessages(ancs_proxy, id, msg);
    else if((id >= GATT_ANCS_MESSAGE_BASE) && (id < GATT_ANCS_MESSAGE_TOP))
        handleAncsClientMessages(ancs_proxy, id, msg);
    else
        handleInternalMessages(ancs_proxy, id, msg);
}
