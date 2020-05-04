/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_gaa_media_server_access.c
@brief   Server read/write access
*/

#include "gatt_gaa_media_server_access.h"
#include "gatt_gaa_media_server_db.h"
#include <byte_utils.h>
#include <string.h>
#include <stdlib.h>
#include <print.h>
#include "gatt_gaa_media_server_debug.h"
#include "gatt_gaa_media_server_private.h"

static void sendGattManagerAccessResponse(uint16 cid, uint16 handle, uint16 result, uint16 size_value, const uint8 *value)
{
    GATT_GAA_MEDIA_SERVER_DEBUG_INFO(("GAA_MEDIA response cid=0x%04X hdl=0x%04X res=%u siz=%u\n", cid, handle, result, size_value));
    if (!GattManagerServerAccessResponse(&gaa_media_local_server->lib_task, cid, handle, result, size_value, value))
    {
        GATT_GAA_MEDIA_SERVER_DEBUG_PANIC(("- couldn't send\n"));
    }
}

static void sendGattManagerErrorAccessResponse(const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind, uint16 error)
{
    sendGattManagerAccessResponse(access_ind->cid, access_ind->handle, error, 0, NULL);
}

static void accessGaaMediaService(const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        sendGattManagerAccessResponse(access_ind->cid, access_ind->handle, gatt_status_success, 0, NULL);
    }
    else if (access_ind->flags & ATT_ACCESS_WRITE)
    {
        sendGattManagerErrorAccessResponse(access_ind, gatt_status_write_not_permitted);
    }
    else
    {
        sendGattManagerErrorAccessResponse(access_ind, gatt_status_request_not_supported);
    }
}

static gatt_gaa_media_char_id_t getCharIdFromConfigHandle(uint16 handle)
{
    switch (handle)
    {
        case HANDLE_GAA_MEDIA_ACTIVE_APP_CLIENT_C_CFG:
            return gaa_media_active_app_char;

        case HANDLE_GAA_MEDIA_COMMAND_CLIENT_C_CFG:
            return gaa_media_command_char;

        case HANDLE_GAA_MEDIA_STATUS_CLIENT_C_CFG:
            return gaa_media_status_char;

        case HANDLE_GAA_MEDIA_BROADCAST_CLIENT_C_CFG:
            return gaa_media_broadcast_char;

        default:
            GATT_GAA_MEDIA_SERVER_DEBUG_PANIC(("Unknown handle\n"));
            return gaa_media_char_last;
    }
}

static gatt_gaa_media_char_id_t getCharIdFromCharHandle(uint16 handle)
{
    switch (handle)
    {
        case HANDLE_GAA_MEDIA_ACTIVE_APP_CHAR:
            return gaa_media_active_app_char;

        case HANDLE_GAA_MEDIA_COMMAND_CHAR:
            return gaa_media_command_char;

        case HANDLE_GAA_MEDIA_STATUS_CHAR:
            return gaa_media_status_char;

        case HANDLE_GAA_MEDIA_BROADCAST_CHAR:
            return gaa_media_broadcast_char;

        default:
            GATT_GAA_MEDIA_SERVER_DEBUG_PANIC(("Unknown handle\n"));
            return gaa_media_char_last;
    }
}

static void notifyClients(uint16 handle, uint16 size_value, uint8 *value)
{
    gatt_gaa_media_char_id_t id = getCharIdFromCharHandle(handle);

    if (id < gaa_media_char_last)
    {
        uint16 i;

        for (i = 0; i < GATT_GAA_MEDIA_MAX_CLIENTS; ++i)
        {
            gatt_gaa_media_client_t *client = &gaa_media_local_server->client[i];
            uint16 cid = client->cid;

            GATT_GAA_MEDIA_SERVER_DEBUG_INFO(("- cid=0x%04X cfg[%u]=%u\n", cid, id, client->config[id]));

            if (cid && (client->config[id] & gatt_client_char_config_notifications_enabled))
            {
                GattManagerRemoteClientNotify(&gaa_media_local_server->lib_task, cid, handle, size_value, value);
            }
        }
    }
}

static void writeCharacteristicConfig(const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind, gatt_gaa_media_char_id_t id)
{
    gatt_gaa_media_client_t *client = gattGaaMediaServerFindClient(access_ind->cid);
    gatt_status_t status = gatt_status_unlikely_error;

    if (client)
    {
        if (id < gaa_media_char_last)
        {
            if (access_ind->size_value == GATT_CLIENT_CHAR_CONFIG_SIZE)
            {
                client->config[id] = MAKEWORD(access_ind->value[0], access_ind->value[1]);
                GATT_GAA_MEDIA_SERVER_DEBUG_INFO(("- cid=0x%04X cfg[%u]=%u\n", access_ind->cid, id, client->config[id]));
                status = gatt_status_success;
            }
            else
            {
                status = gatt_status_invalid_length;
            }
        }
        else
        {
            status = gatt_status_invalid_handle;
        }
    }
    
    sendGattManagerAccessResponse(access_ind->cid, access_ind->handle, status, 0, NULL);
}

static void readCharacteristicConfig(const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind, gatt_gaa_media_char_id_t id)
{
    gatt_gaa_media_client_t *client = gattGaaMediaServerFindClient(access_ind->cid);
    gatt_status_t status = gatt_status_unlikely_error;
    uint16 size_data = 0;
    uint8 data[GATT_CLIENT_CHAR_CONFIG_SIZE];

    if (client)
    {
        if (id < gaa_media_char_last)
        {
            data[0] = LOBYTE(client->config[id]);
            data[1] = HIBYTE(client->config[id]);
            size_data = GATT_CLIENT_CHAR_CONFIG_SIZE;
            status = gatt_status_success;
        }
        else
        {
            status = gatt_status_invalid_handle;
        }
    }
    
    sendGattManagerAccessResponse(access_ind->cid, access_ind->handle, status, size_data, data);
}

static void accessCharecteristicConfig(const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    gatt_gaa_media_char_id_t id = getCharIdFromConfigHandle(access_ind->handle);

    if (access_ind->flags & ATT_ACCESS_WRITE_COMPLETE)
    {
        writeCharacteristicConfig(access_ind, id);
    }
    else if (access_ind->flags & ATT_ACCESS_READ)
    {
        readCharacteristicConfig(access_ind, id);
    }
    else
    {
        sendGattManagerErrorAccessResponse(access_ind, gatt_status_request_not_supported);
    }
}

static gatt_status_t storeActiveApp(uint8 *active_app, uint16 size_active_app)
{
    gatt_status_t result = gatt_status_success;

    free(gaa_media_local_server->active_app);
    gaa_media_local_server->active_app = malloc(size_active_app);
    
    if (gaa_media_local_server->active_app)
    {
        memcpy(gaa_media_local_server->active_app, active_app, size_active_app);
        gaa_media_local_server->size_active_app = size_active_app;
    }
    else
    {
        gaa_media_local_server->size_active_app = 0;
        result = gatt_status_insufficient_resources;
    }

    return result;
}

static void writeGaaMediaCharecteristic(const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    gatt_status_t result = gatt_status_success;
    
    if(access_ind->size_value)
    {
        switch(access_ind->handle)
        {
            case HANDLE_GAA_MEDIA_ACTIVE_APP_CHAR:
                result = storeActiveApp(access_ind->value, access_ind->size_value);
                break;

            case HANDLE_GAA_MEDIA_COMMAND_CHAR:
            case HANDLE_GAA_MEDIA_STATUS_CHAR:
            case HANDLE_GAA_MEDIA_BROADCAST_CHAR:
                break;

            default:
                result = gatt_status_invalid_handle;
                break;
        }
    }

    if (result == gatt_status_success)
    {        
        sendGattManagerAccessResponse(access_ind->cid, access_ind->handle, result, 0, NULL);
        notifyClients(access_ind->handle, access_ind->size_value, access_ind->value);
    }
    else
    {
        sendGattManagerErrorAccessResponse(access_ind, result);
    }
}
 
static void readGaaMediaCharacteristic(const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->handle == HANDLE_GAA_MEDIA_ACTIVE_APP_CHAR)
    {
        sendGattManagerAccessResponse(
                    access_ind->cid,
                    access_ind->handle,
                    gatt_status_success,
                    gaa_media_local_server->size_active_app,
                    gaa_media_local_server->active_app);
    }
    else
    {
        sendGattManagerErrorAccessResponse(access_ind, gatt_status_read_not_permitted);
    }
}

static void accessGaaMediaCharacteristic(const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->flags & ATT_ACCESS_WRITE_COMPLETE)
    {
        writeGaaMediaCharecteristic(access_ind);
    }
    else if(access_ind->flags & ATT_ACCESS_READ)
    {
        readGaaMediaCharacteristic(access_ind);
    }
    else
    {
        sendGattManagerErrorAccessResponse(access_ind, gatt_status_request_not_supported);
    }
}

void handleGaaMediaServerAccess(const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    GATT_GAA_MEDIA_SERVER_DEBUG_INFO(("GAA_MEDIA access cid=0x%04X hdl=0x%04X flg=0x%02X=%c siz=%u",
                                access_ind->cid,
                                access_ind->handle,
                                access_ind->flags,
                                access_ind->flags & ATT_ACCESS_WRITE_COMPLETE ? 'W' :
                                access_ind->flags & ATT_ACCESS_READ ? 'R' : '?',
                                access_ind->size_value));

    if (access_ind->size_value)
    {
        uint16 i;
        PRINT((" val=0x"));

        for (i = 0; i < access_ind->size_value; ++i)
        {
            PRINT((" %02X", access_ind->value[i]));
        }
    }

    PRINT(("\n"));

    switch (access_ind->handle)
    {
        case HANDLE_GAA_MEDIA_SERVICE:
            accessGaaMediaService(access_ind);
            break;

        case HANDLE_GAA_MEDIA_ACTIVE_APP_CLIENT_C_CFG:
        case HANDLE_GAA_MEDIA_COMMAND_CLIENT_C_CFG:
        case HANDLE_GAA_MEDIA_STATUS_CLIENT_C_CFG:
        case HANDLE_GAA_MEDIA_BROADCAST_CLIENT_C_CFG:
            accessCharecteristicConfig(access_ind);
            break;
    
        case HANDLE_GAA_MEDIA_ACTIVE_APP_CHAR:
        case HANDLE_GAA_MEDIA_COMMAND_CHAR:
        case HANDLE_GAA_MEDIA_STATUS_CHAR:
        case HANDLE_GAA_MEDIA_BROADCAST_CHAR:
            accessGaaMediaCharacteristic(access_ind);
            break;
        
        default:
            sendGattManagerErrorAccessResponse(access_ind, gatt_status_invalid_handle);
            break;
    }
}
