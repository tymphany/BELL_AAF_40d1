/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Declarations for gaa connection state public functions.
*/


#include "gsound.h"

/*********************************************************************/
void Gaa_ConnectionStateInit(void);
bool Gaa_IsTotallyConnected(void);
bool Gaa_IsPartiallyConnected(void);
bool Gaa_IsTotallyDisconnected(void);
void Gaa_SetChannelConnected(GSoundChannelType channel, bool connected);

