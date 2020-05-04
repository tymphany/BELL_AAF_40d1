/* Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd. */

#include "gatt_ams_proxy_server_access.h"
#include "gatt_ams_proxy_server.h"
#include "gatt_ams_proxy_server_external_msg_send.h"
#include "gatt_ams_proxy_server_db.h"
#include "gatt_ams_proxy_server_client_list.h"
#include "gatt_ams_proxy_server_ready.h"
#include <byte_utils.h>

static void sendAccessResponseToGattManager(Task task, uint16 cid, uint16 handle, gatt_status_t result_status, uint16 size_value, const uint8 *value)
{
    if (!GattManagerServerAccessResponse(task, cid, handle, result_status, size_value, value))
        DEBUG_PANIC(("AMS PROXY: GATT manager access response failed, handle [0x%04x], cid [0x%04x]\n", handle, cid));
}

static void sendAccessErrorResponseToGattManager(GAMS_PROXY_SS *ams_proxy, uint16 cid, uint16 handle, gatt_status_t error_status)
{
    sendAccessResponseToGattManager(&ams_proxy->lib_task, cid, handle, error_status, 0, NULL);
}

static void sendReadSuccessResponseToGattManager(GAMS_PROXY_SS *ams_proxy, uint16 cid, uint16 handle, uint16 size_value, const uint8 *value)
{
    sendAccessResponseToGattManager(&ams_proxy->lib_task, cid, handle, gatt_status_success, size_value, value);
}

static void sendWriteSuccessResponseToGattManager(GAMS_PROXY_SS *ams_proxy, uint16 cid, uint16 handle)
{
    sendAccessErrorResponseToGattManager(ams_proxy, cid, handle, gatt_status_success);
}

static void readCharacteristicConfig(GAMS_PROXY_SS *ams_proxy, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->handle == HANDLE_AMS_PROXY_READY_CLIENT_C_CFG)
    {
        PRINT(("AMS PROXY: Read Ready config\n"));
        /* Send GATT manager our response when the application calls GattAmsProxyServerReadReadyClientConfigResponse() */
        gattAmsProxySendReadReadyConfigInd(ams_proxy, access_ind->cid);
    }
    else
    {
        /* Assuming all other configs are WRITE (which is probably wrong) */
        PRINT(("AMS PROXY: Read characteristic config not handled!!! Handle [0x%04x]\n", access_ind->handle));
        sendAccessErrorResponseToGattManager(ams_proxy, access_ind->cid, access_ind->handle, gatt_status_read_not_permitted);
    }
}

static void writeReadyConfig(GAMS_PROXY_SS *ams_proxy, uint16 cid, uint16 ready_c_cfg)
{
    PRINT(("AMS PROXY: Write Ready config, ready_c_cfg [0x%04x]\n", ready_c_cfg));

    gattAmsProxySendWriteReadyConfigInd(ams_proxy, cid, ready_c_cfg);

    /* It is the applications responsibility to make sure the ready config value is written, respond to GATT manager now */
    sendWriteSuccessResponseToGattManager(ams_proxy, cid, HANDLE_AMS_PROXY_READY_CLIENT_C_CFG);

    if (ready_c_cfg & gatt_client_char_config_notifications_enabled)
        GattAmsProxyServerSendReadyCharNotification(cid);
}

static void writeAmsCharacteristicConfig(GAMS_PROXY_SS *ams_proxy, uint16 cid, uint16 handle, uint16 client_config)
{
    const GAMS *client = gattAmsProxyGetClient(cid);
    bool notification_enable;

    if (client == NULL)
    {
        PRINT(("AMS PROXY: Write characteristic config, client not found!!!\n"));
        sendAccessErrorResponseToGattManager(ams_proxy, cid, handle, gatt_status_failure);
        return;
    }

    if (client_config & gatt_client_char_config_notifications_enabled)
        notification_enable = TRUE;
    else
        notification_enable = FALSE;

    switch (handle)
    {
        case HANDLE_AMS_PROXY_REMOTE_COMMAND_CLIENT_C_CFG:
            PRINT(("AMS PROXY: Write Remote Command config, notify = %d\n", notification_enable));
            /* Will send GATT manager the response when the client response is received */
            GattAmsSetRemoteCommandNotificationEnableRequest(&ams_proxy->lib_task, client, notification_enable);
            break;

        case HANDLE_AMS_PROXY_ENTITY_UPDATE_CLIENT_C_CFG:
            PRINT(("AMS PROXY: Write Entity Update config, notify = %d\n", notification_enable));
            /* Will send GATT manager the response when the client response is received */
            GattAmsSetEntityUpdateNotificationEnableRequest(&ams_proxy->lib_task, client, notification_enable);
            break;

        default:
            PRINT(("AMS PROXY: Write characteristic config, unknown handle [0x%04x]!!!\n", handle));
            sendAccessErrorResponseToGattManager(ams_proxy, cid, handle, gatt_status_write_not_permitted);
            break;
    }
}

static void writeCharacteristicConfig(GAMS_PROXY_SS *ams_proxy, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->size_value != GATT_CLIENT_CHAR_CONFIG_SIZE)
    {
        PRINT(("AMS PROXY: Write characteristic config invalid length!!! Handle [0x%04x], length [%d]\n", access_ind->handle, access_ind->size_value));
        sendAccessErrorResponseToGattManager(ams_proxy, access_ind->cid, access_ind->handle, gatt_status_invalid_length);
    }
    else
    {
        uint16 client_config = MAKEWORD(access_ind->value[0], access_ind->value[1]);

        if (access_ind->handle == HANDLE_AMS_PROXY_READY_CLIENT_C_CFG)
            writeReadyConfig(ams_proxy, access_ind->cid, client_config);
        else
            writeAmsCharacteristicConfig(ams_proxy, access_ind->cid, access_ind->handle, client_config);
    }
}

static void readReadyCharacteristic(GAMS_PROXY_SS *ams_proxy, uint16 cid)
{
    uint8 characteristic[ready_size];
    uint16 ready_char = gattAmsProxyGetReadyCharacteristic(cid);

    characteristic[0] = LOBYTE(ready_char);
    characteristic[1] = HIBYTE(ready_char);

    PRINT(("AMS PROXY: Read Ready characteristic, value [0x%04x]\n", ready_char));

    sendReadSuccessResponseToGattManager(ams_proxy, cid, HANDLE_AMS_PROXY_READY_CHAR, ready_size, characteristic);
}

static void readAmsCharacteristic(GAMS_PROXY_SS *ams_proxy, uint16 cid, uint16 handle)
{
    const GAMS *client = gattAmsProxyGetClient(cid);

    if (client == NULL)
    {
        PRINT(("AMS PROXY: Read characteristic, client not found!!!\n"));
        sendAccessErrorResponseToGattManager(ams_proxy, cid, handle, gatt_status_failure);
        return;
    }

    switch (handle)
    {
        case HANDLE_AMS_PROXY_ENTITY_ATTRIBUTE_CHAR:
            PRINT(("AMS PROXY: Read Entity Attribute\n"));
            /* Will send GATT manager the response when the client response is received */
            GattAmsReadEntityAttribute(&ams_proxy->lib_task, client);
            break;

        default:
            PRINT(("AMS PROXY: Read characteristic, unknown handle [0x%04x]!!!\n", handle));
            sendAccessErrorResponseToGattManager(ams_proxy, cid, handle, gatt_status_read_not_permitted);
            break;
    }
}

static void readCharacteristic(GAMS_PROXY_SS *ams_proxy, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->handle == HANDLE_AMS_PROXY_READY_CHAR)
        readReadyCharacteristic(ams_proxy, access_ind->cid);
    else
        readAmsCharacteristic(ams_proxy, access_ind->cid, access_ind->handle);
}

static void printRemoteCommandType(gatt_ams_remote_command_id_t remote_cmd)
{
    switch (remote_cmd)
    {
        case RemoteCommandIDPlay:
            PRINT(("Play"));
            break;
        case RemoteCommandIDPause:
            PRINT(("Pause"));
            break;
        case RemoteCommandIDTogglePlayPause:
            PRINT(("PlayPause"));
            break;
        case RemoteCommandIDNextTrack:
            PRINT(("Next Track"));
            break;
        case RemoteCommandIDPreviousTrack:
            PRINT(("Previous Track"));
            break;
        case RemoteCommandIDVolumeUp:
            PRINT(("Volume Up"));
            break;
        case RemoteCommandIDVolumeDown:
            PRINT(("Volume Down"));
            break;
        case RemoteCommandIDAdvanceRepeatMode:
            PRINT(("Advance Repeat Mode"));
            break;
        case RemoteCommandIDAdvanceShuffleMode:
            PRINT(("Advance Shuffle Mode"));
            break;
        case RemoteCommandIDSkipForward:
            PRINT(("Skip Forward"));
            break;
        case RemoteCommandIDSkipBackward:
            PRINT(("Skip Backward"));
            break;
        case RemoteCommandIDLikeTrack:
            PRINT(("Like Track"));
            break;
        case RemoteCommandIDDislikeTrack:
            PRINT(("Dislike Track"));
            break;
        case RemoteCommandIDBookmarkTrack:
            PRINT(("BookmarkTrack"));
            break;
        default:
            PRINT(("Unknown Remote Command [%u]", remote_cmd));
            break;
    }
}

static void writeCharacteristic(GAMS_PROXY_SS *ams_proxy, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    const GAMS *client = gattAmsProxyGetClient(access_ind->cid);

    if (client == NULL)
    {
        PRINT(("AMS PROXY: Write characteristic, client not found!!!\n"));
        sendAccessErrorResponseToGattManager(ams_proxy, access_ind->cid, access_ind->handle, gatt_status_failure);
        return;
    }

    switch (access_ind->handle)
    {
        case HANDLE_AMS_PROXY_REMOTE_COMMAND_CHAR:
            PRINT(("AMS PROXY: Write Remote Command "));
            printRemoteCommandType(access_ind->value[0]);
            PRINT(("\n"));

            if (access_ind->size_value == remote_command_size)
            {
                /* Response send to GATT manager when client response is received */
                GattAmsWriteRemoteCommand(&ams_proxy->lib_task, client, access_ind->value[0]);
            }
            else
                sendAccessErrorResponseToGattManager(ams_proxy, access_ind->cid, access_ind->handle, gatt_status_invalid_length);
            break;

        case HANDLE_AMS_PROXY_ENTITY_UPDATE_CHAR:
            PRINT(("AMS PROXY: Write Entity Update\n"));
            if (access_ind->size_value >= entity_update_size_not_counting_attributes)
            {
                uint16 size_attribute_list = access_ind->size_value - entity_update_size_not_counting_attributes;
                /* Response send to GATT manager when client response is received */
                GattAmsWriteEntityUpdate(&ams_proxy->lib_task, client, access_ind->value[0], &access_ind->value[1], size_attribute_list);
            }
            else
                sendAccessErrorResponseToGattManager(ams_proxy, access_ind->cid, access_ind->handle, gatt_status_invalid_length);
            break;

        case HANDLE_AMS_PROXY_ENTITY_ATTRIBUTE_CHAR:
            PRINT(("AMS PROXY: Write Entity Attribute\n"));
            if (access_ind->size_value == entity_attribute_size)
            {
                /* Response send to GATT manager when client response is received */
                GattAmsWriteEntityAttribute(&ams_proxy->lib_task, client, access_ind->value[0], access_ind->value[1]);
            }
            else
                sendAccessErrorResponseToGattManager(ams_proxy, access_ind->cid, access_ind->handle, gatt_status_invalid_length);
            break;

        default:
            PRINT(("AMS PROXY: Write characteristic, unknown handle [0x%04x]!!!\n", access_ind->handle));
            sendAccessErrorResponseToGattManager(ams_proxy, access_ind->cid, access_ind->handle, gatt_status_write_not_permitted);
            break;
    }
}

static void accessAmsProxyService(GAMS_PROXY_SS *ams_proxy, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    PRINT(("AMS PROXY: Service attribute access\n"));

    if (access_ind->flags & ATT_ACCESS_READ)
        sendReadSuccessResponseToGattManager(ams_proxy, access_ind->cid, access_ind->handle, 0, NULL);
    else if (access_ind->flags & ATT_ACCESS_WRITE)
        sendAccessErrorResponseToGattManager(ams_proxy, access_ind->cid, access_ind->handle, gatt_status_write_not_permitted);
    else
        sendAccessErrorResponseToGattManager(ams_proxy, access_ind->cid, access_ind->handle, gatt_status_request_not_supported);
}

static void accessCharacteristicConfig(GAMS_PROXY_SS *ams_proxy, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->flags & ATT_ACCESS_READ)
        readCharacteristicConfig(ams_proxy, access_ind);
    else if (access_ind->flags & ATT_ACCESS_WRITE)
        writeCharacteristicConfig(ams_proxy, access_ind);
    else
        sendAccessErrorResponseToGattManager(ams_proxy, access_ind->cid, access_ind->handle, gatt_status_request_not_supported);
}

static void accessCharacteristic(GAMS_PROXY_SS *ams_proxy, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->flags & ATT_ACCESS_READ)
        readCharacteristic(ams_proxy, access_ind);
    else if (access_ind->flags & ATT_ACCESS_WRITE)
        writeCharacteristic(ams_proxy, access_ind);
    else
        sendAccessErrorResponseToGattManager(ams_proxy, access_ind->cid, access_ind->handle, gatt_status_request_not_supported);
}

void gattAmsProxyHandleServerAccess(GAMS_PROXY_SS *ams_proxy, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    PRINT(("AMS PROXY: Client access with cid [0x%04x]\n", access_ind->cid));

    switch (access_ind->handle)
    {
        case HANDLE_AMS_PROXY_SERVICE:
            accessAmsProxyService(ams_proxy, access_ind);
            break;

        case HANDLE_AMS_PROXY_REMOTE_COMMAND_CLIENT_C_CFG:
        case HANDLE_AMS_PROXY_ENTITY_UPDATE_CLIENT_C_CFG:
        case HANDLE_AMS_PROXY_READY_CLIENT_C_CFG:
            accessCharacteristicConfig(ams_proxy, access_ind);
            break;

        case HANDLE_AMS_PROXY_REMOTE_COMMAND_CHAR:
        case HANDLE_AMS_PROXY_ENTITY_UPDATE_CHAR:
        case HANDLE_AMS_PROXY_ENTITY_ATTRIBUTE_CHAR:
        case HANDLE_AMS_PROXY_READY_CHAR:
            accessCharacteristic(ams_proxy, access_ind);
            break;

        default:
            sendAccessErrorResponseToGattManager(ams_proxy, access_ind->cid, access_ind->handle, gatt_status_invalid_handle);
            break;
    }
}

void gattAmsProxyServerSendAccessResponse(GAMS_PROXY_SS *ams_proxy, uint16 cid, uint16 handle, gatt_status_t status, uint16 value_size, const uint8 *value)
{
    sendAccessResponseToGattManager(&ams_proxy->lib_task, cid, handle, status, value_size, value);
}

void gattAmsProxySendClientConfigReadResponse(GAMS_PROXY_SS *ams_proxy, uint16 cid, uint16 handle, uint16 client_config)
{
    uint8 config_resp[GATT_CLIENT_CHAR_CONFIG_SIZE];

    config_resp[0] = LOBYTE(client_config);
    config_resp[1] = HIBYTE(client_config);

    sendReadSuccessResponseToGattManager(ams_proxy, cid, handle, GATT_CLIENT_CHAR_CONFIG_SIZE, config_resp);
}
