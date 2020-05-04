/*!
\copyright  Copyright (c) 2018-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation for GAA Bluetooth APIs
*/

#include <connection_no_ble.h>
#include <sink.h>
#include <source.h>
#include <stream.h>
#include <connection_manager.h>
#include <hfp.h>
#include <hfp_profile.h>
#include <logging.h>
#include "gsound_target.h"
#include "gaa_private.h"
#include "gaa_config.h"
#include "gaa_connect_state.h"
#include "hfp_profile_voice_source_device_mapping.h"
#include "voice_sources.h"
#include "bt_device.h"

#define RFCOMM_GAA_CONTROL_CHANNEL    20
#define RFCOMM_GAA_AUDIO_CHANNEL      21

static const uint8 control_channel_sdp_record [] =
{
    0x09, 0x00, 0x01,           /* ServiceClassIDList(0x0001) */
    0x35, 0x11,                 /* DataElSeq 17 bytes */
    0x1c,                       /* UUID f8d1fbe4-7966-4334-8024-ff96c9330e15 */
    0xf8, 0xd1, 0xfb, 0xe4,
    0x79, 0x66, 0x43, 0x34,
    0x80, 0x24, 0xff, 0x96,
    0xc9, 0x33, 0x0e, 0x15,
    0x09, 0x00, 0x04,           /* 22 23 24  ProtocolDescriptorList(0x0004) */
    0x35,   12,                 /* 25 26     DataElSeq 12 bytes */
    0x35,    3,                 /* 27 28     DataElSeq 3 bytes */
    0x19, 0x01, 0x00,           /* 29 30 31  UUID L2CAP(0x0100) */
    0x35,    5,                 /* 32 33     DataElSeq 5 bytes */
    0x19, 0x00, 0x03,           /* 34 35 36  UUID RFCOMM(0x0003) */
    0x08, RFCOMM_GAA_CONTROL_CHANNEL,  /* 37 38     uint8 RFCOMM channel */
    0x09, 0x00, 0x06,           /* 39 40 41  LanguageBaseAttributeIDList(0x0006) */
    0x35,    9,                 /* 42 43     DataElSeq 9 bytes */
    0x09,  'e',  'n',           /* 44 45 46  Language: English */
    0x09, 0x00, 0x6A,           /* 47 48 49  Encoding: UTF-8 */
    0x09, 0x01, 0x00,           /* 50 51 52  ID base: 0x0100 */
    0x09, 0x01, 0x00,           /* 53 54 55  ServiceName 0x0100, base + 0 */
    0x25, 0x07,                 /* String length 7 */
    'C','O','N','T','R','O','L'
};

static const uint8 audio_channel_sdp_record [] =
{
    0x09, 0x00, 0x01,           /* ServiceClassIDList(0x0001) */
    0x35, 0x11,                 /* DataElSeq 3 bytes */
    0x1c,                       /* UUID 81c2e72a-0591-443e-a1ff-05f988593351 */
    0x81, 0xc2, 0xe7, 0x2a,
    0x05, 0x91, 0x44, 0x3e,
    0xa1, 0xff, 0x05, 0xf9,
    0x88, 0x59, 0x33, 0x51,
    0x09, 0x00, 0x04,           /* 22 23 24  ProtocolDescriptorList(0x0004) */
    0x35,   12,                 /* 25 26     DataElSeq 12 bytes */
    0x35,    3,                 /* 27 28     DataElSeq 3 bytes */
    0x19, 0x01, 0x00,           /* 29 30 31  UUID L2CAP(0x0100) */
    0x35,    5,                 /* 32 33     DataElSeq 5 bytes */
    0x19, 0x00, 0x03,           /* 34 35 36  UUID RFCOMM(0x0003) */
    0x08, RFCOMM_GAA_AUDIO_CHANNEL,  /* 37 38     uint8 RFCOMM channel */
    0x09, 0x00, 0x06,           /* 39 40 41  LanguageBaseAttributeIDList(0x0006) */
    0x35,    9,                 /* 42 43     DataElSeq 9 bytes */
    0x09,  'e',  'n',           /* 44 45 46  Language: English */
    0x09, 0x00, 0x6A,           /* 47 48 49  Encoding: UTF-8 */
    0x09, 0x01, 0x00,           /* 50 51 52  ID base: 0x0100 */
    0x09, 0x01, 0x00,           /* 53 54 55  ServiceName 0x0100, base + 0 */
    0x25, 0x05,                 /* String length 5 */
    'A','U','D','I','O'
};

typedef struct
{
    TaskData              task;
    Sink                  sink;
    GSoundChannelType     channel_type;
} gaa_link_t;

typedef struct
{
    gaa_link_t control_channel;
    gaa_link_t audio_channel;
} gaa_service_t;

static gaa_service_t gaa_service = {
    .control_channel = {{.handler = NULL}, .sink = NULL, .channel_type = GSOUND_CHANNEL_CONTROL},
    .audio_channel = {{.handler = NULL}, .sink = NULL, .channel_type = GSOUND_CHANNEL_AUDIO}
};

TaskData gaa_link_task_data;

static const GSoundBtInterface *bt_handler;

static void gaa_updateGaaTwsIfTotallyDisconnected(void)
{
    if (Gaa_IsTotallyDisconnected())
    {
        if(gaa_IsTwsBuild() && Gaa_GetTwsTask())
            MessageSend(Gaa_GetTwsTask(),GAA_INTERNAL_TWS_DISCONNECTION_IND,NULL);               
    }
}

static void gaa_RegisterServiceRecord(const gaa_link_t *const gaa_link, const CL_RFCOMM_REGISTER_CFM_T *cfm)
{
    DEBUG_LOG("gaa_RegisterServiceRecord channel type %d", gaa_link->channel_type);
    if (cfm->status == success)
    {
        if (gaa_link->channel_type == GSOUND_CHANNEL_CONTROL)
        {
            ConnectionRegisterServiceRecord((Task)&gaa_link->task, sizeof(control_channel_sdp_record), control_channel_sdp_record);
        }
        else if (gaa_link->channel_type == GSOUND_CHANNEL_AUDIO)
        {
            ConnectionRegisterServiceRecord((Task)&gaa_link->task, sizeof(audio_channel_sdp_record), audio_channel_sdp_record);
        }
        else
        {
            DEBUG_LOG("gaa_RegisterServiceRecord no SDP record for Channel %d", gaa_link->channel_type);
            Panic();
        }
    }
}

static void gaa_ConvertBdAddrToGaaType(GSoundBTAddr *gaa_bd_addr, const bdaddr *bd_addr)
{
    gaa_bd_addr->lap = bd_addr->lap;
    gaa_bd_addr->nap = bd_addr->nap;
    gaa_bd_addr->uap = bd_addr->uap;
}

static void gaa_ConvertGaaAddrToBdAddr(bdaddr *bd_addr, const GSoundBTAddr *gaa_bd_addr)
{
    bd_addr->lap = gaa_bd_addr->lap;
    bd_addr->nap = gaa_bd_addr->nap;
    bd_addr->uap = gaa_bd_addr->uap;
}

static void gaa_HandleConnectionInd(const gaa_link_t *const gaa_link, const CL_RFCOMM_CONNECT_IND_T *ind)
{
    GSoundBTAddr gaa_bd_addr;
    DEBUG_LOG("gaa_HandleConnectionInd channel type %d", gaa_link->channel_type);
    
    gaa_ConvertBdAddrToGaaType(&gaa_bd_addr, &ind->bd_addr);
 
    bool response = bt_handler->gsound_bt_on_connect_ind(gaa_link->channel_type, &gaa_bd_addr) == GSOUND_STATUS_OK;
    if (response)
    {
        Gaa_SetChannelConnected(gaa_link->channel_type, TRUE);
    }
    ConnectionRfcommConnectResponse(&gaa_link->task, response, ind->sink, ind->server_channel, 0);
}

static void gaa_HandleConnectionCfm(gaa_link_t *const gaa_link, const CL_RFCOMM_SERVER_CONNECT_CFM_T* cfm)
{
    DEBUG_LOG("gaa_HandleConnectionCfm channel type %d", gaa_link->channel_type);
    if (cfm->status == success)
    {
        GSoundBTAddr gaa_bd_addr;
        gaa_ConvertBdAddrToGaaType(&gaa_bd_addr, &cfm->addr);

        if (gaa_link->sink)
        {
            Panic();
        }

        gaa_link->sink = cfm->sink;

        if (bt_handler->gsound_bt_on_connected(gaa_link->channel_type, &gaa_bd_addr) != GSOUND_STATUS_OK)
        {
            DEBUG_LOG("gaa_HandleConnectionCfm bt_on_connected returned an error");
        }
        else
        {
            Gaa_SetChannelConnected(gaa_link->channel_type, TRUE);
        }
    }
}

static void gaa_HandleDisconnection(gaa_link_t *const gaa_link)
{
    DEBUG_LOG("gaa_HandleDisconnection channel type %d", gaa_link->channel_type);
    if (gaa_link->sink)
    {
        StreamConnectDispose(StreamSourceFromSink(gaa_link->sink));
        gaa_link->sink = NULL;
    }

    if (bt_handler->gsound_bt_on_channel_disconnect(gaa_link->channel_type) != GSOUND_STATUS_OK)
    {
        DEBUG_LOG("gaa_HandleDisconnection bt_on_channel_disconnect returned an error");
    }
    else
    {
        Gaa_SetChannelConnected(gaa_link->channel_type, FALSE);
    }
}

static inline void gaa_HandleDisconnectionCfm(gaa_link_t *const gaa_link)
{
    DEBUG_LOG("gaa_HandleDisconnectionCfm channel type %d", gaa_link->channel_type);
    gaa_HandleDisconnection(gaa_link);
    gaa_updateGaaTwsIfTotallyDisconnected();
}

static void gaa_HandleDisconnectionInd(gaa_link_t *const gaa_link, const CL_RFCOMM_DISCONNECT_IND_T* ind)
{
    DEBUG_LOG("gaa_HandleDisconnectionInd channel type %d", gaa_link->channel_type);
    gaa_HandleDisconnection(gaa_link);
    ConnectionRfcommDisconnectResponse(ind->sink);
    gaa_updateGaaTwsIfTotallyDisconnected();
}

static inline void gaa_HandlePortNegoiationInd(const gaa_link_t *const gaa_link, const CL_RFCOMM_PORTNEG_IND_T* ind)
{
    DEBUG_LOG("gaa_HandlePortNegoiationInd channel type %d", gaa_link->channel_type);
    ConnectionRfcommPortNegResponse(&gaa_link->task, ind->sink, 0);
}

static void gaa_HandleReceivedData(const gaa_link_t *const gaa_link, Source source)
{
    if (gaa_link->channel_type != GSOUND_CHANNEL_CONTROL)
    {
        Panic();
    }

    if (StreamSinkFromSource(source) == gaa_link->sink)
    {
        uint16 length = SourceSize(source);
        const uint8 *received_data = SourceMap(source);

        if (bt_handler->gsound_bt_on_rx_ready(gaa_link->channel_type, received_data, length) != GSOUND_STATUS_OK)
        {
            DEBUG_LOG("gaa_HandleReceivedData bt_on_rx_ready returned an error");
        }
    }
}

static void gaa_HandleRfcommConnection(gaa_link_t *gaa_link, MessageId id, Message message)
{
    DEBUG_LOG("gaa_HandleRfcommConnection channel type %d msg id 0x%04x", gaa_link->channel_type, id);
    switch (id)
    {
        case CL_RFCOMM_REGISTER_CFM:
        {
            DEBUG_LOG("gaa_HandleRfcommConnection CL_RFCOMM_REGISTER_CFM");
            gaa_RegisterServiceRecord(gaa_link, (const CL_RFCOMM_REGISTER_CFM_T*)message);
        }
            break;
        case CL_RFCOMM_SERVER_CONNECT_CFM:
        {
            DEBUG_LOG("gaa_HandleRfcommConnection CL_RFCOMM_SERVER_CONNECT_CFM");
            gaa_HandleConnectionCfm(gaa_link, (const CL_RFCOMM_SERVER_CONNECT_CFM_T*)message);
        }
            break;
        case CL_RFCOMM_CONNECT_IND:
        {
            DEBUG_LOG("gaa_HandleRfcommConnection CL_RFCOMM_CONNECT_IND");
            gaa_HandleConnectionInd(gaa_link, (CL_RFCOMM_CONNECT_IND_T*) message);
        }
            break;
        case CL_RFCOMM_DISCONNECT_IND:
        {
            DEBUG_LOG("gaa_HandleRfcommConnection CL_RFCOMM_DISCONNECT_IND");
            gaa_HandleDisconnectionInd(gaa_link, (const CL_RFCOMM_DISCONNECT_IND_T *)message);
        }
            break;
        case CL_RFCOMM_DISCONNECT_CFM:
        {
            DEBUG_LOG("gaa_HandleRfcommConnection CL_RFCOMM_DISCONNECT_CFM");
            gaa_HandleDisconnectionCfm(gaa_link);
        }
            break;
        case CL_RFCOMM_PORTNEG_IND:
        {
            DEBUG_LOG("gaa_HandleRfcommConnection CL_RFCOMM_PORTNEG_IND");
            gaa_HandlePortNegoiationInd(gaa_link, (const CL_RFCOMM_PORTNEG_IND_T*)message);
        }
            break;
        default:
            DEBUG_LOG("gaa_HandleRfcommConnection un-handled CL message received id 0x%x", id);
            break;
    }
}

static void gaa_HandleRfcommData(gaa_link_t *gaa_link, MessageId id, Message message)
{
    switch(id)
    {
        case MESSAGE_MORE_SPACE:
        {
            DEBUG_LOG("gaa_HandleRfcommData MESSAGE_MORE_SPACE");
            bt_handler->gsound_bt_on_tx_available(gaa_link->channel_type);
        }
        break;
        case MESSAGE_MORE_DATA:
        {
            DEBUG_LOG("gaa_HandleRfcommData MESSAGE_MORE_DATA");
            /* We have received more data into the RFCOMM buffer */
            gaa_HandleReceivedData(gaa_link, ((const MessageMoreData *) message)->source);
        }
        break;

        default:
            /* Silently ignore these messages */
        break;
    }
}

static void gaa_GaaRfcommLinkHandler(Task task, MessageId id, Message message)
{
    gaa_link_t *gaa_link = (gaa_link_t *)task;

    if((id >= CL_MESSAGE_BASE) && (id < CL_MESSAGE_TOP))
    {
        gaa_HandleRfcommConnection(gaa_link, id, message);
    }
    else
    {
        gaa_HandleRfcommData(gaa_link, id, message);
    }
}

static void gaa_AllocateRfcommChannel(gaa_link_t *gaa_link)
{
    uint8 server_channel = 0;

    DEBUG_LOG("gaa_AllocateRfcommChannel channel type %d", gaa_link->channel_type);
    switch (gaa_link->channel_type)
    {
    case GSOUND_CHANNEL_CONTROL:
        server_channel = RFCOMM_GAA_CONTROL_CHANNEL;
        break;
    case GSOUND_CHANNEL_AUDIO:
        server_channel = RFCOMM_GAA_AUDIO_CHANNEL;
        break;
    default:
        DEBUG_LOG("gaa_AllocateRfcommChannel un-handled channel");
        Panic();
        break;
    }

    gaa_link->task.handler = gaa_GaaRfcommLinkHandler;

    ConnectionRfcommAllocateChannel((Task)&gaa_link->task, server_channel);
}

static GSoundStatus gaa_GetSinkForRfcommChannel(GSoundChannelType channel, Sink *sink)
{
    GSoundStatus status = GSOUND_STATUS_OK;

    switch (channel)
    {
    case GSOUND_CHANNEL_CONTROL:
        *sink = gaa_service.control_channel.sink;
        break;
    case GSOUND_CHANNEL_AUDIO:
        *sink = gaa_service.audio_channel.sink;
        break;
    default:
        DEBUG_LOG("gaa_GetSinkForRfcommChannel un-handled channel");
        status = GSOUND_STATUS_ERROR;
        break;
    }

    if ((sink == NULL) && (status == GSOUND_STATUS_OK))
    {
        DEBUG_LOG("gaa_GetSinkForRfcommChannel no associated sink with channel");
        status = GSOUND_STATUS_OUT_OF_MEMORY;
    }

    return status;
}

static GSoundStatus gaa_TransmitData(Sink *sink, const uint8_t *data, uint32_t length)
{
    GSoundStatus status = GSOUND_STATUS_OK;
    uint8 *mapped_addr;
    uint16 offset;

    mapped_addr = SinkMap(*sink);
    if (mapped_addr == NULL)
    {
        DEBUG_LOG("gaa_TransmitData SinkMap failed");
        status = GSOUND_STATUS_OUT_OF_MEMORY;
    }

    if (status == GSOUND_STATUS_OK)
    {
        offset = SinkClaim(*sink, length);

        if(offset == 0xFFFF)
        {
            DEBUG_LOG("gaa_TransmitData SinkClaim failed");
            status = GSOUND_STATUS_OUT_OF_MEMORY;
        }
        else
        {
            memcpy(mapped_addr+offset, data, length);

            if (SinkFlush(*sink, length) == FALSE)
            {
                DEBUG_LOG("gaa_TransmitData SinkFlush failed");
                status = GSOUND_STATUS_OUT_OF_MEMORY;
            }
        }
    }
    return status;
}

static void gaa_OnLinkMessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch(id)
    {
    /* HFP status change indications */
    case APP_HFP_CONNECTED_IND:
    {
        DEBUG_LOG("Gaa_OnLinkMessageHandler SLC Connected ");
        Gaa_OnLinkConnected(&((APP_HFP_CONNECTED_IND_T*)message)->bd_addr);
        break;
    }
    case APP_HFP_DISCONNECTED_IND:
    {
        DEBUG_LOG("Gaa_OnLinkMessageHandler SLC Disconnected ");
        Gaa_OnLinkDisconnected(&((APP_HFP_DISCONNECTED_IND_T*)message)->bd_addr);
        break;
    }
    default :
        break;
    }
}

void Gaa_OnLinkDisconnected(bdaddr *bd_addr)
{
    GSoundBTAddr addr;

    gaa_ConvertBdAddrToGaaType(&addr, bd_addr);

    bt_handler->gsound_bt_on_link_change(GSOUND_TARGET_BT_LINK_DISCONNECTED, &addr);
}

void Gaa_OnLinkConnected(bdaddr *bd_addr)
{
    GSoundBTAddr addr;

    gaa_ConvertBdAddrToGaaType(&addr, bd_addr);

    bt_handler->gsound_bt_on_link_change(GSOUND_TARGET_BT_LINK_CONNECTED, &addr);
}

void Gaa_InitialiseChannels(void)
{
    gaa_AllocateRfcommChannel(&gaa_service.control_channel);
    gaa_AllocateRfcommChannel(&gaa_service.audio_channel);
}

GSoundStatus GSoundTargetBtInit(const GSoundBtInterface *handler)
{
    bt_handler = handler;

    gaa_link_task_data.handler = gaa_OnLinkMessageHandler;
    /* Register with HFP to receive notifications of state changes .*/
    appHfpStatusClientRegister((Task)&gaa_link_task_data);
    return GSOUND_STATUS_OK;
}

GSoundStatus GSoundTargetBtHfpDial(const GSoundTargetBtHfpNumber *number, const GSoundBTAddr *gsound_addr)
{
    DEBUG_LOG("GSoundTargetBtHfpDial");
    GSoundStatus status = GSOUND_STATUS_ERROR;

    if(gsound_addr)
    {
        bdaddr bd_addr;
        gaa_ConvertGaaAddrToBdAddr(&bd_addr, gsound_addr);

        hfp_link_priority  link_to_dial  = HfpLinkPriorityFromBdaddr(&bd_addr);

        phone_number_t telephony_number;
        telephony_number.number_of_digits = number->size;
        telephony_number.digits = &number->number[0];

        VoiceSources_InitiateCallUsingNumber(HfpProfile_VoiceSourceDeviceMappingGetSourceForIndex(link_to_dial),telephony_number);
        status = GSOUND_STATUS_OK;
    }
    return status;
}

uint16_t GSoundTargetBtGetMtu(void)
{
    DEBUG_LOG("GSoundTargetBtGetMtu %d", RFCOMM_DEFAULT_PAYLOAD_SIZE);
    return RFCOMM_DEFAULT_PAYLOAD_SIZE;
}

bool GSoundTargetBtIsConnected(GSoundBTAddr const *gaa_addr)
{
    bdaddr bluetooth_address = {gaa_addr->lap, gaa_addr->uap, gaa_addr->nap};
    bool bt_connected = FALSE;

    DEBUG_LOG("GSoundTargetBtIsConnected %08x:%02x:%04x",
        bluetooth_address.lap, bluetooth_address.uap, bluetooth_address.nap);
    if (ConManagerIsConnected(&bluetooth_address))
    {
        bt_connected = TRUE;
    }

    return bt_connected;
}

void GSoundTargetBtRxComplete(GSoundChannelType type, const uint8_t *buffer,uint32_t len)
{
    UNUSED(buffer);

    if ((type == GSOUND_CHANNEL_CONTROL) && (gaa_service.control_channel.sink != NULL))
    {
        Source source = StreamSourceFromSink(gaa_service.control_channel.sink);
        SourceDrop(source, len);
    }
}

uint8_t GSoundTargetBtNumDevicesConnected(void)
{
    uint8_t number_of_devices = appDeviceIsHandsetConnected();

    DEBUG_LOG("GSoundTargetBtNumDevicesConnected %d", number_of_devices);

    return number_of_devices;
}

bool GSoundTargetBtHfpIsActive(void)
{
    hfp_call_state state;
    HfpLinkGetCallState(hfp_primary_link, &state) ;
    bool mic_in_use = state != hfp_call_state_idle;
    DEBUG_LOG("GSoundTargetBtHfpIsActive hfp_active %d", mic_in_use);

    if (!mic_in_use)
    {
        /* If HFP itself is not active, with iPhone, Siri might have SCO active to use the mic. */
        mic_in_use = appHfpIsScoActive();
        DEBUG_LOG("GSoundTargetBtHfpIsActive sco_active %d", mic_in_use);
    }

    if (!mic_in_use)
    {     
        mic_in_use = VoiceSources_IsVoiceRouted();
        DEBUG_LOG("GSoundTargetBtHfpIsActive voice_routed %d", mic_in_use);
    }

    return mic_in_use;
}

GSoundStatus GSoundTargetBtTransmit(GSoundChannelType channel,
                                    const uint8_t *data, uint32_t length,
                                    uint32_t *bytes_consumed)
{
    Sink sink = NULL;
    GSoundStatus status = GSOUND_STATUS_OK;

    if ((data == NULL) || (length == 0) || (bytes_consumed == NULL))
    {
        status = GSOUND_STATUS_ERROR;
    }
    else
    {
        *bytes_consumed = 0;
        status = gaa_GetSinkForRfcommChannel(channel, &sink);
    }

    if (status == GSOUND_STATUS_OK)
    {
        status = gaa_TransmitData(&sink, data, length);

        if (status == GSOUND_STATUS_OK)
        {
            DEBUG_LOG("GSoundTargetBtTransmit Transmission Successful");
            *bytes_consumed = length;
        }
    }

    return status;
}

GSoundStatus GSoundTargetBtChannelClose(GSoundChannelType channel)
{
    GSoundStatus status = GSOUND_STATUS_ERROR;
    Sink sink = NULL;
    Task task = NULL;

    switch (channel)
    {
        case GSOUND_CHANNEL_CONTROL:
            DEBUG_LOG("GSoundTargetBtChannelClose Disconnecting Control Channel");
            sink = gaa_service.control_channel.sink;
            task = (Task)&gaa_service.control_channel.task;
        break;

        case GSOUND_CHANNEL_AUDIO:
            DEBUG_LOG("GSoundTargetBtChannelClose Disconnecting Audio Channel");
            sink = gaa_service.audio_channel.sink;
            task = (Task)&gaa_service.audio_channel.task;
        break;

        default:
            DEBUG_LOG("GSoundTargetBtChannelClose Disconnecting requested of unsupported Channel");
            break;
    }

    if (sink && task)
    {
        ConnectionRfcommDisconnectRequest(&gaa_service.control_channel.task, sink);
        status = GSOUND_STATUS_OK;
    }

    return status;
}

