/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       gatt_server_gaa_comm.c
\brief      Gaa communications component.
*/

#include "gaa_private.h"
#include <gatt_gaa_comm_server.h>
#include <gatt_handler.h>
#include <logging.h>
#include <connection_manager.h>
#include <gatt_connect.h>
#include "gatt_server_gaa_comm.h"

static void gattServerGaaComm_Hander(Task task, MessageId id, Message message);
static void gattServerGaaComm_OnConnection(uint16 cid);
static void gattServerGaaComm_OnDisconnection(uint16 cid);

static TaskData gaaCommTask = {gattServerGaaComm_Hander};

static const gatt_connect_observer_callback_t connect_observer =
{
    .OnConnection = gattServerGaaComm_OnConnection,
    .OnDisconnection = gattServerGaaComm_OnDisconnection
};

static void gattServerGaaComm_HandleConnectInd(GATT_GAA_COMM_SERVER_CONNECT_IND_T *ind)
{
    bool ok = Gaa_BleConnectInd(ind->channel, &ind->addr);

    GattGaaCommConnectRsp(ind->channel, ok);
}

static void gattServerGaaComm_OnConnection(uint16 cid)
{
    DEBUG_LOG("gattServerGaaComm_OnConnection cid=0x%04X", cid);
    
    if (cid == GattGaaCommServerGetCid())
    {
    }
}

static void gattServerGaaComm_OnDisconnection(uint16 cid)
{
    DEBUG_LOG("gattServerGaaComm_OnDisconnection cid=0x%04X", cid);
    
    if (cid == GattGaaCommServerGetCid())
    {
        Gaa_BleDisconnectCfm((uint16)gatt_gaa_channel_control);
        Gaa_BleDisconnectCfm((uint16)gatt_gaa_channel_audio);
        GattGaaCommDisconnect();
    }
}

static void gattServerGaaComm_HandleDisconnectInd(GATT_GAA_COMM_SERVER_DISCONNECT_IND_T *ind)
{
    Gaa_BleDisconnectCfm(ind->channel);
    GattGaaCommDisconnectRsp(ind->channel);
}

static void gattServerGaaComm_HandleWriteInd(GATT_GAA_COMM_SERVER_WRITE_IND_T *ind)
{
    gatt_gaa_channel_t channel = ind->channel;
    
    DEBUG_LOG("gattServerGaaComm_HandleWriteInd: ch=%d len=%d", channel, ind->length);
    Gaa_BleOnRxReady((uint16) channel, ind->value, ind->length);
}


static void gattServerGaaComm_Hander(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch(id)
    {
    case GATT_GAA_COMM_SERVER_CONNECT_IND:
        gattServerGaaComm_HandleConnectInd((GATT_GAA_COMM_SERVER_CONNECT_IND_T*) message);
        break;

    case GATT_GAA_COMM_SERVER_DISCONNECT_IND:
        gattServerGaaComm_HandleDisconnectInd((GATT_GAA_COMM_SERVER_DISCONNECT_IND_T*) message);
        break;

    case GATT_GAA_COMM_SERVER_WRITE_IND:
        gattServerGaaComm_HandleWriteInd((GATT_GAA_COMM_SERVER_WRITE_IND_T*) message);
        break;

    case GATT_GAA_COMM_TX_AVAILABLE:
        Gaa_BleTxAvailable();
        break;

    default:
        DEBUG_LOG("gaaCommHander: unhandled 0x%04X", id);
        break;
    }
}

bool GattServerGaaComm_Init(Task init_task)
{
    UNUSED(init_task);

    DEBUG_LOG("GattServerGaaComm_Init");

    GattConnect_RegisterObserver(&connect_observer);
    
    return GattGaaCommServerInit(
                &gaaCommTask,
                GattHandler_GetGattStartHandle(gatt_server_gaa_comm),
                GattHandler_GetGattEndHandle(gatt_server_gaa_comm));
}
