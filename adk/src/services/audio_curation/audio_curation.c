/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       audio_curation.c
\brief      A component responsible for controlling audio curation services.
*/

#include "audio_curation.h"
#include "logging.h"
#include "ui.h"
#include "anc_state_manager.h"
#include "anc.h"
#include "power_manager.h"
#include "aec_leakthrough.h"
#ifdef ENABLE_TYM_PLATFORM
#include "tym_anc.h"
#include "av.h"
#include "kymera_private.h"
#include "earbud_tym_gaia.h"
#include "state_proxy.h"
#include "ui_prompts.h"
#include "earbud_tym_sync.h"
#endif

#define audioCuration_SendEvent(msg_id) TaskList_MessageSendId(audioCuration_GetMessageClients(), msg_id)
static anc_mode_t anc_mode;/* An odd hook to know there was ANC mode change */
static task_list_t * client_list;

static void audioCuration_HandleMessage(Task task, MessageId id, Message message);
static const TaskData ui_task = {audioCuration_HandleMessage};
#ifdef ENABLE_TYM_PLATFORM
static void BellUiAncOff(void);
static void BellUiAncOn(void);
static void BellUiAmbientOn(void);
static void BellUiSpeechOn(void);
static void BellUiPPAmbient(void);
#endif


static const message_group_t ui_inputs[] =
{
    UI_INPUTS_AUDIO_CURATION_MESSAGE_GROUP
};

static Task audioCuration_UiTask(void)
{
  return (Task)&ui_task;
}

static task_list_t * audioCuration_GetMessageClients(void)
{
    return client_list;
}

static void audioCuration_InitMessages(void)
{
    client_list = TaskList_Create();
}

static void handleAncClientEvent(MessageId id, Message message)
{
    if(id == ANC_UPDATE_IND)
    {
        ANC_UPDATE_IND_T *ind = (ANC_UPDATE_IND_T*)message;
        DEBUG_LOG("handleAncClientEvent, ANC State: %d, Mode: %d", ind->state, ind->mode);
        /* check if the ANC mode is changed? */
        if(ind->mode != anc_mode)
            audioCuration_SendEvent(AUDIO_CURATION_ANC_MODE_CHANGED);
        else if(ind->state)
            audioCuration_SendEvent(AUDIO_CURATION_ANC_ON);
        else
            audioCuration_SendEvent(AUDIO_CURATION_ANC_OFF);
    }
}

static anc_mode_t getAncModeFromUiInput(MessageId ui_input)
{
    anc_mode_t mode;

    switch(ui_input)
    {
        case ui_input_anc_set_mode_1:
            mode = anc_mode_1;
            break;
        case ui_input_anc_set_mode_2:
            mode = anc_mode_2;
            break;
        case ui_input_anc_set_mode_3:
            mode = anc_mode_3;
            break;
        case ui_input_anc_set_mode_4:
            mode = anc_mode_4;
            break;
        case ui_input_anc_set_mode_5:
            mode = anc_mode_5;
            break;
        case ui_input_anc_set_mode_6:
            mode = anc_mode_6;
            break;
        case ui_input_anc_set_mode_7:
            mode = anc_mode_7;
            break;
        case ui_input_anc_set_mode_8:
            mode = anc_mode_8;
            break;
        case ui_input_anc_set_mode_9:
            mode = anc_mode_9;
            break;
        case ui_input_anc_set_mode_10:
            mode = anc_mode_10;
        break;
        default:
            mode = anc_mode_1;
            break;
    }
    return mode;
}

#ifdef ENABLE_AEC_LEAKTHROUGH
static leakthrough_mode_t getLeakthroughModeFromUiInput(MessageId ui_input)
{
    leakthrough_mode_t leakthrough_mode;

    switch(ui_input)
    {
        case ui_input_leakthrough_set_mode_1:
            leakthrough_mode = LEAKTHROUGH_MODE_1;
            break;
        case ui_input_leakthrough_set_mode_2:
            leakthrough_mode = LEAKTHROUGH_MODE_2;
            break;
        case ui_input_leakthrough_set_mode_3:
            leakthrough_mode = LEAKTHROUGH_MODE_3;
            break;
        default:
            leakthrough_mode = LEAKTHROUGH_MODE_1;
            break;
    }
    return leakthrough_mode;
}
#endif

/*! \brief provides ANC state machine context to the User Interface module.

    \param[in]  void

    \return     context - current context of ANC state machine.
*/
#ifdef ENABLE_ANC
static unsigned getAncCurrentContext(void)
{
    audio_curation_provider_context_t context = BAD_CONTEXT;
#ifdef ENABLE_TYM_PLATFORM
    tymAncTaskData *tymAnc = TymAncGetTaskData();
#endif

    if(AncStateManager_IsTuningModeActive())
    {
        context = context_anc_tuning_mode_active;
    }
#ifdef ENABLE_TYM_PLATFORM
    else if(tymAnc->curAncMode == ambient)
    {
        context = context_ambient_enabled;
    }
    else
    {
        context = context_ambient_disabled;
    }
#else
    else if(AncStateManager_IsEnabled())
    {
        context = context_anc_enabled;
    }
    else
    {
        context = context_anc_disabled;
    }
#endif
    return (unsigned)context;
}
#endif

/*! Provides the leakthrough context to the User Interface module. */
#ifdef ENABLE_AEC_LEAKTHROUGH
static unsigned getLeakthroughCurrentContext(void)
{
    audio_curation_provider_context_t context = BAD_CONTEXT;
    if(AecLeakthrough_IsLeakthroughEnabled())
    {
        context = context_leakthrough_enabled;
    }
    else
    {
        context = context_leakthrough_disabled;
    }
    return (unsigned)context;
}
#endif

static void handlePowerClientEvents(MessageId id)
{
    switch(id)
    {
        /* Power indication */
        case APP_POWER_SHUTDOWN_PREPARE_IND:
            AncStateManager_PowerOff();
            AecLeakthrough_PowerOff();
            appPowerShutdownPrepareResponse(audioCuration_UiTask());
            break;

        case APP_POWER_SLEEP_PREPARE_IND:
            AncStateManager_PowerOff();
            AecLeakthrough_PowerOff();
            appPowerSleepPrepareResponse(audioCuration_UiTask());
            break;

        /*In case of Sleep/SHUTDOWN cancelled*/
        case APP_POWER_SHUTDOWN_CANCELLED_IND:
        case APP_POWER_SLEEP_CANCELLED_IND:
            AncStateManager_PowerOn();
            AecLeakthrough_PowerOn();
            break;

        default:
            break;
    }
}

static void handleUiDomainInput(MessageId ui_input)
{

    anc_mode = AncStateManager_GetMode();

    switch(ui_input)
    {
        case ui_input_anc_on:
            DEBUG_LOG("handleUiDomainInput, anc on input");
#ifdef ENABLE_TYM_PLATFORM        /*temp solution, prompt play will cut when enable internal ANC*/    
            if(uiPrompt_NoPromptPlay() == TRUE)
                AncStateManager_Enable();
            else                                    
                MessageSendLater(audioCuration_UiTask(), ui_input, NULL, D_SEC(1));
#else
            AncStateManager_Enable();
#endif                
            break;

        case ui_input_anc_off:
            DEBUG_LOG("handleUiDomainInput, anc off input");
#ifdef ENABLE_TYM_PLATFORM                
            if(uiPrompt_NoPromptPlay() == TRUE)
                AncStateManager_Disable();
            else
                MessageSendLater(audioCuration_UiTask(), ui_input, NULL, D_SEC(1));   
#else
            AncStateManager_Disable();
#endif                
            break;

        case ui_input_anc_toggle_on_off:
            DEBUG_LOG("handleUiDomainInput, anc toggle on/off input");
            if(AncStateManager_IsEnabled())
            {
                AncStateManager_Disable();
            }
            else
            {
                AncStateManager_Enable();
            }

            break;

        case ui_input_anc_set_mode_1:
        case ui_input_anc_set_mode_2:
        case ui_input_anc_set_mode_3:
        case ui_input_anc_set_mode_4:
        case ui_input_anc_set_mode_5:
        case ui_input_anc_set_mode_6:
        case ui_input_anc_set_mode_7:
        case ui_input_anc_set_mode_8:
        case ui_input_anc_set_mode_9:
        case ui_input_anc_set_mode_10:
            DEBUG_LOG("handleUiDomainInput, anc set mode input");
#ifdef ENABLE_TYM_PLATFORM        /*temp solution, prompt play will cut when enable internal ANC*/                
            if(uiPrompt_NoPromptPlay() == TRUE)
                AncStateManager_SetMode(getAncModeFromUiInput(ui_input));
            else                    
                MessageSendLater(audioCuration_UiTask(), ui_input, NULL, D_SEC(1));
            break;
#else
            AncStateManager_SetMode(getAncModeFromUiInput(ui_input));
#endif
        case ui_input_anc_set_next_mode:
            DEBUG_LOG("handleUiDomainInput, anc next mode input");
            AncStateManager_SetNextMode();
            break;

        case ui_input_anc_enter_tuning_mode:
            DEBUG_LOG("handleUiDomainInput, enter anc tuning input");
            AncStateManager_EnterTuningMode();
            break;

        case ui_input_anc_exit_tuning_mode:
            DEBUG_LOG("handleUiDomainInput, exit anc tuning input");
            AncStateManager_ExitTuningMode();
            break;

        case ui_input_anc_set_leakthrough_gain:
            DEBUG_LOG("handleUiDomainInput, set anc leakthrough gain input");
            AncStateManager_UpdateAncLeakthroughGain();
            break;

        /* Leak-through UI Input events */
        case ui_input_leakthrough_on:
            DEBUG_LOG("handleUiDomainInput, leakthrough on input");
            AecLeakthrough_Enable();
            break;

        case ui_input_leakthrough_off:
            DEBUG_LOG("handleUiDomainInput, leakthrough off input");
            AecLeakthrough_Disable();
            break;

        case ui_input_leakthrough_toggle_on_off:
            DEBUG_LOG("handleUiDomainInput, leakthrough toggle on/off input");
            if(AecLeakthrough_IsLeakthroughEnabled())
            {
                AecLeakthrough_Disable();
            }
            else
            {
                AecLeakthrough_Enable();
            }
            break;

        case ui_input_leakthrough_set_mode_1:
        case ui_input_leakthrough_set_mode_2:
        case ui_input_leakthrough_set_mode_3:
            DEBUG_LOG("handleUiDomainInput, leakthrough set mode input");
            AecLeakthrough_SetMode(getLeakthroughModeFromUiInput(ui_input));
            break;

        case ui_input_leakthrough_set_next_mode:
            DEBUG_LOG("handleUiDomainInput, leakthrough set next mode input");
            AecLeakthrough_SetNextMode();
            break;
#ifdef ENABLE_TYM_PLATFORM
        case ui_input_ext_anc_on:
            DEBUG_LOG("handleUiDomainInput, ext anc on");
            if(getExtAncEnableStatus() == FALSE)
                stanc3_ancon();
            break;
        case ui_input_ext_anc_off:
            DEBUG_LOG("handleUiDomainInput, ext anc off");
            if(getExtAncEnableStatus() == TRUE)
                stanc3_ancoff();
            break;
        case ui_input_bell_ui_anc_on:
            DEBUG_LOG("handleUiDomainInput, ui_input_bell_ui_anc_on");
            if(StateProxy_IsInCase() == FALSE)
                Ui_InjectUiInput(ui_input_prompt_anc_on);
            BellUiAncControl(ui_input);
            break;
        case ui_input_bell_ui_anc_off:
            DEBUG_LOG("handleUiDomainInput, ui_input_bell_ui_anc_off");
            if(StateProxy_IsInCase() == FALSE)
                Ui_InjectUiInput(ui_input_prompt_anc_off);
            BellUiAncControl(ui_input);
            break;
        case ui_input_bell_ui_ambient_on:
            DEBUG_LOG("handleUiDomainInput, ui_input_bell_ui_ambient_on");
            if(StateProxy_IsInCase() == FALSE)
                Ui_InjectUiInput(ui_input_prompt_ambient_on);
            BellUiAncControl(ui_input);
            break;
        case ui_input_bell_ui_ambient_off:
            DEBUG_LOG("handleUiDomainInput, ui_input_bell_ui_ambient_off");
            if(StateProxy_IsInCase() == FALSE)
                Ui_InjectUiInput(ui_input_prompt_ambient_off);
            BellUiAncControl(ui_input);
            break;
        case ui_input_bell_ui_speech_on:
            DEBUG_LOG("handleUiDomainInput, ui_input_bell_ui_speech_on");
            if(StateProxy_IsInCase() == FALSE)
                Ui_InjectUiInput(ui_input_prompt_speech_on);
            BellUiAncControl(ui_input);
            break;
        case ui_input_bell_ui_speech_off:
            DEBUG_LOG("handleUiDomainInput, ui_input_bell_ui_speech_off");
            if(StateProxy_IsInCase() == FALSE)
                Ui_InjectUiInput(ui_input_prompt_speech_off);
            BellUiAncControl(ui_input);
            break;
        case ui_input_bell_ui_pp_ambient:
            DEBUG_LOG("handleUiDomainInput, ui_input_bell_ui_pp_ambient");
            BellUiAncControl(ui_input);
            break;
        case ui_input_bell_ui_switch_preset_bank0:
        case ui_input_bell_ui_switch_preset_bank1:
        case ui_input_bell_ui_switch_preset_bank2:
        case ui_input_bell_ui_switch_preset_bank3:
        case ui_input_bell_ui_switch_preset_bank4:
        case ui_input_bell_ui_switch_preset_bank5:
        case ui_input_bell_ui_switch_preset_bank6:
            DEBUG_LOG("handleUiDomainInput, ui_input_bell_ui_switch_preset_bank %x",(ui_input - ui_input_bell_ui_switch_preset_bank0));
            appKymeraSelectUsrEQPreset(ui_input - ui_input_bell_ui_switch_preset_bank0);
            break;
#endif
        default:
            DEBUG_LOG("handleUiDomainInput, unhandled input");
            break;
    }
}

static unsigned getCurrentContext(void)
{
#ifdef ENABLE_AEC_LEAKTHROUGH
    return getLeakthroughCurrentContext();
#elif defined ENABLE_ANC
    return getAncCurrentContext();
#else
    return (unsigned)BAD_CONTEXT;
#endif
}


static void audioCuration_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    if (id >= APP_POWER_INIT_CFM && id <= APP_POWER_SHUTDOWN_CANCELLED_IND)
    {
        handlePowerClientEvents(id);
    }
    if(ID_TO_MSG_GRP(id) == ANC_MESSAGE_GROUP)
    {
        handleAncClientEvent(id, message);
    }
    if (ID_TO_MSG_GRP(id) == UI_INPUTS_AUDIO_CURATION_MESSAGE_GROUP)
    {
        handleUiDomainInput(id);
    }
}


/*! \brief Initialise the audio curation service

    \param[in]  init_task - added to inline with app_init

    \return     bool - returns initalization status
*/
bool AudioCuration_Init(Task init_task)
{
    Ui_RegisterUiProvider(ui_provider_audio_curation, getCurrentContext);

    Ui_RegisterUiInputConsumer(audioCuration_UiTask(), ui_inputs, ARRAY_DIM(ui_inputs));

    DEBUG_LOG("AudioCuration_Init, called");

    

    /* register with power to receive shutdown messages. */
    appPowerClientRegister(audioCuration_UiTask());
    appPowerClientAllowSleep(audioCuration_UiTask());
	audioCuration_InitMessages();
	AncStateManager_ClientRegister(audioCuration_UiTask());
    /* AEC-Leakthrough Post Init setup for peer sync */
    AecLeakthrough_PostInitSetup();

    UNUSED(init_task);
    
    return TRUE;
}

static void audioCuration_RegisterMessageGroup(Task task, message_group_t group)
{
    PanicFalse(group == AUDIO_CURATION_SERVICE_MESSAGE_GROUP);
    TaskList_AddTask(audioCuration_GetMessageClients(), task);
}

MESSAGE_BROKER_GROUP_REGISTRATION_MAKE(AUDIO_CURATION_SERVICE, audioCuration_RegisterMessageGroup, NULL);


#ifdef ENABLE_TYM_PLATFORM
/*\brief external ANC off,internal ANC off*/
static void BellUiAncOff(void)
{
    if(getExtAncEnableStatus() == TRUE)
    {
        Ui_InjectUiInput(ui_input_ext_anc_off);
    }
    if(AncStateManager_IsEnabled())
    {
        Ui_InjectUiInput(ui_input_anc_off);
    }

}

/*\brief external ANC on,internal ANC off*/
static void BellUiAncOn(void)
{
    if(getExtAncEnableStatus() == FALSE)
    {
        Ui_InjectUiInput(ui_input_ext_anc_on);
    }
    if(AncStateManager_IsEnabled())
    {
        Ui_InjectUiInput(ui_input_anc_off);
    }
}

/*\brief setup ampbient level,1: anc_mode_1(0) */
void setupAmbientLevel(void)
{
    tymAncTaskData *tymAnc = TymAncGetTaskData();
    anc_mode_t  anc_mode_l = AncStateManager_GetMode();
    MessageId ui_input;
    if(tymAnc->ambientLevel != (anc_mode_l + 1))
    {
        ui_input = ui_input_anc_set_mode_1 + (tymAnc->ambientLevel-1);
        Ui_InjectUiInput(ui_input);
    }
}

/*\brief setup speech level 1: anc_mode_6(5)*/
void setupSpeechLevel(void)
{
    tymAncTaskData *tymAnc = TymAncGetTaskData();
    anc_mode_t  anc_mode_l = AncStateManager_GetMode();
    MessageId ui_input;
    if((tymAnc->speechLevel+4) != anc_mode_l)
    {
        ui_input = ui_input_anc_set_mode_6 + (tymAnc->speechLevel-1);
        Ui_InjectUiInput(ui_input);
    }
}
/*\brief external ANC on,internal ANC on, choose different mode */
static void BellUiAmbientOn(void)
{
    tym_sync_app_configuration_t *app_set = TymGet_AppSetting();
    if(app_set->ambient_ext_anc)
    {
        if(getExtAncEnableStatus() == FALSE)
        {
            Ui_InjectUiInput(ui_input_ext_anc_on);
        }
    }
    else
    {
        if(getExtAncEnableStatus() == TRUE)
        {
            Ui_InjectUiInput(ui_input_ext_anc_off);
        }
    }
    setupAmbientLevel();
    if(AncStateManager_IsEnabled() == FALSE)
    {
        Ui_InjectUiInput(ui_input_anc_on);
    }
}

/*\brief external ANC on,internal ANC on, choose different mode */
static void BellUiSpeechOn(void)
{
    if(getExtAncEnableStatus() == FALSE)
    {
        Ui_InjectUiInput(ui_input_ext_anc_on);
    }
    setupSpeechLevel();
    if(AncStateManager_IsEnabled() == FALSE)
    {
        Ui_InjectUiInput(ui_input_anc_on);
    }
}

static void BellUiPPAmbient(void)
{
    bool playing = (appAvPlayStatus() == avrcp_play_status_playing);
    tymAncTaskData *tymAnc = TymAncGetTaskData();
    /* == play === */
    if(playing == TRUE)
    {
        if(tymAnc->curAncMode != ambient)
        {
            tymAnc->prevAncMode = tymAnc->curAncMode;
            tymAnc->curAncMode = ambient;
            BellUiAmbientOn();
            bell_gaia_anc_notify_event(BELL_GAIA_AMBIENT_NOTIFY, 1);
        }
    }
    else
    {
        /* == pause === */
        if(tymAnc->prevAncMode != amcinvalid)
        {
            tymAnc->curAncMode = tymAnc->prevAncMode;
            tymAnc->prevAncMode = ambient;
            if(tymAnc->curAncMode == ancoff)
            {
                BellUiAncOff();
                bell_gaia_anc_notify_event(BELL_GAIA_ANC_NOTIFY, 0);
            }
            else if(tymAnc->curAncMode == ancon)
            {
                BellUiAncOn();
                bell_gaia_anc_notify_event(BELL_GAIA_ANC_NOTIFY, 1);
            }
            else if(tymAnc->curAncMode == ambient)
            {
                BellUiAmbientOn();
                bell_gaia_anc_notify_event(BELL_GAIA_AMBIENT_NOTIFY, 1);
            }
            else if(tymAnc->curAncMode == speech)
            {
                BellUiSpeechOn();
                bell_gaia_anc_notify_event(BELL_GAIA_SPEECH_NOTIFY, 1);
            }
        }
    }
}

/*\brief Bell ANC control,check external/internal ANC on/off , internal mode*/
void BellUiAncControl(MessageId ui_input)
{
    tymAncTaskData *tymAnc = TymAncGetTaskData();
    if(ui_input != ui_input_bell_ui_pp_ambient)
    {
        tymAnc->prevAncMode = amcinvalid;
    }
    switch(ui_input)
    {
        case ui_input_bell_ui_anc_on:
            tymAnc->curAncMode = ancon;
            tymAnc->onceAnc = 1;
            BellUiAncOn();
            bell_gaia_anc_notify_event(BELL_GAIA_ANC_NOTIFY, 1);
            break;
        case ui_input_bell_ui_ambient_on:
            tymAnc->curAncMode = ambient;
            BellUiAmbientOn();
            bell_gaia_anc_notify_event(BELL_GAIA_AMBIENT_NOTIFY, 1);
            break;
        case ui_input_bell_ui_speech_on:
            tymAnc->curAncMode = speech;
            tymAnc->onceAnc = 1;
            BellUiSpeechOn();
            bell_gaia_anc_notify_event(BELL_GAIA_SPEECH_NOTIFY, 1);
            break;
        case ui_input_bell_ui_pp_ambient:
            BellUiPPAmbient();
            break;
        case ui_input_bell_ui_anc_off:
        case ui_input_bell_ui_ambient_off:
        case ui_input_bell_ui_speech_off:
        default:
            tymAnc->curAncMode = ancoff;
            BellUiAncOff();
            break;

    }
    if(ui_input == ui_input_bell_ui_anc_off)
    {
        bell_gaia_anc_notify_event(BELL_GAIA_ANC_NOTIFY, 0);
    }
    else if(ui_input == ui_input_bell_ui_ambient_off)
    {
        bell_gaia_anc_notify_event(BELL_GAIA_AMBIENT_NOTIFY, 0);
    }
    else if(ui_input == ui_input_bell_ui_speech_off)
    {
        bell_gaia_anc_notify_event(BELL_GAIA_SPEECH_NOTIFY, 0);
    }
}
#endif

