/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation for GAA Actions APIs
*/

#include <logging.h>
#include "gsound_target.h"
#include "ui.h"
#include "gaa_private.h"
#include "voice_ui_container.h"
#ifdef ENABLE_TYM_PLATFORM
#include "logical_input_switch.h"
#endif

typedef struct
{
    ui_input_t voice_assistant_user_event;
    GSoundActionEvents action_event;
} va_event_translation_t;

typedef struct
{
    const va_event_translation_t* event_translations;
    unsigned num_translations;
} va_translation_table_t;

static const GSoundActionInterface *action_handlers;
static va_translation_table_t va_translation_table;
inject_ui_input inject_unhandled_ui_events;

static const struct
{
    ui_input_t ui_event;
    GSoundActionEvents action_event;
} ui_translation_table[] =
{
    { ui_input_toggle_play_pause,   GSOUND_TARGET_ACTION_TOGGLE_PLAY_PAUSE},
    { ui_input_av_forward,          GSOUND_TARGET_ACTION_NEXT_TRACK},
    { ui_input_av_backward,         GSOUND_TARGET_ACTION_PREV_TRACK},
    { ui_input_hfp_voice_dial,      GSOUND_TARGET_ACTION_LEGACY_VOICE_ACTIVATION}
};

static const va_event_translation_t one_button_va_event_translations[] =
{
    { ui_input_va_1, GSOUND_TARGET_ACTION_GA_WILL_PAUSE | GSOUND_TARGET_ACTION_GA_FETCH_PREPARE | GSOUND_TARGET_ACTION_GA_VOICE_PREPARE},
    { ui_input_va_2, 0},
    { ui_input_va_3, GSOUND_TARGET_ACTION_GA_FETCH | GSOUND_TARGET_ACTION_GA_VOICE_CONFIRM},
    { ui_input_va_4, GSOUND_TARGET_ACTION_GA_STOP_ASSISTANT},
    { ui_input_va_5, GSOUND_TARGET_ACTION_GA_VOICE_PTT},
    { ui_input_va_6, GSOUND_TARGET_ACTION_GA_VOICE_DONE}
};

static const va_event_translation_t three_button_va_event_translations[] =
{
    { ui_input_va_1, GSOUND_TARGET_ACTION_GA_WILL_PAUSE | GSOUND_TARGET_ACTION_GA_VOICE_PREPARE},
    { ui_input_va_2, GSOUND_TARGET_ACTION_GA_FETCH_PREPARE},
    { ui_input_va_3, GSOUND_TARGET_ACTION_GA_FETCH | GSOUND_TARGET_ACTION_GA_VOICE_CONFIRM},
    { ui_input_va_4, GSOUND_TARGET_ACTION_TOGGLE_PLAY_PAUSE | GSOUND_TARGET_ACTION_GA_STOP_ASSISTANT},
    { ui_input_va_5, GSOUND_TARGET_ACTION_GA_VOICE_PTT},
    { ui_input_va_6, GSOUND_TARGET_ACTION_GA_VOICE_DONE}
};

static const va_event_translation_t five_button_va_event_translations[] =
{
    { ui_input_va_1, GSOUND_TARGET_ACTION_GA_WILL_PAUSE | GSOUND_TARGET_ACTION_GA_FETCH_PREPARE | GSOUND_TARGET_ACTION_GA_VOICE_PREPARE},
    { ui_input_va_2, 0},
    { ui_input_va_3, GSOUND_TARGET_ACTION_TOGGLE_PLAY_PAUSE | GSOUND_TARGET_ACTION_GA_STOP_ASSISTANT},
    { ui_input_va_4, GSOUND_TARGET_ACTION_GA_FETCH | GSOUND_TARGET_ACTION_GA_VOICE_CONFIRM},
    { ui_input_va_5, GSOUND_TARGET_ACTION_GA_VOICE_PTT},
    { ui_input_va_6, GSOUND_TARGET_ACTION_GA_VOICE_DONE}
};

static const va_event_translation_t reduced_one_button_va_event_translations[] =
{
    { ui_input_va_1, GSOUND_TARGET_ACTION_GA_WILL_PAUSE | GSOUND_TARGET_ACTION_GA_FETCH_PREPARE | GSOUND_TARGET_ACTION_GA_VOICE_PREPARE},
    { ui_input_va_2, 0},
    { ui_input_va_3, GSOUND_TARGET_ACTION_TOGGLE_PLAY_PAUSE | GSOUND_TARGET_ACTION_GA_STOP_ASSISTANT},
    { ui_input_va_4, 0},
    { ui_input_va_5, GSOUND_TARGET_ACTION_GA_VOICE_PTT_OR_FETCH},
    { ui_input_va_6, GSOUND_TARGET_ACTION_GA_VOICE_DONE}
};

static va_translation_table_t va_translation_tables[] =
{
    {one_button_va_event_translations,          ARRAY_DIM(one_button_va_event_translations)},
    {three_button_va_event_translations,        ARRAY_DIM(three_button_va_event_translations)},
    {five_button_va_event_translations,         ARRAY_DIM(five_button_va_event_translations)},
    {reduced_one_button_va_event_translations,  ARRAY_DIM(reduced_one_button_va_event_translations)}
};

#define NUM_VA_TRANSLATION_TABLES ARRAY_DIM(va_translation_tables)
#define NUM_UI_TRANSLATIONS (sizeof(ui_translation_table)/sizeof(ui_translation_table[0]))

static void gaa_SendActionIndicationToAudioTask(GSoundActionEvents action)
{
    static Task gaa_audio_task = NULL;
    gaa_audio_task = Gaa_GetAudioTask();
    
    if(action == GSOUND_TARGET_ACTION_GA_VOICE_PTT)
        MessageSend(gaa_audio_task, GAA_INTERNAL_START_QUERY_IND, NULL);

    if(action == GSOUND_TARGET_ACTION_GA_VOICE_DONE)
        MessageSend(gaa_audio_task, GAA_INTERNAL_END_QUERY_IND, NULL);
#ifdef ENABLE_TYM_PLATFORM        
    if(action & GSOUND_TARGET_ACTION_GA_STOP_ASSISTANT)
    {
        MessageSend(gaa_audio_task, GAA_INTERNAL_STOP_ASSISTANT, NULL);
    }    
#endif          
}

static void gaa_SetVaActionsTranslationTable(uint8 translation_id)
{
     if(translation_id < NUM_VA_TRANSLATION_TABLES)
        va_translation_table = va_translation_tables[translation_id];
     else
         Panic();
}

static void gaa_SendUiInputEvent(ui_input_t ui_event,uint32 delay)
{
    if(inject_unhandled_ui_events)
        inject_unhandled_ui_events(ui_event,delay);
}

static bool gaa_UiEvents2actionEvents(ui_input_t ui_event, GSoundActionEvents *action_event)
{
    bool translated = FALSE;
    uint16 index;

    for (index = 0; (index < NUM_UI_TRANSLATIONS) && !translated; index++)
    {
        if (ui_event == ui_translation_table[index].ui_event)
        {
            *action_event = ui_translation_table[index].action_event;
            translated = TRUE;
        }
    }

    return translated;
}

static bool gaa_ActionEvents2UiEvents(GSoundActionEvents action_event, ui_input_t *ui_event)
{
    bool translated = FALSE;
    uint16 index;

    for (index = 0; (index < NUM_UI_TRANSLATIONS) && !translated; index++)
    {
        if (action_event == ui_translation_table[index].action_event)
        {
            *ui_event = ui_translation_table[index].ui_event;
            translated = TRUE;
        }
    }

    return translated;
}

static void gaa_HandleUiEvent(ui_input_t ui_input, uint32 delay)
{
    GSoundActionEvents action_event;

    bool handled = gaa_UiEvents2actionEvents(ui_input, &action_event);

    if(handled)
    {
        DEBUG_LOG("gaa_HandleUiEvent sending action %d", action_event);
        handled = (action_handlers->gsound_action_on_event(action_event,NULL) == GSOUND_STATUS_OK);
    }
    else
        gaa_SendUiInputEvent(ui_input,delay);
}

bool Gaa_HandleVaEvent(ui_input_t voice_assistant_user_event)
{
    GSoundActionEvents action_event;
    uint16 index;
    bool handled = FALSE;

    for (index = 0; (index < va_translation_table.num_translations); index++)
    {
        if (voice_assistant_user_event == va_translation_table.event_translations[index].voice_assistant_user_event)
        {
            action_event = va_translation_table.event_translations[index].action_event;
            DEBUG_LOG("Gaa_HandleVaEvent sending action %d", action_event);
            handled = (action_handlers->gsound_action_on_event(action_event, NULL) == GSOUND_STATUS_OK);
#ifdef ENABLE_TYM_PLATFORM
            if(action_event == (GSOUND_TARGET_ACTION_TOGGLE_PLAY_PAUSE | GSOUND_TARGET_ACTION_GA_STOP_ASSISTANT))
            {
                if(VoiceUi_IsVoiceAssistantA2dpStreamActive())
                {
                    DEBUG_LOG("IT IS GSOUND_TARGET_ACTION_GA_STOP_ASSISTANT");
                }
                else
                {
                    DEBUG_LOG("IT IS GSOUND_TARGET_ACTION_TOGGLE_PAUSE/PLAY ");
                    if(VoiceUi_GetAmbientTrigger() == TRUE)
                    {
                        DEBUG_LOG("GSOUND_TARGET_ACTION_TOGGLE_PAUSE ui_input_bell_ui_pp_ambient");
                        LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_bell_ui_pp_ambient);
                    }
                }
            }
#endif

            if(handled)
                gaa_SendActionIndicationToAudioTask(action_event);
        }
    }

    return handled;
}

GSoundStatus GSoundTargetActionInit(const GSoundActionInterface *handlers)
{
    action_handlers = handlers;
    gaa_SetVaActionsTranslationTable(Gaa_GetActionMapping());
    inject_unhandled_ui_events = Ui_RegisterUiInputsInterceptor(gaa_HandleUiEvent);

    return GSOUND_STATUS_OK;
}

GSoundStatus GSoundTargetActionRejected(GSoundActionMask rejected_action)
{
    GSoundStatus status = GSOUND_STATUS_OK;
    uint32 delay = 0;
    ui_input_t ui_event;

    DEBUG_LOG("GSoundTargetActionRejected %d", rejected_action);
    bool handled = gaa_ActionEvents2UiEvents(rejected_action, &ui_event);

    if (handled)
        gaa_SendUiInputEvent(ui_event,delay);
    else
        status = GSOUND_STATUS_ERROR;

    return status;
}
