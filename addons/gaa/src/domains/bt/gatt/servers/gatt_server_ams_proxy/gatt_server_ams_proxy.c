/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       gatt_server_ams_proxy.c
\brief      Routines to handle messages sent from the GATT AMS Server Task.
*/


#include <logging.h>
#include <gatt.h>
#include <gatt_handler.h>
#include "gatt_server_ams_proxy.h"
#include <gatt_ams_proxy_server.h>
#include <gatt_ams_client.h>

#ifdef DEBUG_GATT_AMS_PROXY
#define GATT_AMS_PROXY_INFO(x) DEBUG(("GATT AMS Proxy: "##x))
#else
#define GATT_AMS_PROXY_INFO(x) {}
#endif

#define NUMBER_GATT_AMS_PROXY_SERVERS   1

/*! Structure holding the client data */
typedef struct
{
    /*! Client connection ID */
    uint16  cid;

    bool  ams_proxy_ready;
}gattServerAmsClientData;

/*! Structure holding information for the application handling of GATT ANCS Proxy Server */
typedef struct
{
    /*! Client data associated with the server */
    gattServerAmsClientData client_data;
} gattServerAmsProxyInstanceInfo;

/*! Structure holding information for the application handling of GATT ANCS Proxy Server */
typedef struct
{
    /*! Task for handling battery related messages */
    TaskData                        task;

    /*! Instances of the ANCS proxy servers*/
    gattServerAmsProxyInstanceInfo   instance[NUMBER_GATT_AMS_PROXY_SERVERS];
} gattServerAmsProxyData;

static void gattServerAmsProxy_MessageHandler(Task task, MessageId id, Message message);

static gattServerAmsProxyData gatt_server_ams_proxy = {
    .task = {.handler = gattServerAmsProxy_MessageHandler},
    .instance = {0}
};

static uint8 getInstanceIndex(uint16 cid)
{
    uint8 index = 0;
    for(index; index < NUMBER_GATT_AMS_PROXY_SERVERS; index++)
    {
        if(gatt_server_ams_proxy.instance[index].client_data.cid == cid)
        {
            break;
        }
    }

    return index;
}
static uint16 getReadyClientConfig(uint16 cid)
{
    uint16 ready_c_cfg = 0;
    uint8 index = getInstanceIndex(cid);
    if(index < NUMBER_GATT_AMS_PROXY_SERVERS)
    {
        ready_c_cfg = gatt_server_ams_proxy.instance[index].client_data.ams_proxy_ready;
    }
    return ready_c_cfg;
}

static void handleReadReadyClientConfig(const GATT_AMS_PROXY_SERVER_READ_READY_C_CFG_IND_T *ind)
{
    uint16 ready_c_cfg = getReadyClientConfig(ind->cid);
    GattAmsProxyServerReadReadyClientConfigResponse(ind->cid, ready_c_cfg);
}

static void handleWriteReadyClientConfig(const GATT_AMS_PROXY_SERVER_WRITE_READY_C_CFG_IND_T *ind)
{
    uint8 index = getInstanceIndex(ind->cid);
    if(index < NUMBER_GATT_AMS_PROXY_SERVERS)
    {
        gatt_server_ams_proxy.instance[index].client_data.ams_proxy_ready = ind->ready_c_cfg;
    }
}

static void handleReadyCharacteristicUpdate(const GATT_AMS_PROXY_SERVER_READY_CHAR_UPDATE_IND_T *ind)
{
    uint16 ready_c_cfg = getReadyClientConfig(ind->cid);
    if (ready_c_cfg & gatt_client_char_config_notifications_enabled)
        GattAmsProxyServerSendReadyCharNotification(ind->cid);
}

static void gattServerAmsProxy_MessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
        case GATT_AMS_PROXY_SERVER_READ_READY_C_CFG_IND:
            handleReadReadyClientConfig(message);
            break;
        case GATT_AMS_PROXY_SERVER_WRITE_READY_C_CFG_IND:
            handleWriteReadyClientConfig(message);
            break;
        case GATT_AMS_PROXY_SERVER_READY_CHAR_UPDATE_IND:
            handleReadyCharacteristicUpdate(message);
            break;
    }
}

bool GattServerAmsProxy_Init(Task init_task)
{
    /* !The server is not instance based and self-manages the memory it needs */
    UNUSED(init_task);

    if (GattAmsProxyServerInit(&gatt_server_ams_proxy.task,
                               GattHandler_GetGattStartHandle(gatt_server_gaa_ams_proxy),
                               GattHandler_GetGattEndHandle(gatt_server_gaa_ams_proxy)))
    {
        DEBUG_LOG("GATT Ams Proxy Server initialised");
        return TRUE;
    }

    DEBUG_LOG("GATT AMA Proxy init failed");
    return FALSE;
}
