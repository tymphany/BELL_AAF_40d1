/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_gaa_comm_server_init.c
@brief   Initialise GAA COMM Server
*/
#include "gatt_gaa_comm_server.h"
#include "gatt_gaa_comm_server_msg_handler.h"
#include "gatt_gaa_comm_server_private.h"
#include "gatt_gaa_comm_server_db.h"


static TaskData gaaCommTaskData = {GattGaaCommServerMsgHandler};
static GGAA_COMM_SS gaaCommLocalServer;

GGAA_COMM_SS * GattGaaCommGetServer(void)
{
    return &gaaCommLocalServer;
}

Task GattGaaCommGetAppTask(void)
{
    return gaaCommLocalServer.app_task;
}

Task GattGaaCommGetLibTask(void)
{
    return gaaCommLocalServer.lib_task;
}

bool GattGaaCommServerInit(
                           Task app_task,
                           uint16 start_handle,
                           uint16 end_handle)
{
    gatt_manager_server_registration_params_t registration_params;

    if (app_task == NULL)
    {
        GATT_GAA_COMM_SERVER_DEBUG_PANIC("GattGaaCommServerInit: invalid parameters");
        return FALSE;
    }
    
    /* Set up library handler for external messages */
    gaaCommLocalServer.lib_task = &gaaCommTaskData;
        
    /* Store the Task function parameter. All library messages need to be sent here */
    gaaCommLocalServer.app_task = app_task;

    /* Setup data required for GAA COMM Service to be registered with the GATT Manager */
    registration_params.task = gaaCommLocalServer.lib_task;
    registration_params.start_handle = start_handle;
    registration_params.end_handle = end_handle;

    gaaCommLocalServer.start_handle = start_handle;
    gaaCommLocalServer.cid = INVALID_CID;

    /* Register with the GATT Manager and verify the result */
    return GattManagerRegisterServer(&registration_params) == gatt_manager_status_success;
}

uint16 GattGaaCommServerGetCid(void)
{
    return gaaCommLocalServer.cid;
}

uint16 GattGaaCommServerGetMtu(void)
{
    uint16 mtu = GattGetMaxTxDataLength(gaaCommLocalServer.cid);
    GATT_GAA_COMM_SERVER_DEBUG_INFO("GattGaaCommServerGetMtu %u", mtu);
    return mtu;
}
