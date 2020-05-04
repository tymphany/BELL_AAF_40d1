/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_gaa_comm_server_access.c
@brief   Server read/write access
*/

#include "gatt_gaa_comm_server_access.h"

#include "gatt_gaa_comm_server_db.h"

#include <vm.h>
#include <connection.h>
#include <gatt.h>
#include <string.h>
#include <logging.h>
#include <bdaddr.h>
#include "gatt_gaa_comm_server_private.h"

static void readClientDetails(void)
{
    GGAA_COMM_SS *server = GattGaaCommGetServer();
    tp_bdaddr client_tpaddr;
    tp_bdaddr public_tpaddr;
    
    memset(&client_tpaddr, 0, sizeof client_tpaddr);
    memset(&public_tpaddr, 0, sizeof public_tpaddr);

    if(VmGetBdAddrtFromCid(GattGaaCommServerGetCid(), &client_tpaddr))
    {
        if (client_tpaddr.taddr.type == TYPED_BDADDR_RANDOM)
        {
            VmGetPublicAddress(&client_tpaddr, &public_tpaddr);
        }
        else
        {
            memcpy(&public_tpaddr, &client_tpaddr, sizeof(tp_bdaddr));
        }
        if(BdaddrIsZero(&public_tpaddr.taddr.addr))
        {
            GATT_GAA_COMM_SERVER_DEBUG_PANIC("Unable to retrieve client details for this connection");
        }
        server->addr = public_tpaddr.taddr.addr;
    }
}

static void sendGaaCommServerAccessRsp(uint16 cid, uint16 handle, uint16 result, uint16 size_value, const uint8 *value)
{
    if (!GattManagerServerAccessResponse(GattGaaCommGetLibTask(), cid, handle, result, size_value, value))
    {
        GATT_GAA_COMM_SERVER_DEBUG_PANIC("sendGaaCommServerAccessRsp; response failed");
    }
}

static void sendGaaCommServerAccessErrorRsp(const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind, uint16 error)
{
    sendGaaCommServerAccessRsp(access_ind->cid, access_ind->handle, error, 0, NULL);
}

static void gaaCommServiceAccess(const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        sendGaaCommServerAccessRsp(access_ind->cid, access_ind->handle, gatt_status_success, 0, NULL);
    }
    else if (access_ind->flags & ATT_ACCESS_WRITE)
    {
        sendGaaCommServerAccessErrorRsp(access_ind, gatt_status_write_not_permitted);
    }
    else
    {
        sendGaaCommServerAccessErrorRsp(access_ind, gatt_status_request_not_supported);
    }
}

static void sendConnectInd(gatt_gaa_channel_t channel)
{
    MAKE_GATT_GAA_COMM_SERVER_MESSAGE(GATT_GAA_COMM_SERVER_CONNECT_IND);

    message->addr = GattGaaCommGetServer()->addr;
    message->channel = channel;
    MessageSend(GattGaaCommGetAppTask(), GATT_GAA_COMM_SERVER_CONNECT_IND, message);
}

static void sendDisonnectInd(gatt_gaa_channel_t channel)
{
    MAKE_GATT_GAA_COMM_SERVER_MESSAGE(GATT_GAA_COMM_SERVER_DISCONNECT_IND);

    message->channel = channel;
    MessageSend(GattGaaCommGetAppTask(), GATT_GAA_COMM_SERVER_DISCONNECT_IND, message);
}

static gatt_status_t setClientEnable(uint16 handle, uint8 value)
{
    GGAA_COMM_SS *server = GattGaaCommGetServer();
    gatt_status_t status = gatt_status_success;
    
    if (value)
    {
        if (handle == HANDLE_GAA_OUTGOING_CONTROL_CHANNEL_CHAR_CLIENT_C_CFG)
        {
            if (server->cc_control == 0)
            {
                server->cc_control = value;
                readClientDetails();
                sendConnectInd(gatt_gaa_channel_control);
                status = gatt_status_busy;
            }
        }
        else /* must be HANDLE_GAA_AUDIO_DATA_OUTPUT_CHANNEL_CHAR_C_CFG */
        {
            if (server->cc_audio == 0)
            {
                server->cc_audio = value;
                readClientDetails();
                sendConnectInd(gatt_gaa_channel_audio);
                status = gatt_status_busy;
            }
        }
    }
    else
    {
        if (handle == HANDLE_GAA_OUTGOING_CONTROL_CHANNEL_CHAR_CLIENT_C_CFG)
        {
            if (server->cc_control != 0)
            {
                sendDisonnectInd(gatt_gaa_channel_control);
                status = gatt_status_busy;
            }
        }
        else /* must be HANDLE_GAA_AUDIO_DATA_OUTPUT_CHANNEL_CHAR_C_CFG */
        {
            if (server->cc_audio != 0)
            {
                sendDisonnectInd(gatt_gaa_channel_audio);
                status = gatt_status_busy;
            }
        }
    }
       
    return status;
}

static gatt_status_t writeConfig(uint16 cid, uint16 handle, uint8 value)
{
    GGAA_COMM_SS *server = GattGaaCommGetServer();
    gatt_status_t status = gatt_status_success;

    GATT_GAA_COMM_SERVER_DEBUG_INFO("conf %d/%d %d = %d", cid, GattGaaCommServerGetCid(), handle, value);
    
    if (server->cid == INVALID_CID)
    {
        server->cid = cid;
    }

    if (cid == server->cid)
    {
        status = setClientEnable(handle, value);
    }
    else
    {
        status = gatt_status_insufficient_resources;
    }

    return status;
}

static void gaaCommCharacteristicConfigAccess(const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    gatt_status_t result;
    
    if (access_ind->flags & ATT_ACCESS_WRITE_COMPLETE)
    {
        if (access_ind->size_value == GATT_CLIENT_CHAR_CONFIG_SIZE)
        {
            result = writeConfig(access_ind->cid, access_ind->handle, access_ind->value[0]);
        }
        else
        {
            result = gatt_status_invalid_length;
        }
    }
    else if (access_ind->flags & ATT_ACCESS_READ)
    {
        result = gatt_status_read_not_permitted;
    }
    else
    {
        result = gatt_status_invalid_pdu;
    }
    
    if (result != gatt_status_busy)
    {
        sendGaaCommServerAccessRsp(access_ind->cid, access_ind->handle, result, 0, NULL);
    }
}

static gatt_gaa_channel_t handleForChannel(gatt_gaa_channel_t channel)
{
    return channel == gatt_gaa_channel_control ?
        HANDLE_GAA_OUTGOING_CONTROL_CHANNEL_CHAR_CLIENT_C_CFG :
        HANDLE_GAA_AUDIO_DATA_OUTPUT_CHANNEL_CHAR_C_CFG;
}

static void gaaCommCharacteristicAccess(const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    uint16 result = gatt_status_success;
    uint16 size_value = 0;

    GATT_GAA_COMM_SERVER_DEBUG_INFO("gaaCommCharacteristicAccess hdl=0x%04X, flg=0x%04X val=0x%02X", access_ind->handle, access_ind->flags , access_ind->value[0]);

    if (access_ind->flags & ATT_ACCESS_WRITE)
    {
        if(access_ind->size_value)
        {
            if (access_ind->handle == HANDLE_GAA_INCOMING_CONTROL_CHANNEL_CHAR)
            {
                MAKE_GATT_GAA_COMM_SERVER_MESSAGE_WITH_LEN(GATT_GAA_COMM_SERVER_WRITE_IND, access_ind->size_value);

                message->cid       = access_ind->cid;
                message->handle    = access_ind->handle;
                message->length    = access_ind->size_value;
                message->channel   = gatt_gaa_channel_control;
                memcpy(message->value, access_ind->value, access_ind->size_value);

                MessageSend(GattGaaCommGetAppTask(), GATT_GAA_COMM_SERVER_WRITE_IND, message);
            }
            else
            {
                result = gatt_status_invalid_handle;
            }
        }            
    }
    else
    {
        result = gatt_status_invalid_pdu;
    }

    sendGaaCommServerAccessRsp(access_ind->cid, access_ind->handle, result, size_value, NULL);
}

static bool channelIsOpen(gatt_gaa_channel_t channel)
{
    GGAA_COMM_SS *server = GattGaaCommGetServer();

    return channel == gatt_gaa_channel_control ?
        server->cc_control != 0 :
        server->cc_audio != 0;
}

static void closeChannel(gatt_gaa_channel_t channel)
{
    GGAA_COMM_SS *server = GattGaaCommGetServer();

    if (channel == gatt_gaa_channel_control)
    {
        server->cc_control = 0;
    }
    else /* must be gatt_gaa_channel_audio */
    {
        server->cc_audio = 0;
    }
    
    if (server->cc_control == 0 && server->cc_audio == 0)
    {
        server->cid = INVALID_CID;
    }
}

static void connectChannel(gatt_gaa_channel_t channel, bool ok)
{
    gatt_status_t result;
    
    if (ok)
    {
        GaaCommServerNotifyRegisterInterestSinkMsg();
        result = gatt_status_success;
    }
    else
    {
        result = gatt_status_insufficient_resources;
        closeChannel(channel);
    }
    
    DEBUG_LOG("connectChannel ch=%d ok=%d, RESULT = %d", channel, ok, result);

    sendGaaCommServerAccessRsp(
        GattGaaCommServerGetCid(),
        handleForChannel(channel),
        result, 
        0, 
        NULL);
}

static void disconnectChannel(gatt_gaa_channel_t channel)
{
    if (channelIsOpen(channel))
    {
        sendGaaCommServerAccessRsp(
            GattGaaCommServerGetCid(),
            handleForChannel(channel),
            gatt_status_success, 
            0, 
            NULL);
            
        closeChannel(channel);
    }
}

void handleGaaCommServerAccess(const GATT_MANAGER_SERVER_ACCESS_IND_T *ind)
{
    GATT_GAA_COMM_SERVER_DEBUG_INFO("handleGaaCommServerAccess cid=0x%04X hdl=0x%04X flg=%c%c%c%c",
                                      ind->cid,
                                      ind->handle,
                                      ind->flags & ATT_ACCESS_PERMISSION       ? 'p' : '-',
                                      ind->flags & ATT_ACCESS_WRITE_COMPLETE   ? 'c' : '-',
                                      ind->flags & ATT_ACCESS_WRITE            ? 'w' : '-',
                                      ind->flags & ATT_ACCESS_READ             ? 'r' : '-');

    switch (ind->handle)
    {
        case HANDLE_GAA_COMM_SERVICE:
            gaaCommServiceAccess(ind);
            break;

        case HANDLE_GAA_OUTGOING_CONTROL_CHANNEL_CHAR_CLIENT_C_CFG:
        case HANDLE_GAA_AUDIO_DATA_OUTPUT_CHANNEL_CHAR_C_CFG:
            gaaCommCharacteristicConfigAccess(ind);
            break;

        case HANDLE_GAA_INCOMING_CONTROL_CHANNEL_CHAR:
            gaaCommCharacteristicAccess(ind);
            break;

        default:
            sendGaaCommServerAccessErrorRsp(ind, gatt_status_invalid_handle);
            break;
    }
}

void GattGaaCommConnectRsp(gatt_gaa_channel_t channel, bool ok)
{
    if ((channel == gatt_gaa_channel_control || channel == gatt_gaa_channel_audio))
    {
        connectChannel(channel, ok);
    }
}

void GattGaaCommDisconnectRsp(gatt_gaa_channel_t channel)
{
    if ((channel == gatt_gaa_channel_control || channel == gatt_gaa_channel_audio))
    {
        disconnectChannel(channel);
    }
}

void GattGaaCommDisconnect(void)
{
    disconnectChannel(gatt_gaa_channel_control);
    disconnectChannel(gatt_gaa_channel_audio);
}
