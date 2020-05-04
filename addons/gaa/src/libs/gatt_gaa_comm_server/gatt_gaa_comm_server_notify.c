/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_gaa_comm_server_notify.c
@brief   Handle notifications to the server
*/

#include <string.h>

#include <message.h>
#include <panic.h>
#include <sink.h>
#include <stream.h>

#include "gatt_gaa_comm_server.h"
#include "gatt_gaa_comm_server_access.h"
#include "gatt_gaa_comm_server_private.h"
#include "gatt_gaa_comm_server_db.h"

static void gaaCommNotifyHandler(Task task, MessageId id, Message message);
static const TaskData gaa_comm_notify_task = {gaaCommNotifyHandler};

static void gaaCommNotifyHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    switch(id)
    {
    case MESSAGE_MORE_SPACE:
        MessageSend(GattGaaCommGetAppTask(), GATT_GAA_COMM_TX_AVAILABLE, NULL);
        break;

    default:
        break;
    }
}


static uint16 getHandleForChannel(const GGAA_COMM_SS *gaaComm, gatt_gaa_channel_t channel)
{
    uint16 handle = INVALID_HANDLE;

    if(channel == gatt_gaa_channel_control)
        handle = gaaComm->start_handle + HANDLE_GAA_OUTGOING_CONTROL_CHANNEL_CHAR - 1;

    else if(channel == gatt_gaa_channel_audio)
        handle = gaaComm->start_handle + HANDLE_GAA_AUDIO_DATA_OUTPUT_CHANNEL_CHAR - 1;

    else
        Panic();

    return handle;
}

static void panicIfSinkValid(Sink sink)
{
    if(SinkIsValid(sink))
        Panic();
}

static gatt_gaa_tx_status_t transmitData(const GGAA_COMM_SS *gaaComm, uint16 handle, const uint8_t *data, uint32_t length)
{
    Sink sink = StreamAttServerSink(gaaComm->cid);
    uint16 sink_size = length + HANDLE_OFFSET;

    if(sink == NULL)
        return FALSE;

    if(SinkSlack(sink) >= sink_size)
    {
        uint16 offset;
        uint8 *sink_data = NULL;

        offset = SinkClaim(sink, sink_size);

        if(offset == INVALID_SINK)
        {
            panicIfSinkValid(sink);
            return tx_failed_sink_invalid;
        }

        sink_data = SinkMap(sink);

        if (sink_data == NULL)
        {
            panicIfSinkValid(sink);
            return tx_failed_sink_invalid;
        }
        else
        {
            sink_data[0] = handle & 0xFF;
            sink_data[1] = handle >> 8;
            memmove(&sink_data[HANDLE_OFFSET], data, length);
        }

        if (SinkFlush(sink, sink_size))
            return tx_success;
        else
        {
            panicIfSinkValid(sink);
            return tx_failed_sink_invalid;
        }
    }
    else
    {
        if (!SinkIsValid(sink))
            return tx_failed_sink_invalid;
        return tx_failed_no_space_available;
    }
}

gatt_gaa_tx_status_t GaaCommServerSendNotification(gatt_gaa_channel_t channel, const uint8 *data, uint32 length)
{
    GGAA_COMM_SS *server = GattGaaCommGetServer();
    uint16 handle = getHandleForChannel(server, channel);

    return transmitData(server, handle, data, length);
}

void GaaCommServerNotifyRegisterInterestSinkMsg(void)
{
    Sink sink = StreamAttServerSink(GattGaaCommServerGetCid());
    SinkConfigure(sink, VM_SOURCE_MESSAGES, VM_MESSAGES_SOME);
    MessageStreamTaskFromSink(sink, (Task)&gaa_comm_notify_task);
}
