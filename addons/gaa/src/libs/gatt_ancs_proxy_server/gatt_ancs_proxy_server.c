/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_ancs_proxy_server_init.c
@brief   Initialise ANCS proxy server
*/

#include "gatt_ancs_proxy_server.h"
#include "gatt_ancs_proxy_server_private.h"
#include "gatt_ancs_proxy_server_notify.h"
#include "gatt_ancs_proxy_server_msg_handler.h"
#include "gatt_ancs_proxy_server_db.h"
#include "gatt_ancs_proxy_server_ready.h"
#include "gatt_ancs_proxy_server_access.h"
#include <gatt_apple_notification_client.h>
#include <gatt_manager.h>
#include <byte_utils.h>
#include <string.h>

static GANCS_PROXY_SS ancs_proxy;

static void initProxyServer(Task app_task, uint16 start_handle)
{
    ancs_proxy.lib_task.handler = gattAncsProxyServerMsgHandler;
    ancs_proxy.start_handle = start_handle;
    ancs_proxy.app_task = app_task;
}

static bool registerWithGattManager(Task msg_handler, uint16 start_handle, uint16 end_handle)
{
    gatt_manager_server_registration_params_t registration_params;

    registration_params.task = msg_handler;
    registration_params.start_handle = start_handle;
    registration_params.end_handle = end_handle;

    return (GattManagerRegisterServer(&registration_params) == gatt_manager_status_success);
}

static void handleClientReadyStateUpdate(const GANCS *ancs, bool is_ready)
{
   if (is_ready)
       gattAncsProxyClientIsReady(&ancs_proxy, ancs);
   else
       gattAncsProxyClientIsDestroyed(&ancs_proxy, ancs);
}

bool GattAncsProxyServerInit(Task app_task, uint16 start_handle, uint16 end_handle)
{
    if (ancs_proxy.lib_task.handler)
    {
        DEBUG_PANIC(("ANCS PROXY: Library cannot be initialised more than once\n"));
        return FALSE;
    }

    initProxyServer(app_task, start_handle);

    if (registerWithGattManager(&ancs_proxy.lib_task, start_handle, end_handle) == FALSE)
    {
        DEBUG_PANIC(("ANCS PROXY: Failed to register with GATT manager\n"));
        return FALSE;
    }

    if (GattAncsAddReadyStateObserver(handleClientReadyStateUpdate) == FALSE)
    {
        DEBUG_PANIC(("ANCS PROXY: Failed to register proxy task with client\n"));
        return FALSE;
    }

    return TRUE;
}

void GattAncsProxyServerSendReadyCharNotification(uint16 cid)
{
    uint8 characteristic[ready_size];
    uint16 ready_char = gattAncsProxyGetReadyCharacteristic(cid);

    characteristic[0] = LOBYTE(ready_char);
    characteristic[1] = HIBYTE(ready_char);

    PRINT(("ANCS PROXY: Send Ready notification, cid [0x%04x], ready [0x%04x]\n", cid, ready_char));
    gattAncsProxySendNotification(&ancs_proxy, cid, HANDLE_ANCS_PROXY_READY_CHAR, ready_size, characteristic);
}

void GattAncsProxyServerReadReadyClientConfigResponse(uint16 cid, uint16 ready_c_cfg)
{
    PRINT(("ANCS PROXY: Read Ready config response, cid [0x%04x], ready_c_cfg [0x%04x]\n", cid, ready_c_cfg));
    gattAncsProxySendClientConfigReadResponse(&ancs_proxy, cid, HANDLE_ANCS_PROXY_READY_CLIENT_C_CFG, ready_c_cfg);
}
