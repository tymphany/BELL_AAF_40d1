/* Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd. */

#include "gatt_ams_proxy_server_msg_handler.h"
#include "gatt_ams_proxy_server_private.h"
#include "gatt_ams_proxy_server_access.h"
#include "gatt_ams_proxy_server_db.h"
#include "gatt_ams_proxy_server_ready.h"
#include "gatt_ams_proxy_server_notify.h"

static void handleAmsClientWriteRemoteCommandCfmMsg(GAMS_PROXY_SS *ams_proxy, const GATT_AMS_CLIENT_WRITE_REMOTE_COMMAND_CFM_T *cfm)
{
    PRINT(("AMS PROXY: Write Remote Command response, cid [0x%04x], status [0x%04x]\n", cfm->cid, cfm->gatt_status));
    gattAmsProxyServerSendAccessResponse(ams_proxy, cfm->cid, HANDLE_AMS_PROXY_REMOTE_COMMAND_CHAR, cfm->gatt_status, 0, NULL);
}

static void handleAmsClientWriteEntityUpdateCfmMsg(GAMS_PROXY_SS *ams_proxy, const GATT_AMS_CLIENT_WRITE_ENTITY_UPDATE_CFM_T *cfm)
{
    PRINT(("AMS PROXY: Write Entity Update response, cid [0x%04x], status [0x%04x]\n", cfm->cid, cfm->gatt_status));
    gattAmsProxyServerSendAccessResponse(ams_proxy, cfm->cid, HANDLE_AMS_PROXY_ENTITY_UPDATE_CHAR, cfm->gatt_status, 0, NULL);
}

static void handleAmsClientWriteEntityAttributeCfmMsg(GAMS_PROXY_SS *ams_proxy, const GATT_AMS_CLIENT_WRITE_ENTITY_ATTRIBUTE_CFM_T *cfm)
{
    PRINT(("AMS PROXY: Write Entity Attribute response, cid [0x%04x], status [0x%04x]\n", cfm->cid, cfm->gatt_status));
    gattAmsProxyServerSendAccessResponse(ams_proxy, cfm->cid, HANDLE_AMS_PROXY_ENTITY_ATTRIBUTE_CHAR, cfm->gatt_status, 0, NULL);
}

static void handleAmsClientReadEntityAttributeCfmMsg(GAMS_PROXY_SS *ams_proxy, const GATT_AMS_CLIENT_READ_ENTITY_ATTRIBUTE_CFM_T *cfm)
{
    PRINT(("AMS PROXY: Read Entity Attribute response, cid [0x%04x], status [0x%04x]\n", cfm->cid, cfm->gatt_status));
    gattAmsProxyServerSendAccessResponse(ams_proxy, cfm->cid, HANDLE_AMS_PROXY_ENTITY_ATTRIBUTE_CHAR, cfm->gatt_status, cfm->value_size, cfm->value);
}

static void handleAmsClientSetRemoteCommandNotificationCfmMsg(GAMS_PROXY_SS *ams_proxy, const GATT_AMS_CLIENT_SET_REMOTE_COMMAND_NOTIFICATION_CFM_T *cfm)
{
    PRINT(("AMS PROXY: Set Remote Command notification response, cid [0x%04x], status [0x%04x]\n", cfm->cid, cfm->gatt_status));
    gattAmsProxyServerSendAccessResponse(ams_proxy, cfm->cid, HANDLE_AMS_PROXY_REMOTE_COMMAND_CLIENT_C_CFG, cfm->gatt_status, 0, NULL);
}

static void handleAmsClientSetEntityUpdateNotificationCfmMsg(GAMS_PROXY_SS *ams_proxy, const GATT_AMS_CLIENT_SET_ENTITY_UPDATE_NOTIFICATION_CFM_T *cfm)
{
    PRINT(("AMS PROXY: Set Entity Update notification response, cid [0x%04x], status [0x%04x]\n", cfm->cid, cfm->gatt_status));
    gattAmsProxyServerSendAccessResponse(ams_proxy, cfm->cid, HANDLE_AMS_PROXY_ENTITY_UPDATE_CLIENT_C_CFG, cfm->gatt_status, 0, NULL);
}

static void handleGattManagerMessages(GAMS_PROXY_SS *ams_proxy, MessageId id, Message msg)
{
    switch (id)
    {
        case GATT_MANAGER_SERVER_ACCESS_IND:
            gattAmsProxyHandleServerAccess(ams_proxy, (GATT_MANAGER_SERVER_ACCESS_IND_T *) msg);
        break;

        case GATT_MANAGER_REMOTE_CLIENT_NOTIFICATION_CFM:
            gattAmsProxyHandleNotificationCfm(ams_proxy, (GATT_MANAGER_REMOTE_CLIENT_NOTIFICATION_CFM_T *) msg);
        break;

        default:
            DEBUG_PANIC(("AMS PROXY: GATT manager msg not handled [0x%04x]\n", id));
        break;
    }
}

static void handleAmsClientMessages(GAMS_PROXY_SS *ams_proxy, MessageId id, Message msg)
{
    switch (id)
    {
        case GATT_AMS_CLIENT_WRITE_REMOTE_COMMAND_CFM:
            handleAmsClientWriteRemoteCommandCfmMsg(ams_proxy, (GATT_AMS_CLIENT_WRITE_REMOTE_COMMAND_CFM_T *) msg);
        break;

        case GATT_AMS_CLIENT_WRITE_ENTITY_UPDATE_CFM:
            handleAmsClientWriteEntityUpdateCfmMsg(ams_proxy, (GATT_AMS_CLIENT_WRITE_ENTITY_UPDATE_CFM_T *) msg);
        break;

        case GATT_AMS_CLIENT_WRITE_ENTITY_ATTRIBUTE_CFM:
            handleAmsClientWriteEntityAttributeCfmMsg(ams_proxy, (GATT_AMS_CLIENT_WRITE_ENTITY_ATTRIBUTE_CFM_T *) msg);
        break;

        case GATT_AMS_CLIENT_READ_ENTITY_ATTRIBUTE_CFM:
            handleAmsClientReadEntityAttributeCfmMsg(ams_proxy, (GATT_AMS_CLIENT_READ_ENTITY_ATTRIBUTE_CFM_T *) msg);
        break;

        case GATT_AMS_CLIENT_SET_REMOTE_COMMAND_NOTIFICATION_CFM:
            handleAmsClientSetRemoteCommandNotificationCfmMsg(ams_proxy, (GATT_AMS_CLIENT_SET_REMOTE_COMMAND_NOTIFICATION_CFM_T *) msg);
        break;

        case GATT_AMS_CLIENT_SET_ENTITY_UPDATE_NOTIFICATION_CFM:
            handleAmsClientSetEntityUpdateNotificationCfmMsg(ams_proxy, (GATT_AMS_CLIENT_SET_ENTITY_UPDATE_NOTIFICATION_CFM_T *) msg);
        break;

        default:
            DEBUG_PANIC(("AMS PROXY: Ams client msg not handled [0x%04x]\n", id));
        break;
    }
}

static void handleInternalMessages(GAMS_PROXY_SS *ams_proxy, MessageId id, Message msg)
{
    switch (id)
    {
        case GATT_AMS_PROXY_SERVER_SEND_NOTIFICATION:
            gattAmsProxyHandleSendNotification(ams_proxy, (GATT_AMS_PROXY_SERVER_SEND_NOTIFICATION_T *) msg);
        break;

        case GATT_AMS_PROXY_SERVER_SEND_NOTIFICATION_LATER:
            gattAmsProxyHandleSendNotificationLater(ams_proxy);
        break;

        default:
            DEBUG_PANIC(("AMS PROXY: Internal msg not handled [0x%04x]\n", id));
        break;
    }
}

void gattAmsProxyMsgHandler(Task task, MessageId id, Message msg)
{
    GAMS_PROXY_SS *ams_proxy = (GAMS_PROXY_SS *) task;

    if ((id >= GATT_MANAGER_MESSAGE_BASE) && (id < GATT_MANAGER_MESSAGE_TOP))
        handleGattManagerMessages(ams_proxy, id, msg);
    else if((id >= GATT_AMS_CLIENT_MESSAGE_BASE) && (id < GATT_AMS_CLIENT_MESSAGE_TOP))
        handleAmsClientMessages(ams_proxy, id, msg);
    else
        handleInternalMessages(ams_proxy, id, msg);
}
