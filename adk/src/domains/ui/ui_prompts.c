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
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch,for sync of prompt*/
#include "ui_private.h"
#include <av.h>
#include <hfp_profile.h>
#endif
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
#include "audio_curation.h"
#include "kymera_adaptation.h"
#include "kymera_private.h"
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
#ifdef ENABLE_TYM_PLATFORM
    volume_t a2dp_volume;
    volume_parameters_t volume_params;
    bool force_play_prompt = FALSE;
    
    if((prompt_index == 0) || (prompt_index == 8))// power-on, findme force play prompt
        force_play_prompt = TRUE;
	/*fixed BEL0677. FindMe Tone in left bud is not loud as FindMe tone in right bud*/
    if(prompt_index == 8)//findMe set max volume flag
        appKymeraSetPromptVol(1);
    else
        appKymeraSetPromptVol(0);         
        
    if (the_prompts.prompt_playback_enabled || (force_play_prompt == TRUE))//force play
#else
    if (the_prompts.prompt_playback_enabled)
#endif    	
    {
#ifdef ENABLE_TYM_PLATFORM
        if(appAvIsStreaming())
        {
            a2dp_volume = AudioSources_GetVolume(audio_source_a2dp_1);
            a2dp_volume.value = 7;
            if(a2dp_volume_backup > 0){
                DEBUG_LOG("[TYM ]Decrease A2DP Volume");
                volume_params.source_type = source_type_audio;
                volume_params.volume = a2dp_volume;
                KymeraAdaption_TYM_SetVolume(&volume_params);
            }

        }
#else   /*add Qualcomm patch,for sync of prompt*/     
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
#endif   
        FILE_INDEX *index = &the_prompts.prompt_file_indexes[prompt_index];

        if (*index == FILE_NONE)
        {
            const char* name = config->filename;
            *index = FileFind(FILE_ROOT, name, strlen(name));
            /* Prompt not found */
            PanicFalse(*index != FILE_NONE);
        }
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch,for sync of prompt*/     
        if (the_prompts.sys_event_to_prompt_data_mappings[prompt_index].await_indication_completion)
        {
            MessageSendConditionally(&the_prompts.task, UI_INTERNAL_PROMPT_PLAYBACK_COMPLETED, NULL, ui_GetKymeraResourceLockAddress());
        }
#endif        
        DEBUG_LOG("uiPrompts_PlayPrompt FILE_INDEX=%08x format=%d rate=%d", *index , config->format, config->rate );
        
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch,for sync of prompt*/     
        appKymeraPromptPlay(*index, config->format, config->rate, time_to_play,
                            config->interruptible, ui_GetKymeraResourceLockAddress(), UI_KYMERA_RESOURCE_LOCKED);
#else
        appKymeraPromptPlay(*index, config->format, config->rate, time_to_play,
                            config->interruptible, client_lock, client_lock_mask);
#endif                            

        if(the_prompts.no_repeat_period_in_ms != 0)
        {
            MessageCancelFirst(&the_prompts.task, UI_INTERNAL_CLEAR_LAST_PROMPT);
            MessageSendLater(&the_prompts.task, UI_INTERNAL_CLEAR_LAST_PROMPT, NULL,
                             the_prompts.no_repeat_period_in_ms);
            the_prompts.last_prompt_played_index = prompt_index;
            #ifdef ENABLE_TYM_PLATFORM
            if(appAvIsStreaming() && (prompt_index != 6))
            {
                a2dp_volume = AudioSources_GetVolume(audio_source_a2dp_1);
                volume_params.source_type = source_type_audio;
                volume_params.volume = a2dp_volume;

                if(a2dp_volume_backup > 0){
                    a2dp_volume.value = a2dp_volume_backup;
                    KymeraAdaption_TYM_SetVolume(&volume_params);
                }
            }
            #endif
        }
    }      
}

static void uiPrompts_SchedulePromptPlay(uint16 prompt_index)
{
    const ui_prompt_data_t *config = uiPrompts_GetDataForPrompt(prompt_index);
    
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch,for sync of prompt*/  
    if (uiPrompt_isNotARepeatPlay(prompt_index) &&
        (config->queueable || (!appKymeraIsTonePlaying() && !ui_IsKymeraResourceLocked()))) 
#else
    if (uiPrompt_isNotARepeatPlay(prompt_index) &&
        (config->queueable || (!appKymeraIsTonePlaying() && (the_prompts.prompt_playback_ongoing_mask == 0))))
#endif        
    {
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch,for sync of prompt*/
        /* Factor in the propagation latency through the various buffers for the aux channel and the time to start the file source */
        rtime_t time_to_play = SystemClockGetTimerTime();

        /* First stage delay for tone prompt chain creation */
        time_to_play = rtime_add(time_to_play, UI_SYNC_IND_AUDIO_SS_FIXED_DELAY);

        if (!appAvIsStreaming() && !appHfpIsScoActive())
        {
            /* Second stage delay for out chain creation, including PEQ */
            time_to_play = rtime_add(time_to_play, UI_SYNC_IND_OUTPUT_CHAIN_CREATION_DELAY);
            int32 due;
            if (appKymeraAudioDisabled(&due) || (due > 0 && due < D_SEC(3)))
            {
                /* Third stage delay for audio SS boot, when audio SS has less than 3 seconds of active time */
                time_to_play = rtime_add(time_to_play, UI_SYNC_IND_AUDIO_SS_POWER_ON_DELAY);
            }
        }

        /* Always add extra event marshal delay */
        time_to_play = rtime_add(time_to_play, UI_SYNC_IND_UI_EVENT_MARSHAL_DELAY);
#else
        /* Factor in the propagation latency through the various buffers for the aux channel and the time to start the file source */
        rtime_t time_now = SystemClockGetTimerTime();
        rtime_t time_to_play = rtime_add(time_now, UI_INDICATOR_DELAY_FOR_SYNCHRONISED_TTP_IN_MICROSECONDS);
#endif
        time_to_play = Ui_RaiseUiEvent(ui_indication_type_audio_prompt, prompt_index, time_to_play);

        uiPrompts_PlayPrompt(prompt_index, time_to_play, config);
    }
}

#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch,for sync of prompt*/  
static bool uiPrompts_IsPromptMandatory(uint16 prompt_index)
{
    ui_event_indicator_table_t prompt_to_play = the_prompts.sys_event_to_prompt_data_mappings[prompt_index];
    bool indicate_on_shutdown = prompt_to_play.await_indication_completion;
    return indicate_on_shutdown;
}

static void uiPrompts_HandleInternalPrompt(Task task, MessageId prompt_index, Message message)
{
    UNUSED(task);
    UNUSED(message);

    DEBUG_LOG("uiPrompts_HandleInternalPrompt index=%u", prompt_index);

    /* Mandatory prompts (e.g. indicating shutdown) should always be played,
       regardless of whether we are rendering indications based on the current
       device topology role and any other gating factors. */
    if (the_prompts.generate_ui_events || uiPrompts_IsPromptMandatory(prompt_index))
    {
        uiPrompts_SchedulePromptPlay(prompt_index);
    }
}
#endif

static void uiPrompts_HandleMessage(Task task, MessageId id, Message message)
{
    uint16 prompt_index = 0;

    UNUSED(task);
    UNUSED(message);

    DEBUG_LOG("uiPrompts_HandleMessage Id=%04x", id);
#ifdef ENABLE_TYM_PLATFORM   /*add Qualcomm patch,for sync of prompt*/  
    if (uiPrompts_GetPromptIndexFromMappingTable(id, &prompt_index))
    {
        Task t = &the_prompts.prompt_task;
        if (MessagesPendingForTask(t, NULL) < UI_PROMPTS_MAX_QUEUE_SIZE || uiPrompts_IsPromptMandatory(prompt_index))
        {
            MessageSendConditionally(t, prompt_index, NULL, ui_GetKymeraResourceLockAddress());
        }
        else
        {
            DEBUG_LOG("uiPrompts_HandleMessage not queuing id=%04x", id);
        }
    }
#else 
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
#endif    
    else if (id == UI_INTERNAL_CLEAR_LAST_PROMPT)
    {
        DEBUG_LOG("UI_INTERNAL_CLEAR_LAST_PROMPT");
#ifdef ENABLE_TYM_PLATFORM          
        audioCurationClearLockBit(0x01);  
#endif        
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
            MessageSendLater(&the_prompts.task, APP_POWER_USERPOWEROFF_RESPOND_IND, NULL, D_SEC(2));
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
#ifdef ENABLE_TYM_PLATFORM /*fixed left earbud heard power off prompt when right earbud enter power off in the charging case*/
    powerTaskData *thePower = PowerGetTaskData();   

    if(prompt_index == 1)//power off
    {
        if(thePower->poweroff_ongoing == FALSE)/*It's not power off on-going don't play power off*/
            return;
    }    
#endif    
    uiPrompts_PlayPrompt(prompt_index, time_to_play, config);
}

/*! brief Initialise Ui prompts module */
bool UiPrompts_Init(Task init_task)
{
    UNUSED(init_task);
#ifdef ENABLE_TYM_PLATFORM    
    FILE_INDEX prompt_index = FILE_NONE;
    Source prompt_source;    
#endif    
    DEBUG_LOG("UiPrompts_Init");

    memset(&the_prompts, 0, sizeof(ui_prompts_task_data_t));

    the_prompts.last_prompt_played_index = PROMPT_NONE;
    the_prompts.task.handler = uiPrompts_HandleMessage;
#ifdef ENABLE_TYM_PLATFORM   /*add Qualcomm patch,for sync of prompt*/      
    the_prompts.prompt_task.handler = uiPrompts_HandleInternalPrompt;    
#endif    
    the_prompts.no_repeat_period_in_ms = DEFAULT_NO_REPEAT_DELAY;
    the_prompts.generate_ui_events = TRUE;
    the_prompts.prompt_playback_enabled = TRUE;
#ifdef ENABLE_TYM_PLATFORM
    TaskList_Initialise(&the_prompts.clients);
    Ui_RegisterUiInputsMessageGroup(&the_prompts.task, UI_INPUTS_PROMPT_MESSAGE_GROUP);
    prompt_index = FileFind(FILE_ROOT, "prompt.index", strlen("prompt.index"));
    if(prompt_index != FILE_NONE)
    {
        prompt_source = StreamFileSource(prompt_index);
        const uint8 *data = SourceMap(prompt_source);
        the_prompts.prompt_lang = (*data - 0x30);//0 ->asscii 0x30
        DEBUG_LOG("prompt language %d",the_prompts.prompt_lang);
        SourceClose(prompt_source);
    }        
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
    MessageCancelAll(&the_prompts.task, ui_input_prompt_pairing_continue);
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
        MessageSendLater(&the_prompts.task, ui_input_prompt_pairing_continue, NULL, 4500);//interval 2.5 second + 2 second pairing play time
        if((StateProxy_IsInEar() == TRUE)||(StateProxy_IsPeerInEar() == TRUE))
            Ui_InjectUiInput(ui_input_prompt_pairing);
    }
    else if((id == ui_input_prompt_connected) || (id == ui_input_prompt_stop_findme))
    {
        DEBUG_LOG("check connect prompt or stop findme"); 
    }
    else if(id == ui_input_prompt_repeat_findme)
    {
        MessageCancelFirst(&the_prompts.task, ui_input_prompt_repeat_findme);
        MessageSendLater(&the_prompts.task, ui_input_prompt_repeat_findme, NULL, 4500);//interval 2.5 second + 2 second pairing play time
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

void UiPrompts_SetA2DPVolume_InTone(int volume)
{
    a2dp_volume_backup = volume;
    DEBUG_LOG("UiPrompts_SetToneVolume_InMusic %d\n",a2dp_volume_backup);
}

uint8 UiPrompts_GetLanguage(void)
{
    return the_prompts.prompt_lang;
}

MESSAGE_BROKER_GROUP_REGISTRATION_MAKE(PROMPTS, prompts_RegisterMessageGroup, NULL);

#endif
