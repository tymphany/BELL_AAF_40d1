/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_gaa_media_server_init.c
@brief   Initialise GAA Media Server
*/

#include "gatt_gaa_media_server.h"
#include "gatt_gaa_media_server_msg_handler.h"
#include "gatt_gaa_media_server_private.h"
#include <string.h>

static bool registerServerWithGattManager(uint16 start_handle, uint16 end_handle)
{
    gatt_manager_server_registration_params_t registration_params;

    registration_params.task = &gaa_media_local_server->lib_task;
    registration_params.start_handle = start_handle;
    registration_params.end_handle = end_handle;

    return (GattManagerRegisterServer(&registration_params) == gatt_manager_status_success);
}

bool GattGaaMediaServerInit(uint16 start_handle, uint16 end_handle)
{
    bool ok = FALSE;

    if (gaa_media_local_server == NULL)
    {
        gaa_media_local_server = PanicUnlessMalloc(sizeof *gaa_media_local_server);
        memset(gaa_media_local_server, 0, sizeof *gaa_media_local_server);
        gaa_media_local_server->lib_task.handler = GattGaaMediaServerMsgHandler;
        ok = registerServerWithGattManager(start_handle, end_handle);
    }

    return ok;
}
