/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file		gatt_server_ancs_proxy.c
\brief      Routines to handle messages sent from the GATT ANCS Proxy server.
*/
#include <logging.h>
#include <gatt.h>
#include <gatt_handler.h>
#include "gatt_server_ancs_proxy.h"
#include <gatt_ancs_proxy_server.h>
#include <gatt_connect.h>

#define DEBUG_GATT_ANCS_PROXY
#ifdef DEBUG_GATT_ANCS_PROXY
#define GATT_ANCS_PROXY_INFO(x) DEBUG(x)
#else
#define GATT_ANCS_PROXY_INFO(x) {}
#endif

#define ANCS_PROXY_TX_MTU_SIZE    (178)

#define NUMBER_GATT_ANCS_PROXY_SERVERS   1

/*! Structure holding the client data */
typedef struct
{
    /*! Client connection ID */
    uint16  cid;

    bool  ancs_proxy_ready;
}gattServerAncsClientData;

/*! Structure holding information for the application handling of GATT ANCS Proxy Server */
typedef struct
{
    /*! Client data associated with the server */
    gattServerAncsClientData client_data;
} gattServerAncsProxyInstanceInfo;

/*! Structure holding information for the application handling of GATT ANCS Proxy Server */
typedef struct
{
    /*! Task for handling battery related messages */
    TaskData                        task;

    /*! Instances of the ANCS proxy servers*/
    gattServerAncsProxyInstanceInfo   instance[NUMBER_GATT_ANCS_PROXY_SERVERS];
} gattServerAncsProxyData;

static void gattServerAncsProxy_MessageHandler(Task task, MessageId id, Message message);

static gattServerAncsProxyData gatt_server_ancs_proxy = {
    .task = {.handler = gattServerAncsProxy_MessageHandler},
    .instance = {0}
};

static uint8 getInstanceIndex(uint16 cid)
{
    uint8 index = 0;
    for(index; index < NUMBER_GATT_ANCS_PROXY_SERVERS; index++)
    {
        if(gatt_server_ancs_proxy.instance[index].client_data.cid == cid)
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
    if(index < NUMBER_GATT_ANCS_PROXY_SERVERS)
    {
        ready_c_cfg = gatt_server_ancs_proxy.instance[index].client_data.ancs_proxy_ready;
    }
    return ready_c_cfg;
}

static void handleReadReadyClientConfig(const GATT_ANCS_PROXY_SERVER_READ_READY_C_CFG_IND_T *ind)
{
    uint16 ready_c_cfg = getReadyClientConfig(ind->cid);
    GattAncsProxyServerReadReadyClientConfigResponse(ind->cid, ready_c_cfg);
}

static void handleWriteReadyClientConfig(const GATT_ANCS_PROXY_SERVER_WRITE_READY_C_CFG_IND_T *ind)
{
    uint8 index = getInstanceIndex(ind->cid);
    if(index < NUMBER_GATT_ANCS_PROXY_SERVERS)
    {
        gatt_server_ancs_proxy.instance[index].client_data.ancs_proxy_ready = ind->ready_c_cfg;
    }
}

static void handleReadyCharacteristicUpdate(const GATT_ANCS_PROXY_SERVER_READY_CHAR_UPDATE_IND_T *ind)
{
    uint16 ready_c_cfg = getReadyClientConfig(ind->cid);

    if (ready_c_cfg & gatt_client_char_config_notifications_enabled)
        GattAncsProxyServerSendReadyCharNotification(ind->cid);
}

static void gattServerAncsProxy_MessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    switch (id)
    {
        case GATT_ANCS_PROXY_SERVER_READ_READY_C_CFG_IND:
            handleReadReadyClientConfig(message);
            break;
        case GATT_ANCS_PROXY_SERVER_WRITE_READY_C_CFG_IND:
            handleWriteReadyClientConfig(message);
            break;
        case GATT_ANCS_PROXY_SERVER_READY_CHAR_UPDATE_IND:
            handleReadyCharacteristicUpdate(message);
            break;
    }
}

/*****************************************************************************/
bool GattServerAncsProxy_Init(Task init_task)
{
    UNUSED(init_task);

    GattConnect_UpdateMinAcceptableMtu(ANCS_PROXY_TX_MTU_SIZE);

    if (GattAncsProxyServerInit(&gatt_server_ancs_proxy.task,
                                GattHandler_GetGattStartHandle(gatt_server_gaa_ancs_proxy),
                                GattHandler_GetGattEndHandle(gatt_server_gaa_ancs_proxy)))
    {
        DEBUG_LOG("ANCS proxy: Server initialised");

        return TRUE;
    }

    DEBUG_LOG("ANCS proxy: Initialisation failed");
    return FALSE;
}
