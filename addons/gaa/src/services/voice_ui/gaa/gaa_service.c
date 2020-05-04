/*!
\copyright  Copyright (c) 2018-2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief	    Initialisation for the GAA service
*/

#include <logging.h>
#include "gsound_service.h"
#include "gsound_target.h"
#include "gaa_pmalloc_pools.h"
#include "gaa_private.h"
#include "gaa_config.h"

static void gaa_NotifyServiceDisabled(void);
static void gaa_NotifyServiceEnabled(void);

const static GSoundServiceObserver gsound_observer =
{
    .on_gsound_disabled = &gaa_NotifyServiceDisabled,
    .on_gsound_enabled =  &gaa_NotifyServiceEnabled
};

static void gaa_NotifyServiceDisabled(void)
{
    DEBUG_LOG("gaa_NotifyServiceDisabled");
}

static void gaa_NotifyServiceEnabled(void)
{
    DEBUG_LOG("gaa_NotifyServiceEnabled");
    VoiceUi_SelectVoiceAssistant(voice_ui_provider_gaa);
}

void Gaa_InitialiseService(bool gaa_enabled)
{
    GSoundServiceConfig gsound_config;
    gsound_config.gsound_enabled = gaa_enabled;

    DEBUG_LOG("Gaa_InitialiseService enable %u", gsound_config.gsound_enabled);

    Gaa_InitPmallocPools();
    GSoundServiceInit(GSOUND_BUILD_ID, &gsound_config, &gsound_observer);
    Gaa_InitialiseChannels();

    if (gaa_IsTwsBuild())
    {
        DEBUG_LOG("gaa Init as TWS operation");
        GSoundServiceInitAsTws();
    }
    else
    {
        DEBUG_LOG("gaa Init as Stereo operation");
        GSoundServiceInitAsStereo();
    }
}
