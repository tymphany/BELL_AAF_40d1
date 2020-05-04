/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_ancs_proxy_server_access.c
@brief   Server read/write access
*/

#include "gatt_ancs_proxy_server_access.h"
#include "gatt_ancs_proxy_server_external_msg_send.h"
#include "gatt_ancs_proxy_server.h"
#include "gatt_ancs_proxy_server_db.h"
#include "gatt_ancs_proxy_server_client_list.h"
#include "gatt_ancs_proxy_server_ready.h"
#include <gatt_apple_notification_client.h>
#include <byte_utils.h>

static void sendAccessResponseToGattManager(Task task, uint16 cid, uint16 handle, gatt_status_t result_status, uint16 size_value, const uint8 *value)
{
    if (!GattManagerServerAccessResponse(task, cid, handle, result_status, size_value, value))
        DEBUG_PANIC(("ANCS PROXY: GATT manager access response failed, handle [0x%04x], cid [0x%04x]\n", handle, cid));
}

static void sendAccessErrorResponseToGattManager(GANCS_PROXY_SS *ancs_proxy, uint16 cid, uint16 handle, gatt_status_t error_status)
{
    sendAccessResponseToGattManager(&ancs_proxy->lib_task, cid, handle, error_status, 0, NULL);
}

static void sendReadSuccessResponseToGattManager(GANCS_PROXY_SS *ancs_proxy, uint16 cid, uint16 handle, uint16 size_value, const uint8 *value)
{
    sendAccessResponseToGattManager(&ancs_proxy->lib_task, cid, handle, gatt_status_success, size_value, value);
}

static void sendWriteSuccessResponseToGattManager(GANCS_PROXY_SS *ancs_proxy, uint16 cid, uint16 handle)
{
    sendAccessErrorResponseToGattManager(ancs_proxy, cid, handle, gatt_status_success);
}

static void readCharacteristicConfig(GANCS_PROXY_SS *ancs_proxy, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->handle == HANDLE_ANCS_PROXY_READY_CLIENT_C_CFG)
    {
        PRINT(("ANCS PROXY: Read Ready config\n"));
        /* Send GATT manager our response when the application calls GattAncsProxyServerReadReadyClientConfigResponse() */
        gattAncsProxySendReadReadyConfigInd(ancs_proxy, access_ind->cid);
    }
    else
    {
        /* Assuming all other configs are WRITE (which is probably wrong) */
        PRINT(("ANCS PROXY: Read characteristic config not handled!!! Handle [0x%04x]\n", access_ind->handle));
        sendAccessErrorResponseToGattManager(ancs_proxy, access_ind->cid, access_ind->handle, gatt_status_read_not_permitted);
    }
}

static void writeReadyConfig(GANCS_PROXY_SS *ancs_proxy, uint16 cid, uint16 ready_c_cfg)
{
    PRINT(("ANCS PROXY: Write Ready config, ready_c_cfg [0x%04x]\n", ready_c_cfg));

    gattAncsProxySendWriteReadyConfigInd(ancs_proxy, cid, ready_c_cfg);

    /* It is the applications responsibility to make sure the ready config value is written, respond to GATT manager now */
    sendWriteSuccessResponseToGattManager(ancs_proxy, cid, HANDLE_ANCS_PROXY_READY_CLIENT_C_CFG);

    if (ready_c_cfg & gatt_client_char_config_notifications_enabled)
        GattAncsProxyServerSendReadyCharNotification(cid);
}

static void writeAncsCharacteristicConfig(GANCS_PROXY_SS *ancs_proxy, uint16 cid, uint16 handle, uint16 client_config)
{
    const GANCS *client = gattAncsProxyGetClient(cid);
    bool notification_enable;

    if (client == NULL)
    {
        PRINT(("ANCS PROXY: Write characteristic config, client not found!!!\n"));
        sendAccessErrorResponseToGattManager(ancs_proxy, cid, handle, gatt_status_failure);
        return;
    }

    if (client_config & gatt_client_char_config_notifications_enabled)
        notification_enable = TRUE;
    else
        notification_enable = FALSE;

    switch (handle)
    {
        case HANDLE_ANCS_PROXY_NOTIFICATION_SOURCE_CHAR_CLIENT_C_CFG:
            PRINT(("ANCS PROXY: Write Notification Source config, notify = %d\n", notification_enable));
            /* Will send GATT manager the response when the client response is received */
            GattAncsSetNotificationSourceNotificationEnableRequest(&ancs_proxy->lib_task, client, notification_enable, ANCS_NO_CATEGORY);
            break;

        case HANDLE_ANCS_PROXY_DATA_SOURCE_CHAR_CLIENT_C_CFG:
            PRINT(("ANCS PROXY: Write Data Source config, notify = %d\n", notification_enable));
            /* Will send GATT manager the response when the client response is received */
            GattAncsSetDataSourceNotificationEnableRequest(&ancs_proxy->lib_task, client, notification_enable);
            break;

        default:
            PRINT(("ANCS PROXY: Write characteristic config, unknown handle [0x%04x]!!!\n", handle));
            sendAccessErrorResponseToGattManager(ancs_proxy, cid, handle, gatt_status_write_not_permitted);
            break;
    }
}

static void writeCharacteristicConfig(GANCS_PROXY_SS *ancs_proxy, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->size_value != GATT_CLIENT_CHAR_CONFIG_SIZE)
    {
        PRINT(("ANCS PROXY: Write characteristic config invalid length!!! Handle [0x%04x], length [%d]\n", access_ind->handle, access_ind->size_value));
        sendAccessErrorResponseToGattManager(ancs_proxy, access_ind->cid, access_ind->handle, gatt_status_invalid_length);
    }
    else
    {
        uint16 client_config = MAKEWORD(access_ind->value[0], access_ind->value[1]);

        if (access_ind->handle == HANDLE_ANCS_PROXY_READY_CLIENT_C_CFG)
            writeReadyConfig(ancs_proxy, access_ind->cid, client_config);
        else
            writeAncsCharacteristicConfig(ancs_proxy, access_ind->cid, access_ind->handle, client_config);
    }
}

static void readReadyCharacteristic(GANCS_PROXY_SS *ancs_proxy, uint16 cid)
{
    uint8 characteristic[ready_size];
    uint16 ready_char = gattAncsProxyGetReadyCharacteristic(cid);

    characteristic[0] = LOBYTE(ready_char);
    characteristic[1] = HIBYTE(ready_char);

    PRINT(("ANCS PROXY: Read Ready characteristic, value [0x%04x]\n", ready_char));
    sendReadSuccessResponseToGattManager(ancs_proxy, cid, HANDLE_ANCS_PROXY_READY_CHAR, ready_size, characteristic);
}

static void readCharacteristic(GANCS_PROXY_SS *ancs_proxy, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->handle == HANDLE_ANCS_PROXY_READY_CHAR)
        readReadyCharacteristic(ancs_proxy, access_ind->cid);
    else
        sendAccessErrorResponseToGattManager(ancs_proxy, access_ind->cid, access_ind->handle, gatt_status_read_not_permitted);
}

static void writeCharacteristic(GANCS_PROXY_SS *ancs_proxy, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    const GANCS *client = gattAncsProxyGetClient(access_ind->cid);

    if (client == NULL)
    {
        PRINT(("ANCS PROXY: Write characteristic, client not found!!!\n"));
        sendAccessErrorResponseToGattManager(ancs_proxy, access_ind->cid, access_ind->handle, gatt_status_failure);
        return;
    }

    switch (access_ind->handle)
    {
        case HANDLE_ANCS_PROXY_CONTROL_POINT_CHAR:
            PRINT(("ANCS PROXY: Write Control Point\n"));
            /* Will send GATT manager the response when the client response is received */
            GattAncsWriteControlPoint(&ancs_proxy->lib_task, client, access_ind->value, access_ind->size_value);
            break;

        default:
            PRINT(("ANCS PROXY: Write characteristic, unknown handle [0x%04x]!!!\n", access_ind->handle));
            sendAccessErrorResponseToGattManager(ancs_proxy, access_ind->cid, access_ind->handle, gatt_status_write_not_permitted);
            break;
    }
}

static void accessAncsProxyService(GANCS_PROXY_SS *ancs_proxy, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    PRINT(("ANCS PROXY: Service attribute access\n"));

    if (access_ind->flags & ATT_ACCESS_READ)
        sendReadSuccessResponseToGattManager(ancs_proxy, access_ind->cid, access_ind->handle, 0, NULL);
    else if (access_ind->flags & ATT_ACCESS_WRITE)
        sendAccessErrorResponseToGattManager(ancs_proxy, access_ind->cid, access_ind->handle, gatt_status_write_not_permitted);
    else
        sendAccessErrorResponseToGattManager(ancs_proxy, access_ind->cid, access_ind->handle, gatt_status_request_not_supported);
}

static void accessCharacteristicConfig(GANCS_PROXY_SS *ancs_proxy, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->flags & ATT_ACCESS_READ)
        readCharacteristicConfig(ancs_proxy, access_ind);
    else if (access_ind->flags & ATT_ACCESS_WRITE)
        writeCharacteristicConfig(ancs_proxy, access_ind);
    else
        sendAccessErrorResponseToGattManager(ancs_proxy, access_ind->cid, access_ind->handle, gatt_status_request_not_supported);
}

static void accessCharacteristic(GANCS_PROXY_SS *ancs_proxy, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->flags & ATT_ACCESS_READ)
        readCharacteristic(ancs_proxy, access_ind);
    else if (access_ind->flags & ATT_ACCESS_WRITE)
        writeCharacteristic(ancs_proxy, access_ind);
    else
        sendAccessErrorResponseToGattManager(ancs_proxy, access_ind->cid, access_ind->handle, gatt_status_request_not_supported);
}

void gattAncsProxyHandleServerAccess(GANCS_PROXY_SS *ancs_proxy, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    PRINT(("ANCS PROXY: Client access with cid [0x%04x]\n", access_ind->cid));

    switch (access_ind->handle)
    {
        case HANDLE_ANCS_PROXY_SERVICE:
            accessAncsProxyService(ancs_proxy, access_ind);
            break;

        case HANDLE_ANCS_PROXY_NOTIFICATION_SOURCE_CHAR_CLIENT_C_CFG:
        case HANDLE_ANCS_PROXY_DATA_SOURCE_CHAR_CLIENT_C_CFG:
        case HANDLE_ANCS_PROXY_READY_CLIENT_C_CFG:
            accessCharacteristicConfig(ancs_proxy, access_ind);
            break;

        case HANDLE_ANCS_PROXY_CONTROL_POINT_CHAR:
        case HANDLE_ANCS_PROXY_READY_CHAR:
            accessCharacteristic(ancs_proxy, access_ind);
            break;

        default:
            sendAccessErrorResponseToGattManager(ancs_proxy, access_ind->cid, access_ind->handle, gatt_status_invalid_handle);
            break;
    }
}

void gattAncsProxyServerSendAccessResponse(GANCS_PROXY_SS *ancs_proxy, uint16 cid, uint16 handle, gatt_status_t status, uint16 value_size, const uint8 *value)
{
	sendAccessResponseToGattManager(&ancs_proxy->lib_task, cid, handle, status, value_size, value);
}

void gattAncsProxySendClientConfigReadResponse(GANCS_PROXY_SS *ancs_proxy, uint16 cid, uint16 handle, uint16 client_config)
{
    uint8 config_resp[GATT_CLIENT_CHAR_CONFIG_SIZE];

    config_resp[0] = LOBYTE(client_config);
    config_resp[1] = HIBYTE(client_config);

    sendReadSuccessResponseToGattManager(ancs_proxy, cid, handle, GATT_CLIENT_CHAR_CONFIG_SIZE, config_resp);
}
