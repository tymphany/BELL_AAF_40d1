/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation for GAA reset functions
*/


#include <vm.h>

#include "gsound_target_reset.h"

void GSoundTargetGetCrashDump(void (*callback)(const GSoundCrashFatalDump *gsound_crash))
{
    GSoundCrashFatalDump dump;

    memset(&dump, 0, sizeof dump);
    dump.reason = GSOUND_RESET_TYPE_NORMAL;

    callback(&dump);
}


GSoundStatus GSoundTargetGetResetReason(GSoundResetReason *gsound_reset)
{
    GSoundStatus status = GSOUND_STATUS_ERROR;

    if (gsound_reset)
    {
        GSoundResetReason reason = GSOUND_RESET_TYPE_UNKNOWN;
        vm_reset_source source = VmGetResetSource();

    /*  These cases are exhaustive so that adding to enum vm_reset_source
     *  will cause a MissingCase compiler warning.
     */
        switch (source)
        {
        case RESET_SOURCE_POWER_ON:
        case RESET_SOURCE_CHARGER:
        case RESET_SOURCE_TOOLCMD_FULL_RESET:
        case RESET_SOURCE_TOOLCMD_SUBSYS_RESET:
            reason = GSOUND_RESET_TYPE_NORMAL;
            break;

        case RESET_SOURCE_DEBUG_RESET:
            reason = GSOUND_RESET_TYPE_ASSERT;
            break;

        case RESET_SOURCE_FIRMWARE:
        case RESET_SOURCE_PANIC:
        case RESET_SOURCE_APP_SUBSYS_RESET:
        case RESET_SOURCE_APP_PANIC:
        case RESET_SOURCE_HOST_RESET:
        case RESET_SOURCE_TBRIDGE_RESET:
        case RESET_SOURCE_UART_BREAK:
        case RESET_SOURCE_SDIO:
            reason = GSOUND_RESET_TYPE_EXCEPTION;
            break;

        case RESET_SOURCE_APP_WATCHDOG:
        case RESET_SOURCE_WATCHDOG:
        case RESET_SOURCE_DORMANT_WAKEUP:
            reason = GSOUND_RESET_WATCHDOG;
            break;

        case UNEXPECTED_RESET:
        case UNEXPECTED_RESET_REASON_RECEIVED:
            reason = GSOUND_RESET_TYPE_UNKNOWN;
            break;
        }

        *gsound_reset = reason;
        status = GSOUND_STATUS_OK;
    }

    return status;
}
