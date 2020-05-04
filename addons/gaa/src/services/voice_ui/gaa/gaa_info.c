/*!
\copyright  Copyright (c) 2018-2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief	    Accessing GAA relevant information from GSound and from non-volatile memory
\note       Implementation for GSoundTargetInfoGetAppVersion() in gaa_ota.c
*/

#include <byte_utils.h>
#include <ps.h>

#include "gsound_target_info.h"
#include "gaa_private.h"
#include "gaa_config.h"

#define PSKEY_USB_SERIAL_NUMBER_STRING (0x02C3)

bool Gaa_GetModelId(uint32 *model_id)
{
    bool ok = FALSE;

    if ((model_id != NULL))
    {
        *model_id = (uint32) gaa_TargetModelId();
        ok = TRUE;
    }

    return ok;
}

uint8 Gaa_GetActionMapping(void)
{
    return gaa_ActionMapping();
}

GSoundStatus GSoundTargetInfoGetSerial(uint8_t *serial_num, uint32_t max_len)
{
    GSoundStatus status = GSOUND_STATUS_ERROR;
    uint16 ps_len = PsFullRetrieve(PSKEY_USB_SERIAL_NUMBER_STRING, NULL, 0);
    uint16 sn_len = sizeof (uint16) * ps_len;
    
    if ((serial_num != NULL) && (max_len > 0) && (sn_len < max_len))
    {
        PsFullRetrieve(PSKEY_USB_SERIAL_NUMBER_STRING, serial_num, ps_len);
        serial_num[sn_len] = '\0';
        status = GSOUND_STATUS_OK;
    }

    return status;
}

GSoundStatus GSoundTargetInfoGetDeviceId(uint32_t *device_id)
{
    GSoundStatus status = GSOUND_STATUS_ERROR;
    uint32 model_id;

    if ((device_id != NULL) && Gaa_GetModelId(&model_id))
    {
        *device_id = model_id;
        status = GSOUND_STATUS_OK;
    }

    return status;
}
