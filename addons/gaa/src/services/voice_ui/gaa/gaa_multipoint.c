/*!
\copyright  Copyright (c) 2018-2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief	    Implementation for GAA multipoint support
\note       'Secondary': In multipoint mode the partner who provides the library we interface with refers to 'secondary' as the BT connection that does NOT have an active GA
            'Primary': In multipoint mode the partner who provides the library we interface with refers to 'primary' as the BT connection that DOES have an active GA
*/

#include "gaa_private.h"
#include "gsound_target.h"

/* Note: In this case 'secondary' refers to the BT connection in a multi-point scenario that does NOT have an active GA
 * Note: gaa_addr is the address of the active GA (primary) device
 * Note: When gaa device is the routed device, GA sessions are correctly handled without intervention */
GSoundStatus GSoundTargetBtPauseSecondary(const GSoundBTAddr *gaa_addr)
{
    UNUSED(gaa_addr);
    return GSOUND_STATUS_OK;
}

/* Note: In this case 'secondary' refers to the BT connection in a multi-point scenario that does NOT have an active GA
 * Note: gaa_addr is the address of the active GA (primary) device
 * Note: When gaa device is the routed device, GA sessions are correctly handled without intervention */
GSoundStatus GSoundTargetBtResumeSecondary(const GSoundBTAddr *gaa_addr)
{
    UNUSED(gaa_addr);
    MessageSend(Gaa_GetAudioTask(), GAA_INTERNAL_END_RESPONSE_IND, NULL);
    return GSOUND_STATUS_OK;
}
