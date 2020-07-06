/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Source file converts System Events to corresponding Audio Prompt UI Events
            by table look-up, using a configuration table passed in by the Application.
            It then plays these Prompts when required using the Kymera audio framework
            Aux path.
*/

#include "ui_prompts.h"

#include "ui_inputs.h"
#include "pairing.h"
#include "ui.h"
#include "av.h"
#include <power_manager.h>

#include <domain_message.h>
#include <logging.h>
#include <panic.h>

#include <stdlib.h>
#ifdef ENABLE_TYM_PLATFORM
#include <earbud_sm.h>
#include "state_proxy.h"
#include "earbud_tym_sync.h"
#include "tym_anc.h"
#include "audio_sources.h"
#include "kymera_adaptation.h"
#include "multidevice.h"
int a2dp_volume_backup;
#endif
#include "system_clock.h"

#ifdef ENABLE_TYM_PLATFORM
#define DEFAULT_NO_REPEAT_DELAY         300 /*receive next prompt need wait 300 ms*/
#else
#define DEFAULT_NO_REPEAT_DELAY         D_SEC(5)
#endif

ui_prompts_task_data_t the_prompts;

#define PROMPT_NONE                     0xFFFF

#define UI_PROMPTS_WAIT_FOR_PROMPT_COMPLETION 0x1

/*! User interface internal messasges */
enum ui_internal_messages
{
    /*! Message sent later when a prompt is played. Until this message is delivered
        repeat prompts will not be played */
    UI_INTERNAL_CLEAR_LAST_PROMPT,
    UI_INTERNAL_PROMPT_PLAYBACK_COMPLETED
};

#ifdef ENABLE_TYM_PLATFORM
static void uiPrompts_UiInputProcess(MessageId id);
static void uiPrompts_RunUiPowerOff(void);
#endif
static bool uiPrompts_GetPromptIndexFromMappingTable(MessageId id, uint16 * prompt_index)
{
    return UiIndicator_GetIndexFromMappingTable(
                the_prompts.sys_event_to_prompt_data_mappings,
                the_prompts.mapping_table_size,
                id,
                prompt_index);
}

static const ui_prompt_data_t * uiPrompts_GetDataForPrompt(uint16 prompt_index)
{
    return &UiIndicator_GetDataForIndex(
                the_prompts.sys_event_to_prompt_data_mappings,
                the_prompts.mapping_table_size,
                prompt_index)->prompt;
}

inline static bool uiPrompt_isNotARepeatPlay(uint16 prompt_index)
{
    return prompt_index != the_prompts.last_prompt_played_index;
}

bool uiPrompt_NoPromptPlay(void)
{
    return (the_prompts.last_prompt_played_index == PROMPT_NONE);
}

/*! \brief Play prompt.

    \param prompt_index The prompt to play from the mappings table.
    \param time_to_play The microsecond at which to begin mixing of this audio prompt.
    \param config The prompt configuration data for the prompt to play.
*/
static void uiPrompts_PlayPrompt(uint16 prompt_index, rtime_t time_to_play, const ui_prompt_data_t *config)
{
    DEBUG_LOG("uiPrompts_PlayPrompt index=%d ttp=%d enabled=%d",
              prompt_index, time_to_play, the_prompts.prompt_playback_enabled );

    if (the_prompts.prompt_playback_enabled)
    {
        uint16 *client_lock = NULL;
        uint16 client_lock_mask = 0;

        UiIndicator_ScheduleIndicationCompletedMessage(
                the_prompts.sys_event_to_prompt_data_mappings,
                the_prompts.mapping_table_size,
                prompt_index,
                UI_INTERNAL_PROMPT_PLAYBACK_COMPLETED,
                &the_prompts.task,
                &the_prompts.prompt_playback_ongoing_mask,
                &client_lock,
                &client_lock_mask);

        FILE_INDEX *index = &the_prompts.prompt_file_indexes[prompt_index];

        if (*index == FILE_NONE)
        {
            const char* name = config->filename;
            *index = FileFind(FILE_ROOT, name, strlen(name));
            /* Prompt not found */
            PanicFalse(*index != FILE_NONE);
        }

        DEBUG_LOG("uiPrompts_PlayPrompt FILE_INDEX=%08x format=%d rate=%d", *index , config->format, config->rate );

        appKymeraPromptPlay(*index, config->format, config->rate, time_to_play,
                            config->interruptible, client_lock, client_lock_mask);

        if(the_prompts.no_repeat_period_in_ms != 0)
        {
            MessageCancelFirst(&the_prompts.task, UI_INTERNAL_CLEAR_LAST_PROMPT);
            MessageSendLater(&the_prompts.task, UI_INTERNAL_CLEAR_LAST_PROMPT, NULL,
                             the_prompts.no_repeat_period_in_ms);
            the_prompts.last_prompt_played_index = prompt_index;

            #ifdef ENABLE_TYM_PLATFORM
            if(!Multidevice_IsLeft()){
                volume_t a2dp_volume;
                a2dp_volume = AudioSources_GetVolume(audio_source_a2dp_1);
                a2dp_volume.value = a2dp_volume_backup;
                DEBUG_LOG("[TYM] Recovery A2DP Volume %d %d",a2dp_volume.value);
                AudioSources_SetVolume(audio_source_a2dp_1, a2dp_volume);
                AudioSources_OnVolumeChange(audio_source_a2dp_1, event_origin_local, a2dp_volume);

                if(audio_source_a2dp_1 == AudioSources_GetRoutedSource())
                {
                    volume_parameters_t volume_params = { .source_type = source_type_audio, .volume = a2dp_volume };
                    KymeraAdaptation_SetVolume(&volume_params);
                }
            }
            #endif
        }
    }      
}

static void uiPrompts_SchedulePromptPlay(uint16 prompt_index)
{
    const ui_prompt_data_t *config = uiPrompts_GetDataForPrompt(prompt_index);
    #ifdef ENABLE_TYM_PLATFORM
    volume_t a2dp_volume;
    #endif


    if (uiPrompt_isNotARepeatPlay(prompt_index) &&
        (config->queueable || (!appKymeraIsTonePlaying() && (the_prompts.prompt_playback_ongoing_mask == 0))))
    {
        /* Factor in the propagation latency through the various buffers for the aux channel and the time to start the file source */
        rtime_t time_now = SystemClockGetTimerTime();
        rtime_t time_to_play = rtime_add(time_now, UI_INDICATOR_DELAY_FOR_SYNCHRONISED_TTP_IN_MICROSECONDS);

        #ifdef ENABLE_TYM_PLATFORM
        if(!Multidevice_IsLeft()){
            a2dp_volume = AudioSources_GetVolume(audio_source_a2dp_1);
            a2dp_volume_backup = a2dp_volume.value;
            DEBUG_LOG("[TYM] uiPrompts_PlayPrompt right %d",a2dp_volume_backup);
            if(a2dp_volume.value >=0)
            {
                /*
                if(a2dp_volume.value >= 56)
                    a2dp_volume.value = a2dp_volume.value - 49; //decrease ~20db
                else
                    a2dp_volume.value = 7; //~-43db
                */
                a2dp_volume.value = 7; //~-43db
            }

            AudioSources_SetVolume(audio_source_a2dp_1, a2dp_volume);
            AudioSources_OnVolumeChange(audio_source_a2dp_1, event_origin_local, a2dp_volume);

            if(audio_source_a2dp_1 == AudioSources_GetRoutedSource())
            {
                volume_parameters_t volume_params = { .source_type = source_type_audio, .volume = a2dp_volume };
                KymeraAdaptation_SetVolume(&volume_params);
            }
        }
        #endif

        time_to_play = Ui_RaiseUiEvent(ui_indication_type_audio_prompt, prompt_index, time_to_play);

        uiPrompts_PlayPrompt(prompt_index, time_to_play, config);
    }
}

static void uiPrompts_HandleMessage(Task task, MessageId id, Message message)
{
    uint16 prompt_index = 0;

    UNUSED(task);
    UNUSED(message);

    DEBUG_LOG("uiPrompts_HandleMessage Id=%04x", id);

    bool prompt_found = uiPrompts_GetPromptIndexFromMappingTable(id, &prompt_index);
    if (prompt_found)
    {
        if (the_prompts.generate_ui_events)
        {
            uiPrompts_SchedulePromptPlay(prompt_index);
        }
        else
        {
            /* Prompts that need to be indicated before shutdown should always be played,
               regardless of whether we are rendering indications based on the current
               device topology role and any other gating factors. */
            ui_event_indicator_table_t prompt_to_play = the_prompts.sys_event_to_prompt_data_mappings[prompt_index];
            bool indicate_on_shutdown = prompt_to_play.await_indication_completion;
            if (indicate_on_shutdown)
            {
                uiPrompts_SchedulePromptPlay(prompt_index);
            }
        }
    }
    else if (id == UI_INTERNAL_CLEAR_LAST_PROMPT)
    {
        DEBUG_LOG("UI_INTERNAL_CLEAR_LAST_PROMPT");
        the_prompts.last_prompt_played_index = PROMPT_NONE;
    }
    else if (id == UI_INTERNAL_PROMPT_PLAYBACK_COMPLETED)
    {
        DEBUG_LOG("UI_INTERNAL_PROMPT_PLAYBACK_COMPLETED indicate=%d", the_prompts.indicate_when_power_shutdown_prepared);
        DEBUG_LOG("UI_INTERNAL_PROMPT_PLAYBACK_COMPLETED pwr off indicate=%d", the_prompts.indicate_when_user_poweroff_prepared);
        if (the_prompts.indicate_when_power_shutdown_prepared)
        {
            appPowerShutdownPrepareResponse(&the_prompts.task);
        }
#ifdef ENABLE_TYM_PLATFORM        
        if (the_prompts.indicate_when_user_poweroff_prepared)
        {
            uiPrompts_RunUiPowerOff();       
        }    
#endif        
    }
    else if (id == APP_POWER_SHUTDOWN_PREPARE_IND)
    {
        the_prompts.indicate_when_power_shutdown_prepared = TRUE;
    }
    else if (id == APP_POWER_SLEEP_PREPARE_IND)
    {
        appPowerSleepPrepareResponse(&the_prompts.task);
    }
#ifdef ENABLE_TYM_PLATFORM
    else if((id >= UI_INPUTS_PROMPT_MESSAGE_BASE) && (id < UI_INPUTS_VOICE_UI_MESSAGE_BASE))
    {
        uiPrompts_UiInputProcess(id);
    }
    else if(id == APP_POWER_USERPOWEROFF_PREPARE_IND)
    {
        the_prompts.indicate_when_user_poweroff_prepared = TRUE;
        if((StateProxy_IsInEar() == TRUE)||(StateProxy_IsPeerInEar() == TRUE))
        {
            if(the_prompts.generate_ui_events)
            {
                Ui_InjectUiInput(ui_input_prompt_poweroff);
            }
            //wait 1 second, wait master send power off prompts, if not send ,force 1 seconds power off.
            MessageSendLater(&the_prompts.task, APP_POWER_USERPOWEROFF_RESPOND_IND, NULL, D_SEC(1));
        }
        else
        {
            uiPrompts_RunUiPowerOff();
        }
        DEBUG_LOG("UI_PROMPTS recv APP_POWER_USERPOWEROFF_PREPARE_IND");
    }
    else if(id == APP_POWER_USERPOWEROFF_RESPOND_IND)
    {
        uiPrompts_RunUiPowerOff();
    }
#endif
    else
    {
        // Ignore message
    }
}

/*! \brief brief Set/reset play_prompt flag. This is flag is used to check if prompts
  can be played or not. Application will set and reset the flag. Scenarios like earbud
  is in ear or not and etc.

    \param play_prompt If TRUE, prompt can be played, if FALSE, the prompt can not be
    played.
*/
void UiPrompts_SetPromptPlaybackEnabled(bool play_prompt)
{
    the_prompts.prompt_playback_enabled = play_prompt;
}

Task UiPrompts_GetUiPromptsTask(void)
{
    return &the_prompts.task;
}

void UiPrompts_SetPromptConfiguration(const ui_event_indicator_table_t *table, uint8 size)
{
    the_prompts.sys_event_to_prompt_data_mappings = table;
    the_prompts.mapping_table_size = size;

    the_prompts.prompt_file_indexes = PanicUnlessMalloc(size*sizeof(FILE_INDEX));
    memset(the_prompts.prompt_file_indexes, FILE_NONE, size*sizeof(FILE_INDEX));

    UiIndicator_RegisterInterestInConfiguredSystemEvents(
                the_prompts.sys_event_to_prompt_data_mappings,
                the_prompts.mapping_table_size,
                &the_prompts.task);
}

void UiPrompts_SetNoRepeatPeriod(const Delay no_repeat_period_in_ms)
{
    the_prompts.no_repeat_period_in_ms = no_repeat_period_in_ms;
}

void UiPrompts_NotifyUiIndication(uint16 prompt_index, rtime_t time_to_play)
{
    const ui_prompt_data_t *config = uiPrompts_GetDataForPrompt(prompt_index);
    uiPrompts_PlayPrompt(prompt_index, time_to_play, config);
}

/*! brief Initialise Ui prompts module */
bool UiPrompts_Init(Task init_task)
{
    UNUSED(init_task);

    DEBUG_LOG("UiPrompts_Init");

    memset(&the_prompts, 0, sizeof(ui_prompts_task_data_t));

    the_prompts.last_prompt_played_index = PROMPT_NONE;
    the_prompts.task.handler = uiPrompts_HandleMessage;
    the_prompts.no_repeat_period_in_ms = DEFAULT_NO_REPEAT_DELAY;
    the_prompts.generate_ui_events = TRUE;
    the_prompts.prompt_playback_enabled = TRUE;
#ifdef ENABLE_TYM_PLATFORM
    TaskList_Initialise(&the_prompts.clients);
    Ui_RegisterUiInputsMessageGroup(&the_prompts.task, UI_INPUTS_PROMPT_MESSAGE_GROUP);
#endif
    return TRUE;
}


/*! brief de-initialise Ui prompts module */
bool UiPrompts_DeInit(void)
{
    DEBUG_LOG("UiPrompts_DeInit");

    the_prompts.sys_event_to_prompt_data_mappings = NULL;
    the_prompts.mapping_table_size = 0;

    free(the_prompts.prompt_file_indexes);
    the_prompts.prompt_file_indexes = NULL;

    return TRUE;
}

void UiPrompts_GenerateUiEvents(bool generate)
{
    the_prompts.generate_ui_events = generate;
}

#ifdef ENABLE_TYM_PLATFORM
static void uiPrompts_RunUiPowerOff(void)
{
    if (the_prompts.indicate_when_user_poweroff_prepared)
    {
        the_prompts.indicate_when_user_poweroff_prepared = FALSE;
        appUserPowerOffAction();
    }
}

bool Prompts_GetConnectedStatus(void)
{
    return the_prompts.ever_connected_prompt;
}

void Prompts_SetConnectedStatus(bool connected)
{
    the_prompts.ever_connected_prompt = connected;
}

/*! brief cancel prompts pairing */
void Prompts_CancelPairingContinue(void)
{
    MessageCancelFirst(&the_prompts.task, ui_input_prompt_pairing_continue);
}

void UiPrompts_SendTymPrompt(MessageId id)
{
     //TaskList_MessageSendId(&the_prompts.clients, id);
    MessageSend(&the_prompts.task, id, NULL);
}

void UiPrompts_SendTymPromptLater(MessageId id,uint32 delay)
{
     //TaskList_MessageSendLaterId(&the_prompts.clients, id, delay);
    MessageSendLater(&the_prompts.task, id, NULL, delay);
}

void UiPrompt_ConnectedPrompt(void)
{
    if(the_prompts.ever_connected_prompt == FALSE)
    {    
        the_prompts.ever_connected_prompt = TRUE;
        if(appSmIsPrimary())
            Ui_InjectUiInput(ui_input_prompt_connected);    
    }
}

static void prompts_RegisterMessageGroup(Task task, message_group_t group)
{
    PanicFalse(group == PROMPTS_MESSAGE_GROUP);
    TaskList_AddTask(&the_prompts.clients, task);
}


static void uiPrompts_UiInputProcess(MessageId id)
{
    tymAncTaskData *tymAnc = TymAncGetTaskData();
        
    if(id == ui_input_prompt_pairing_continue)
    {        
        if(tymAnc->onceAnc != 0) /*have set ANC don't play pairing prompt*/
            return;
        MessageCancelFirst(&the_prompts.task, ui_input_prompt_pairing_continue);
        MessageSendLater(&the_prompts.task, ui_input_prompt_pairing_continue, NULL, D_SEC(5));//interval 3 second + 2 second pairing play time
        if((StateProxy_IsInEar() == TRUE)||(StateProxy_IsPeerInEar() == TRUE))
            Ui_InjectUiInput(ui_input_prompt_pairing);
    }
    else if(id == ui_input_prompt_connected)
    {
        DEBUG_LOG("check connect prompt"); 
    }
    else if(id == ui_input_prompt_repeat_findme)
    {
        MessageCancelFirst(&the_prompts.task, ui_input_prompt_repeat_findme);
        MessageSendLater(&the_prompts.task, ui_input_prompt_repeat_findme, NULL, D_SEC(5));//interval 3 second + 2 second pairing play time
        Ui_InjectUiInput(ui_input_prompt_findme);
    }
    else if((StateProxy_IsInCase() == TRUE) && (StateProxy_IsPeerInEar() == FALSE) )
    {  
        DEBUG_LOG("In Case && Peer not InEar don't play prompts");
        return;
    }

    switch(id)
    {
        case ui_input_prompt_pairing:
            UiPrompts_SendTymPrompt(PROMPT_PAIRING);
            break;
        case ui_input_prompt_pairing_successful:
            UiPrompts_SendTymPrompt(PROMPT_PAIRING_SUCCESSFUL);
            break;
        case ui_input_prompt_pairing_failed:
            UiPrompts_SendTymPrompt(PROMPT_PAIRING_FAILED);
            break;
        case ui_input_prompt_connected:
            if((StateProxy_IsInEar() == TRUE)||(StateProxy_IsPeerInEar() == TRUE))
                UiPrompts_SendTymPrompt(PROMPT_CONNECTED);
            break;
        case ui_input_prompt_disconnected:
            UiPrompts_SendTymPrompt(PROMPT_DISCONNECTED);
            break;
        case ui_input_prompt_anc_off:
            UiPrompts_SendTymPrompt(PROMPT_ANC_OFF);
            break;
        case ui_input_prompt_anc_on:
            UiPrompts_SendTymPrompt(PROMPT_ANC_ON);
            break;
        case ui_input_prompt_quick_attention_off:
            UiPrompts_SendTymPrompt(PROMPT_QuickAttention_OFF);
            break;
        case ui_input_prompt_quick_attention_on:
            UiPrompts_SendTymPrompt(PROMPT_QuickAttention_ON);
            break;
        case ui_input_prompt_ambient_off:
            UiPrompts_SendTymPrompt(PROMPT_AMBIENT_OFF);
            break;
        case ui_input_prompt_ambient_on:
            UiPrompts_SendTymPrompt(PROMPT_AMBIENT_ON);
            break;
        case ui_input_prompt_speech_off:
            UiPrompts_SendTymPrompt(PROMPT_SPEECH_OFF);
            break;
        case ui_input_prompt_speech_on:
            UiPrompts_SendTymPrompt(PROMPT_SPEECH_ON);
            break;
        case ui_input_prompt_volume_limit:
            UiPrompts_SendTymPrompt(PROMPT_MAX_VOLUME);
            break;
        case ui_input_prompt_battery_low:
            UiPrompts_SendTymPrompt(PROMPT_BATTERY_LOW);
            break;
        case ui_input_prompt_role_switch:
            UiPrompts_SendTymPrompt(PROMPT_ROLE_SWITCH);
            break;
        case ui_input_prompt_findme:
            UiPrompts_SendTymPrompt(PROMPT_FINDME);
            break;
        case ui_input_prompt_stop_findme:
            MessageCancelFirst(&the_prompts.task, ui_input_prompt_repeat_findme);
            break;
        case ui_input_prompt_poweron:
            UiPrompts_SendTymPrompt(PROMPT_POWER_ON);
            break;
        case ui_input_prompt_poweroff:
            UiPrompts_SendTymPrompt(PROMPT_POWER_OFF);
            break;
        case ui_input_prompt_connected_check:
            if((StateProxy_IsInEar() == TRUE) || (StateProxy_IsPeerInEar() == TRUE))
            {
                DEBUG_LOG("ui_input_prompt_connected_check run exe after 1s");
                UiPrompts_SendTymPromptLater(ui_input_prompt_connected_execute, D_SEC(1));
            }
            else
            {
                DEBUG_LOG("ui_input_prompt_connected_check not in ear");
            }    
            break;
        case ui_input_prompt_connected_execute:
            if((StateProxy_IsInEar() == TRUE) || (StateProxy_IsPeerInEar() == TRUE))
                tymSyncdata(connectPromptCmd,0);
            break;
        default:
            break;
    }
}

MESSAGE_BROKER_GROUP_REGISTRATION_MAKE(PROMPTS, prompts_RegisterMessageGroup, NULL);

#endif
