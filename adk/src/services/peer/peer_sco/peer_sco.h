/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for the controlling peer sco (forward or mirroring).
            Abstracts the difference between SCO forwarding and SCO
            mirroring behind a simple API.
*/

#ifndef PEER_SCO_H_
#define PEER_SCO_H_

#if defined(INCLUDE_SCOFWD)

#include "scofwd_profile.h"

#define PeerSco_Enable()  ScoFwdEnableForwarding()
#define PeerSco_Disable() ScoFwdDisableForwarding()
#define PeerSco_IsActive()  ScoFwdIsSending()

#endif

#endif PEER_SCO_H_
