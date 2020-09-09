/*!
\copyright  Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      GAA implementation of voice assistant interface
*/

#include <logging.h>

#include "gatt_server_gap.h"
#include "gaa_private.h"
#include "gaa.h"
#include "gaa_connect_state.h"
#include "gsound_service.h"
#include "voice_ui_container.h"
#ifdef ENABLE_TYM_PLATFORM
#include "logical_input_switch.h"
#endif
static void gaa_EventHandler(ui_input_t event_id);
static void gaa_Suspend(void);
static void gaa_DeselectVoiceAssistant(void);
static void gaa_SelectVoiceAssistant(void);

static voice_ui_if_t voice_assistant_interface =
{
    voice_ui_provider_gaa,
    gaa_EventHandler,
    gaa_Suspend,
    gaa_DeselectVoiceAssistant,
    gaa_SelectVoiceAssistant
};

static voice_ui_protected_if_t *voice_ui_protected_if;

static void gaa_DeselectVoiceAssistant(void)
{
    GSoundServiceDisable();
}

static void gaa_SelectVoiceAssistant(void)
{
    /* Do not need to explicitly call GSoundServiceEnable as enabling has to be
     * completed through user interaction with the handset, which itself invokes enable.
     * As GSoundServiceEnable causes GAA lib to invoke enable callback, and enable
     * callback invokes this functions, this will also introduce recursive function calls */
}


#ifdef ENABLE_TYM_PLATFORM
static void gaa_Notify(void)
{
    DEBUG_LOG("gaa_NotifyBisto");
    /* Simulates a "Button Down -> Button Up -> Single Press Detected" sequence
    for the default configuration of a dedicated VA button */
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_va_1);
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_va_6);
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_va_2);
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_va_3);
}

static void gaa_Cancel(void)
{
    DEBUG_LOG("gaa_CancelBisto");
    /* Simulates a "Button Down -> Button Up -> Button Down -> Button Up -> Double Press Detected"
    sequence for the default configuration of a dedicated VA button */
    //LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_va_1);
    //LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_va_6);
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_va_4);
}

static void gaa_PressHold(void)
{
    DEBUG_LOG("gaa_PressBisto");
    /* Simulates a "Button Down -> Hold" sequence for the default configuration
    of a dedicated VA button */
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_va_1);
    LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_va_5);
}
#endif

static void gaa_EventHandler(ui_input_t event_id)
{
    DEBUG_LOG("gaa_EventHandler event_id %u", event_id);
#ifdef ENABLE_TYM_PLATFORM
    if(event_id >= ui_input_va_notify)
    {
        if(event_id == ui_input_va_notify)
        {
            gaa_Notify();
        }
        else if(event_id == ui_input_va_cancel)
        {
            gaa_Cancel();
            /* clean amibent bit */
            VoiceUi_SetAmbientTrigger(FALSE);
        }
        else if(event_id == ui_input_va_cancel_ambient)
        {
            gaa_Cancel();
            /* set amibent bit */
            VoiceUi_SetAmbientTrigger(TRUE);
        }
        else if(event_id == ui_input_va_presshold)
        {
            gaa_PressHold();
        }
        return;
    }
#endif
    if (!Gaa_HandleVaEvent(event_id))
        DEBUG_LOG("gaa_EventHandler unhandled event %u", event_id);
}

static void gaa_Suspend(void)
{
    DEBUG_LOG("gaa_Suspend");
    Gaa_AudioInClose();
}



bool Gaa_Init(Task init_task)
{
    bool gaa_enabled = VoiceUi_GetSelectedAssistant() == voice_ui_provider_gaa;
    UNUSED(init_task);
    DEBUG_LOG("Gaa_Init");
    Gaa_ConnectionStateInit();
    voice_ui_protected_if = VoiceUi_Register(&voice_assistant_interface);
    Gaa_InitialiseService(gaa_enabled);
    Gaa_BleRegisterAdvertising();
    GattServerGap_UseCompleteLocalName(TRUE);
    return TRUE;
}

voice_ui_protected_if_t *Gaa_GetVoiceUiProtectedInterface(void)
{
    return voice_ui_protected_if;
}

