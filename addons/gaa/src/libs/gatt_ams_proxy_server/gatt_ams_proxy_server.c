/* Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd. */

#include "gatt_ams_proxy_server.h"
#include "gatt_ams_proxy_server_db.h"
#include "gatt_ams_proxy_server_access.h"
#include "gatt_ams_proxy_server_notify.h"
#include "gatt_ams_proxy_server_msg_handler.h"
#include "gatt_ams_proxy_server_ready.h"
#include <gatt_ams_client.h>
#include <gatt_manager.h>
#include <byte_utils.h>
#include <string.h>

static GAMS_PROXY_SS ams_proxy;

static void initProxyServer(Task app_task, uint16 start_handle)
{
    ams_proxy.lib_task.handler = gattAmsProxyMsgHandler;
    ams_proxy.app_task = app_task;
    ams_proxy.start_handle = start_handle;
}

static bool registerWithGattManager(Task msg_handler, uint16 start_handle, uint16 end_handle)
{
    gatt_manager_server_registration_params_t registration_params;
    registration_params.task = msg_handler;
    registration_params.start_handle = start_handle;
    registration_params.end_handle = end_handle;
    return (GattManagerRegisterServer(&registration_params) == gatt_manager_status_success);
}

static void handleClientReadyStateUpdate(const GAMS *ams, bool is_ready)
{
   if (is_ready)
       gattAmsProxyClientIsReady(&ams_proxy, ams);
   else
       gattAmsProxyClientIsDestroyed(&ams_proxy, ams);
}

bool GattAmsProxyServerInit(Task app_task, uint16 start_handle, uint16 end_handle)
{
    if (ams_proxy.lib_task.handler)
    {
        DEBUG_PANIC(("AMS PROXY: Library cannot be initialised more than once\n"));
        return FALSE;
    }

    initProxyServer(app_task, start_handle);

    if (registerWithGattManager(&ams_proxy.lib_task, start_handle, end_handle) == FALSE)
    {
        DEBUG_PANIC(("AMS PROXY: Failed to register with GATT manager\n"));
        return FALSE;
    }

    if (GattAmsAddReadyStateObserver(handleClientReadyStateUpdate) == FALSE)
    {
        DEBUG_PANIC(("AMS PROXY: Failed to register proxy task with client\n"));
        return FALSE;
    }

    return TRUE;
}

void GattAmsProxyServerSendReadyCharNotification(uint16 cid)
{
    uint8 characteristic[ready_size];
    uint16 ready_char = gattAmsProxyGetReadyCharacteristic(cid);

    characteristic[0] = LOBYTE(ready_char);
    characteristic[1] = HIBYTE(ready_char);

    PRINT(("AMS PROXY: Send Ready notification, cid [0x%04x], ready [0x%04x]\n", cid, ready_char));
    gattAmsProxySendNotification(&ams_proxy, cid, HANDLE_AMS_PROXY_READY_CHAR, ready_size, characteristic);
}

void GattAmsProxyServerReadReadyClientConfigResponse(uint16 cid, uint16 ready_c_cfg)
{
    PRINT(("AMS PROXY: Read Ready config response, cid [0x%04x], ready_c_cfg [0x%04x]\n", cid, ready_c_cfg));
    gattAmsProxySendClientConfigReadResponse(&ams_proxy, cid, HANDLE_AMS_PROXY_READY_CLIENT_C_CFG, ready_c_cfg);
}
