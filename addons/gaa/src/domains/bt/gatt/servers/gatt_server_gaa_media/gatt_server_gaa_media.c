/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       gatt_server_gaa_media.c
\brief     Gaa media communication component
*/

#include "gaa_private.h"
#include <gatt_gaa_media_server.h>
#include <gatt_handler.h>
#include <logging.h>
#include <connection_manager.h>
#include <gatt_connect.h>
#include "gatt_server_gaa_media.h"

/*********************** Forward declarations ***********************/
static void gattServerGaaMedia_OnConnection(uint16 cid);
static void gattServerGaaMedia_OnDisconnection(uint16 cid);

static const gatt_connect_observer_callback_t connect_observer =
{
    .OnConnection = gattServerGaaMedia_OnConnection,
    .OnDisconnection = gattServerGaaMedia_OnDisconnection
};


/* Utility function to act on unsolicitated GATT connection indication */
static void gattServerGaaMedia_OnConnection(uint16 cid)
{
    DEBUG_LOG("gattServerGaaMedia_OnConnection cid=0x%04X", cid);

    GattGaaMediaServerConnect(cid);
}

/* Utility function to act on unsolicitated GATT disconnection indication */
static void gattServerGaaMedia_OnDisconnection(uint16 cid)
{
    DEBUG_LOG("gattServerGaaMedia_OnDisconnection cid=0x%04X", cid);

    GattGaaMediaServerDisconnect(cid);
}

/***********************************************************/
bool GattServerGaaMedia_Init(Task init_task)
{
    UNUSED(init_task);

    DEBUG_LOG("GattServerGaaMedia_Init");

    GattConnect_RegisterObserver(&connect_observer);

    return GattGaaMediaServerInit(GattHandler_GetGattStartHandle(gatt_server_gaa_media_server),
                                  GattHandler_GetGattEndHandle(gatt_server_gaa_media_server));
}
