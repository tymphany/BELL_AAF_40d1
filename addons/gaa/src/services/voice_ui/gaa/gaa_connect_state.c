/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Declarations for gaa connection state public functions.
*/


#include "gsound.h"
#include "gaa_connect_state.h"
#include "gaa_private.h"

static bool channel_connected[GSOUND_NUM_CHANNEL_TYPES];

/*********************************************************************/
void Gaa_ConnectionStateInit(void)
{
    channel_connected[GSOUND_CHANNEL_CONTROL] = FALSE;
    channel_connected[GSOUND_CHANNEL_AUDIO] = FALSE;
}
bool Gaa_IsTotallyConnected(void)
{
    return (channel_connected[GSOUND_CHANNEL_CONTROL] 
            && channel_connected[GSOUND_CHANNEL_AUDIO]);
}

bool Gaa_IsPartiallyConnected(void)
{
    return (channel_connected[GSOUND_CHANNEL_CONTROL] 
            != channel_connected[GSOUND_CHANNEL_AUDIO]);
}

bool Gaa_IsTotallyDisconnected(void)
{
    return (channel_connected[GSOUND_CHANNEL_CONTROL] == FALSE 
            && channel_connected[GSOUND_CHANNEL_AUDIO] == FALSE);
}

void Gaa_SetChannelConnected(GSoundChannelType channel, bool connected)
{
    /* Ignore channel if out of range. */
    if (channel < GSOUND_NUM_CHANNEL_TYPES)
    {
        /* Ignore if already at that value; only process changes. */
        if (channel_connected[channel] != connected)
        {
            channel_connected[channel] = connected;
        }
    }
}

