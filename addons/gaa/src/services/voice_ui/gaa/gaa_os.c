/*!
\copyright  Copyright (c) 2018-2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation for GAA OS. Refer to gsound_os.h for API information.
*/

#include <vm.h>

#include "gsound_target.h"


uint32_t GSoundTargetOsIntLock(void)
{
    return  0;
}

void GSoundTargetOsIntRevert(uint32_t int_status)
{
    UNUSED(int_status);
}

uint32_t GSoundTargetOsGetTicksMs(void)
{
    return VmGetClock();
}
