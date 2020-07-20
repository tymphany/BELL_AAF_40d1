/*!
\copyright  Copyright (c) 2018-2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation for GAA audio.Refer to gsound_target_audio.h for API information.
*/
#include "gaa.h"
#include "gaa_debug.h"
#include "gaa_private.h"
#include "voice_audio_manager.h"
#include <source.h>
#include <stdlib.h>
#include <bdaddr.h>
#include <logging.h>
#include <voice_ui.h>
#include "gatt_server_gaa_comm.h"
#include "gsound_target.h"
#include "av.h"
#include "a2dp_typedef.h"
#include "voice_sources.h"

/* Private #defines */
#define TX_FRAMES_THRESHOLD                 5
#define SBC_HEADER_SIZE                     4
#define DISABLE_SIDETONE                    FALSE
#define DISABLE_HOTWORD                     FALSE
#define DISABLE_SBC_HEADERS                 FALSE
#define CORRECT_SBC_FRAME_SYNCWORD          0x9C
#define STARTUP_INVALID_BITPOOL_SETTING     0
#define SYNCWORD_OFFSET                     0
#define BITPOOL_OFFSET                      2
#define NO_DELAY_ON_VOICE_CAPTURE           0

/* Forward function declarations */
static void gaa_AudioInputDataReceived(Source source);
static void gaa_PrepareCodecConfiguration(va_audio_voice_capture_params_t *audio_cfg, uint32 bit_pool);
static inline void gaa_SetNextCaptureState(void);
static void gaa_AudioInternalMessageHandler(Task task, MessageId id, Message message);

static TaskData gaa_audio_task_data = {gaa_AudioInternalMessageHandler};

/* Private types */
typedef enum {audio_uninitialised, capture_idle, capture_in_progress, last_capture_state = capture_in_progress} capture_state;

typedef struct
{
    capture_state               audio_capture_state;
    const GSoundAudioInterface  *gaa_lib_callback;
    GSoundTargetAudioInSettings input_settings;
    GSoundTransport             transport;
    bool                        ble_primary_streaming;
}GAA_AUDIO;

/* Private data */
static GAA_AUDIO gaa_audio_data =
{
    audio_uninitialised,
    NULL,
    {STARTUP_INVALID_BITPOOL_SETTING, DISABLE_SIDETONE, DISABLE_HOTWORD, DISABLE_SBC_HEADERS},
    GSOUND_TRANSPORT_NONE,
    FALSE
};

static bool gaa_BlePrimaryStreaming(const GSoundTargetAudioStreamingSettings *settings)
{
    bool ble_primary_streaming = FALSE;

    if ((settings->connected_transport == GSOUND_TRANSPORT_BLE) && appDeviceIsHandsetA2dpStreaming())
    {
        bdaddr connected_address;
        bdaddr streaming_handset;

        if (appDeviceGetHandsetBdAddr(&streaming_handset))
        {
            connected_address.nap = settings->connected_address.nap;
            connected_address.uap = settings->connected_address.uap;
            connected_address.lap = settings->connected_address.lap;

            ble_primary_streaming = BdaddrIsSame(&streaming_handset, &connected_address);
        }
    }

    return ble_primary_streaming;
}

GSoundStatus GSoundTargetAudioInInit(const GSoundAudioInterface *handler)
{
    gaa_audio_data.gaa_lib_callback = handler;
    gaa_SetNextCaptureState();
    DEBUG_LOG("GSoundTargetAudioInInit");
    return GSOUND_STATUS_OK;
}

GSoundStatus GSoundTargetAudioInOpen(const GSoundTargetAudioInSettings *audio_in_settings)
{
    /* input args:
       raw_samples      = "This is used for 1st stage hotword evaluation" - currently not supported
       enable_sidetone  = currently not supported
       bitpool          = From their spec... "12 is used for BLE and 24 is used for RFCOMM" */
    GSoundStatus status = GSOUND_STATUS_OK;
    va_audio_voice_capture_params_t audio_cfg = {0};

    DEBUG_LOG("GSoundTargetAudioInOpen: Audio In Open request in state %d", gaa_audio_data.audio_capture_state);

    if((gaa_audio_data.audio_capture_state != capture_in_progress)  && (VoiceSources_IsVoiceRouted() == FALSE))
    {
        if(audio_in_settings->raw_samples_required == FALSE)
        {
            gaa_PrepareCodecConfiguration(&audio_cfg, audio_in_settings->sbc_bitpool);
            GAA_DEBUG_ASSERT(gaa_audio_data.audio_capture_state == capture_idle);
            DEBUG_LOG("GSoundTargetAudioInOpen: Trigger Start Capture");
            VoiceAudioManager_StartCapture(gaa_AudioInputDataReceived, &audio_cfg);
            gaa_SetNextCaptureState();
        }
        else
        {
            DEBUG_LOG("GSoundTargetAudioInOpen: Failed to Start Capture");
            GAA_DEBUG_ASSERT(FALSE);
            status = GSOUND_STATUS_ERROR;
        }

        gaa_audio_data.input_settings = *audio_in_settings;
    }
    return status;
}

GSoundStatus GSoundTargetAudioInClose(void)
{
    DEBUG_LOG("GSoundTargetAudioInClose");
    Gaa_AudioInClose();
    return GSOUND_STATUS_OK;
}

GSoundStatus GSoundTargetAudioOutInit(void)
{
    return GSOUND_STATUS_OK;
}

GSoundStatus GSoundTargetAudioOutStart(GSoundTargetAudioOutClip clip)
{
    GSoundStatus status = GSOUND_STATUS_ERROR;


/*  These cases are exhaustive; adding to enum GSoundTargetAudioOutClip
 *  will cause a MissingCase compiler warning.
 */
    switch (clip)
    {
    case GSOUND_AUDIO_OUT_CLIP_PREPARE_MIC_OPEN:
    case GSOUND_AUDIO_OUT_CLIP_PTT_MIC_OPEN:
    case GSOUND_AUDIO_OUT_CLIP_REMOTE_MIC_OPEN:
        VoiceUi_Notify(VOICE_UI_MIC_OPEN);
        status = GSOUND_STATUS_OK;
        break;

    case GSOUND_AUDIO_OUT_CLIP_PTT_MIC_CLOSE:
    case GSOUND_AUDIO_OUT_CLIP_REMOTE_MIC_CLOSE:
        VoiceUi_Notify(VOICE_UI_MIC_CLOSE);
        status = GSOUND_STATUS_OK;
        break;

    case GSOUND_AUDIO_OUT_CLIP_GSOUND_NC:
        VoiceUi_Notify(VOICE_UI_DISCONNECTED);
        status = GSOUND_STATUS_OK;
        break;

    case GSOUND_AUDIO_OUT_CLIP_QUERY_INTERRUPTED:
    case GSOUND_AUDIO_OUT_CLIP_FETCH:
    case GSOUND_AUDIO_OUT_CLIP_PTT_REJECTED:
        break;

    case GSOUND_AUDIO_OUT_CLIP_PTT_QUERY:
        status = GSOUND_STATUS_OK;
        break;
    }
    return status;
}

GSoundStatus GSoundTargetAudioPrepareForStreaming(const GSoundTargetAudioStreamingSettings *settings)
{
    DEBUG_LOG("GSoundTargetAudioPrepareForStreaming: t=%u", settings->connected_transport);

    gaa_audio_data.transport = settings->connected_transport;
    gaa_audio_data.ble_primary_streaming = gaa_BlePrimaryStreaming(settings);

    return GSOUND_STATUS_OK;
}

GSoundStatus GSoundTargetSetSidetone(bool sidetone_enabled)
{
    UNUSED(sidetone_enabled);
    return GSOUND_STATUS_OK;
}

void Gaa_AudioInClose(void)
{
    if (gaa_audio_data.audio_capture_state == capture_in_progress)
    {
        VoiceAudioManager_StopCapture();
        gaa_SetNextCaptureState();
        MessageSend(Gaa_GetAudioTask(), GAA_INTERNAL_START_RESPONSE_IND, NULL);
    }
}

TaskData *Gaa_GetAudioTask(void)
{
    return &gaa_audio_task_data;
}

static inline void gaa_SetNextCaptureState(void)
{
    gaa_audio_data.audio_capture_state++;
    if(gaa_audio_data.audio_capture_state > last_capture_state)
    {
        gaa_audio_data.audio_capture_state = capture_idle;
    }
}

static void SendSbcFramesToGaa(const uint8* sbc_frames, const uint32 number_of_frames, const uint32 frame_size)
{
    const uint32 header_stripped_frame_size = frame_size - SBC_HEADER_SIZE;
    const uint32 buffer_size = number_of_frames * header_stripped_frame_size;
    uint8 *buffer = PanicUnlessMalloc(buffer_size);
    uint32 frame_index;

    /* Strip SBC header from the frames before sending to Gaa */
    for(frame_index = 0; frame_index < number_of_frames; frame_index++)
    {
        GAA_DEBUG_ASSERT(sbc_frames[frame_index * frame_size + SYNCWORD_OFFSET] == CORRECT_SBC_FRAME_SYNCWORD);
        memmove(&buffer[frame_index * header_stripped_frame_size], &sbc_frames[frame_index * frame_size + SBC_HEADER_SIZE], header_stripped_frame_size);
    }

    gaa_audio_data.gaa_lib_callback->gsound_target_on_audio_in(buffer, buffer_size, header_stripped_frame_size);
    free(buffer);
}

static void gaa_AudioInputDataReceived(Source source)
{
    /* Useful Notes:
     * source = block of data containing SBC frame(s).
     * SBC frame = header, scale factors, audio samples, padding
     * encoded_frame_len = length of single sbc frame
     * encoded_payload_length = one or more sbc frames */
    const uint32 encoded_payload_length = SourceSize(source);

    DEBUG_LOG("gaa_AudioInputDataReceived: Received Voice Data Samples");
    if(encoded_payload_length)
    {
        const uint8 *encoded_data = SourceMap(source);
        GAA_DEBUG_ASSERT(encoded_data);
        uint8 bitpool = encoded_data[BITPOOL_OFFSET];

        /* 4 + (4 * num_subbands (8) * num_channels (1) ) / 8 + (block_size (16) * num_channels (1) * bitpool (variable) / 8) */
        const uint32 encoded_frame_len = 8 + (2 * bitpool);

        const uint32 number_of_frames = encoded_payload_length / encoded_frame_len;

        /* Send at least TX_FRAMES_THRESHOLD frames to Gaa. Ensure that the last remaining frames of the audio capture get transmitted */
        if (number_of_frames >= TX_FRAMES_THRESHOLD)
        {
            SendSbcFramesToGaa(encoded_data, TX_FRAMES_THRESHOLD, encoded_frame_len);
            SourceDrop(source, TX_FRAMES_THRESHOLD * encoded_frame_len);
        }
        else if (number_of_frames && (gaa_audio_data.audio_capture_state == capture_idle))
        {
            SendSbcFramesToGaa(encoded_data, number_of_frames, encoded_frame_len);
            SourceDrop(source, number_of_frames * encoded_frame_len);
        }
    }
}

static void gaa_PrepareCodecConfiguration(va_audio_voice_capture_params_t *audio_cfg, uint32 bit_pool)
{
    va_audio_encoder_sbc_params_t *sbc_params = &audio_cfg->encode_config.encoder_params.sbc;

    /* The following settings are specified in gsound_target_audio.h
     * All samples are encoded to 16bits and mono channel encoding is used by default.
     */
    audio_cfg->encode_config.encoder = va_audio_codec_sbc;
    audio_cfg->mic_config.sample_rate = 16000;
    sbc_params->number_of_subbands = 8;
    sbc_params->block_length = 16;
    sbc_params->bitpool_size = bit_pool;
    sbc_params->allocation_method = sbc_encoder_allocation_method_loudness;
}

static void gaa_AudioInternalMessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    switch(id)
    {
        case GAA_INTERNAL_START_QUERY_IND:
            if (gaa_audio_data.ble_primary_streaming)
                appAvStreamingSuspend(AV_SUSPEND_REASON_VA);

            break;

        case GAA_INTERNAL_END_QUERY_IND:
            if (gaa_audio_data.ble_primary_streaming)
                appAvStreamingResume(AV_SUSPEND_REASON_VA);
            break;

        case GAA_INTERNAL_START_RESPONSE_IND:
            Gaa_GetVoiceUiProtectedInterface()->SetVoiceAssistantA2dpStreamState(voice_ui_a2dp_state_streaming);
            break;

        case GAA_INTERNAL_END_RESPONSE_IND:
            Gaa_GetVoiceUiProtectedInterface()->SetVoiceAssistantA2dpStreamState(voice_ui_a2dp_state_suspended);
            break;
#ifdef ENABLE_TYM_PLATFORM          /*added Qualcomm patch,for cancel bisto */
        case GAA_INTERNAL_STOP_ASSISTANT:
            Gaa_GetVoiceUiProtectedInterface()->SetVoiceAssistantA2dpStreamState(voice_ui_a2dp_state_suspended);
            break;
#endif
        default:
            DEBUG_LOG("gaa_AudioInternalMessageHandler unhandled 0x%04X", id);
            break;
    }
}
