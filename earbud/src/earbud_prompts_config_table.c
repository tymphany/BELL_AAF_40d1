/*!
\copyright  Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief	    Earbud Prompts UI Indicator configuration table
*/
#include "earbud_prompts_config_table.h"

#include <domain_message.h>
#include <ui_prompts.h>

#include <av.h>
#include <pairing.h>
#include <telephony_messages.h>
#include <power_manager.h>
#include <voice_ui.h>

#ifdef INCLUDE_PROMPTS
#ifdef ENABLE_TYM_PLATFORM
const ui_event_indicator_table_t earbud_ui_prompts_table[] =
{
    {.sys_event=PROMPT_POWER_ON,          {.prompt.filename = "07.Power_ON.sbc",
                                           .prompt.rate = 44100,
                                           .prompt.format = PROMPT_FORMAT_SBC,
                                           .prompt.interruptible = FALSE,
                                           .prompt.queueable = TRUE }},
    {.sys_event=PROMPT_POWER_OFF,         {.prompt.filename = "06.Power_OFF.sbc",
                                           .prompt.rate = 44100,
                                           .prompt.format = PROMPT_FORMAT_SBC,
                                           .prompt.interruptible = FALSE,
                                           .prompt.queueable = TRUE }},
    {.sys_event=PROMPT_PAIRING,           {.prompt.filename = "05.Pairing.sbc",
                                           .prompt.rate = 44100,
                                           .prompt.format = PROMPT_FORMAT_SBC,
                                           .prompt.interruptible = FALSE,
                                           .prompt.queueable = TRUE }},
    {.sys_event=PROMPT_PAIRING_SUCCESSFUL,{.prompt.filename = "04.Paired.sbc",
                                           .prompt.rate = 44100,
                                           .prompt.format = PROMPT_FORMAT_SBC,
                                           .prompt.interruptible = FALSE,
                                           .prompt.queueable = TRUE }},
    {.sys_event=PROMPT_PAIRING_FAILED,    {.prompt.filename = "11.Unpaired.sbc",
                                           .prompt.rate = 44100,
                                           .prompt.format = PROMPT_FORMAT_SBC,
                                           .prompt.interruptible = FALSE,
                                           .prompt.queueable = TRUE }},
    {.sys_event=PROMPT_CONNECTED,         {.prompt.filename = "04.Paired.sbc",
                                           .prompt.rate = 44100,
                                           .prompt.format = PROMPT_FORMAT_SBC,
                                           .prompt.interruptible = FALSE,
                                           .prompt.queueable = TRUE }},
    {.sys_event=PROMPT_DISCONNECTED,      {.prompt.filename = "11.Unpaired.sbc",
                                           .prompt.rate = 44100,
                                           .prompt.format = PROMPT_FORMAT_SBC,
                                           .prompt.interruptible = FALSE,
                                           .prompt.queueable = TRUE }},
    {.sys_event=PROMPT_BATTERY_LOW,       {.prompt.filename = "01.Battery_Level_Low.sbc",
                                           .prompt.rate = 44100,
                                           .prompt.format = PROMPT_FORMAT_SBC,
                                           .prompt.interruptible = FALSE,
                                           .prompt.queueable = TRUE }},
    {.sys_event=PROMPT_FINDME,            {.prompt.filename = "02.FindMyEarbud_MONO.sbc",
                                           .prompt.rate = 44100,
                                           .prompt.format = PROMPT_FORMAT_SBC,
                                           .prompt.interruptible = FALSE,
                                           .prompt.queueable = TRUE }},
    {.sys_event=PROMPT_MAX_VOLUME,        {.prompt.filename = "03.Max_Volume_Tone.sbc",
                                           .prompt.rate = 44100,
                                           .prompt.format = PROMPT_FORMAT_SBC,
                                           .prompt.interruptible = FALSE,
                                           .prompt.queueable = TRUE }},
    {.sys_event=PROMPT_QuickAttention_OFF,{.prompt.filename = "08.Quick_Attention_OFF.sbc",
                                           .prompt.rate = 44100,
                                           .prompt.format = PROMPT_FORMAT_SBC,
                                           .prompt.interruptible = FALSE,
                                           .prompt.queueable = TRUE }},
    {.sys_event=PROMPT_QuickAttention_ON, {.prompt.filename = "09.Quick_Attention_ON.sbc",
                                           .prompt.rate = 44100,
                                           .prompt.format = PROMPT_FORMAT_SBC,
                                           .prompt.interruptible = FALSE,
                                           .prompt.queueable = TRUE }},
    {.sys_event=PROMPT_ROLE_SWITCH,       {.prompt.filename = "10.Role_Switch.sbc",
                                           .prompt.rate = 44100,
                                           .prompt.format = PROMPT_FORMAT_SBC,
                                           .prompt.interruptible = FALSE,
                                           .prompt.queueable = TRUE }},
    {.sys_event=PROMPT_AMBIENT_OFF,       {.prompt.filename = "12.Ambient_OFF.sbc",
                                           .prompt.rate = 44100,
                                           .prompt.format = PROMPT_FORMAT_SBC,
                                           .prompt.interruptible = FALSE,
                                           .prompt.queueable = TRUE }},
    {.sys_event=PROMPT_AMBIENT_ON,        {.prompt.filename = "13.Ambient_ON.sbc",
                                           .prompt.rate = 44100,
                                           .prompt.format = PROMPT_FORMAT_SBC,
                                           .prompt.interruptible = FALSE,
                                           .prompt.queueable = TRUE }},
    {.sys_event=PROMPT_ANC_OFF,           {.prompt.filename = "14.ANC_OFF.sbc",
                                           .prompt.rate = 44100,
                                           .prompt.format = PROMPT_FORMAT_SBC,
                                           .prompt.interruptible = FALSE,
                                           .prompt.queueable = TRUE }},
    {.sys_event=PROMPT_ANC_ON,            {.prompt.filename = "15.ANC_ON.sbc",
                                           .prompt.rate = 44100,
                                           .prompt.format = PROMPT_FORMAT_SBC,
                                           .prompt.interruptible = FALSE,
                                           .prompt.queueable = TRUE }},
    {.sys_event=PROMPT_SPEECH_OFF,        {.prompt.filename = "16.Speech_OFF.sbc",
                                           .prompt.rate = 44100,
                                           .prompt.format = PROMPT_FORMAT_SBC,
                                           .prompt.interruptible = FALSE,
                                           .prompt.queueable = TRUE }},
    {.sys_event=PROMPT_SPEECH_ON,         {.prompt.filename = "17.Speech_ON.sbc",
                                           .prompt.rate = 44100,
                                           .prompt.format = PROMPT_FORMAT_SBC,
                                           .prompt.interruptible = FALSE,
                                           .prompt.queueable = TRUE }},
    {.sys_event=VOICE_UI_MIC_OPEN,      { .prompt.filename = "mic_open.sbc",
                                          .prompt.rate = 16000,
                                          .prompt.format = PROMPT_FORMAT_SBC,
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch,for sync of prompt*/
                                          .prompt.interruptible = FALSE,
#else
                                          .prompt.interruptible = TRUE,
#endif
                                          .prompt.queueable = FALSE }},
    {.sys_event=VOICE_UI_MIC_CLOSE,     { .prompt.filename = "mic_close.sbc",
                                          .prompt.rate = 16000,
                                          .prompt.format = PROMPT_FORMAT_SBC,
#ifdef ENABLE_TYM_PLATFORM /*add Qualcomm patch,for sync of prompt*/
                                          .prompt.interruptible = FALSE,
#else
                                          .prompt.interruptible = TRUE,
#endif
                                          .prompt.queueable = FALSE }},
    {.sys_event=VOICE_UI_DISCONNECTED,  { .prompt.filename = "bt_va_not_connected.sbc",
                                          .prompt.rate = 48000,
                                          .prompt.format = PROMPT_FORMAT_SBC,
                                          .prompt.interruptible = FALSE,
                                          .prompt.queueable = TRUE }}
};
#else
const ui_event_indicator_table_t earbud_ui_prompts_table[] =
{
    {.sys_event=POWER_ON,                {.prompt.filename = "power_on.sbc",
                                          .prompt.rate = 48000,
                                          .prompt.format = PROMPT_FORMAT_SBC,
                                          .prompt.interruptible = FALSE,
                                          .prompt.queueable = TRUE },
                                          .await_indication_completion = TRUE },
    {.sys_event=POWER_OFF,              { .prompt.filename = "power_off.sbc",
                                          .prompt.rate = 48000,
                                          .prompt.format = PROMPT_FORMAT_SBC,
                                          .prompt.interruptible = FALSE,
                                          .prompt.queueable = TRUE },
                                          .await_indication_completion = TRUE },
    {.sys_event=PAIRING_ACTIVE,         { .prompt.filename = "pairing.sbc",
                                          .prompt.rate = 48000,
                                          .prompt.format = PROMPT_FORMAT_SBC,
                                          .prompt.interruptible = FALSE,
                                          .prompt.queueable = TRUE }},
    {.sys_event=PAIRING_COMPLETE,       { .prompt.filename = "pairing_successful.sbc",
                                          .prompt.rate = 48000,
                                          .prompt.format = PROMPT_FORMAT_SBC,
                                          .prompt.interruptible = FALSE,
                                          .prompt.queueable = TRUE }},
    {.sys_event=PAIRING_FAILED,         { .prompt.filename = "pairing_failed.sbc",
                                          .prompt.rate = 48000,
                                          .prompt.format = PROMPT_FORMAT_SBC,
                                          .prompt.interruptible = FALSE,
                                          .prompt.queueable = TRUE }},
    {.sys_event=TELEPHONY_CONNECTED,    { .prompt.filename = "connected.sbc",
                                          .prompt.rate = 48000,
                                          .prompt.format = PROMPT_FORMAT_SBC,
                                          .prompt.interruptible = FALSE,
                                          .prompt.queueable = TRUE }},
    {.sys_event=TELEPHONY_DISCONNECTED, { .prompt.filename = "disconnected.sbc",
                                          .prompt.rate = 48000,
                                          .prompt.format = PROMPT_FORMAT_SBC,
                                          .prompt.interruptible = FALSE,
                                          .prompt.queueable = TRUE }},
    {.sys_event=VOICE_UI_MIC_OPEN,      { .prompt.filename = "mic_open.sbc",
                                          .prompt.rate = 16000,
                                          .prompt.format = PROMPT_FORMAT_SBC,
                                          .prompt.interruptible = TRUE,
                                          .prompt.queueable = FALSE }},
    {.sys_event=VOICE_UI_MIC_CLOSE,     { .prompt.filename = "mic_close.sbc",
                                          .prompt.rate = 16000,
                                          .prompt.format = PROMPT_FORMAT_SBC,
                                          .prompt.interruptible = TRUE,
                                          .prompt.queueable = FALSE }},
    {.sys_event=VOICE_UI_DISCONNECTED,  { .prompt.filename = "bt_va_not_connected.sbc",
                                          .prompt.rate = 48000,
                                          .prompt.format = PROMPT_FORMAT_SBC,
                                          .prompt.interruptible = FALSE,
                                          .prompt.queueable = TRUE }}
};
#endif
#endif

uint8 EarbudPromptsConfigTable_GetSize(void)
{
#ifdef INCLUDE_PROMPTS
    return ARRAY_DIM(earbud_ui_prompts_table);
#else
    return 0;
#endif
}

