/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for the gaia framework API
*/

#include "gaia_framework.h"

#include <logging.h>
#include <panic.h>

#include "gaia_framework_command.h"
#include "gaia_framework_feature.h"
#include "gaia_core_plugin.h"
#ifdef ENABLE_TYM_PLATFORM
#include "ui.h"
#include "kymera_aec.h"
#include "kymera_private.h"
#include "earbud_tym_gaia.h"
#endif


bool GaiaFramework_Init(Task init_task)
{
    DEBUG_LOG("GaiaFramework_Init");

    GaiaFrameworkCommand_ResetVendorSpecificHandler();

    GaiaFrameworkFeature_Init();

    GaiaCorePlugin_Init();

    return GaiaFrameworkInternal_Init(init_task);
}

void GaiaFramework_RegisterFeature(uint8 feature_id, uint8 version_number, gaia_framework_command_handler_fn_t command_handler, gaia_framework_send_all_notifications_fn_t send_notifications)
{
    DEBUG_LOG("GaiaFramework_RegisterFeature");

    PanicFalse(GaiaFrameworkFeature_AddToList(feature_id, version_number, command_handler, send_notifications));
}

void GaiaFramework_RegisterVendorSpecificHandler(gaia_framework_vendor_specific_handler_fn_t command_handler)
{
    DEBUG_LOG("GaiaFramework_RegisterVendorSpecificHandler");

    PanicFalse(GaiaFrameworkCommand_RegisterVendorSpecificHandler(command_handler));
}

void GaiaFramework_SendResponse(uint8 feature_id, uint8 pdu_id, uint8 length, uint8 * payload)
{
    GAIA_TRANSPORT *transport = GaiaGetTransport();
    uint16 command_id;

    DEBUG_LOG("GaiaFramework_SendResponse");

    command_id = gaiaFramework_BuildCommandId(feature_id, pdu_type_response, pdu_id);

    GaiaBuildAndSendSynch(transport, GAIA_V3_VENDOR_ID, command_id, GAIA_STATUS_NONE, length, payload);
}

void GaiaFramework_SendError(uint8 feature_id, uint8 pdu_id, uint8 status_code)
{
    GAIA_TRANSPORT *transport = GaiaGetTransport();
    uint16 command_id;
    DEBUG_LOG("GaiaFramework_SendError");

    command_id = gaiaFramework_BuildCommandId(feature_id, pdu_type_error, pdu_id);

    GaiaBuildAndSendSynch(transport, GAIA_V3_VENDOR_ID, command_id, GAIA_STATUS_NONE, 1, &status_code);
}

void GaiaFramework_SendNotification(uint8 feature_id, uint8 notification_id, uint8 length, uint8 * payload)
{
    GAIA_TRANSPORT *transport = GaiaGetTransport();
    uint16 command_id;

    DEBUG_LOG("GaiaFramework_SendNotification");

    if (GaiaFrameworkFeature_IsNotificationsActive(feature_id))
    {
        command_id = gaiaFramework_BuildCommandId(feature_id, pdu_type_notification, notification_id);

        GaiaBuildAndSendSynch(transport, GAIA_V3_VENDOR_ID, command_id, GAIA_STATUS_NONE, length, payload);
    }
}

#ifdef ENABLE_TYM_PLATFORM
bool tym_gaia_is_acknowledgement(uint16 command_id)
{
    return (command_id & GAIA_ACK_MASK) != 0;
}

void tym_gaia_send_simple_response(uint16 command_id,uint8 status)
{
    GAIA_TRANSPORT *transport = GaiaGetTransport();
    GaiaBuildAndSendSynch(transport, BELL_VENDOR_QTIL, command_id | GAIA_ACK_MASK, status, 0, NULL);
}

void tym_gaia_send_response(uint16 command_id,uint8 status,uint16 payload_length, uint8 *payload)
{
    GAIA_TRANSPORT *transport = GaiaGetTransport();
    GaiaBuildAndSendSynch(transport, BELL_VENDOR_QTIL, command_id | GAIA_ACK_MASK, status, payload_length, payload);
}

void tym_gaia_send_notification(uint16 event, uint8 status, uint16 payload_length, uint8 *payload)
{
    GAIA_TRANSPORT *transport = GaiaGetTransport();
    GaiaBuildAndSendSynch(transport, BELL_VENDOR_QTIL, event | GAIA_ACK_MASK, status, payload_length, payload);
}

void gaia_send_application_version(uint16 vendor_id)
{
    GAIA_TRANSPORT *transport = GaiaGetTransport();
    uint16 major,minor;
    /*  Read Device ID from config, irrespective of DEVICE_ID_PSKEY  */
    uint8 payload[8];
    uint16 payload_length;
    UpgradeGetVersion(&major, &minor, NULL);
    memset(payload,0x0,sizeof(payload));
    payload[0] = (major >> 8);
    payload[1] = (major & 0xff);
    payload[2] = (minor >> 8);
    payload[3] = (minor & 0xff);
    DEBUG_LOG("payload 0x%x 0x%x 0x%x 0x%x\n",payload[0],payload[1],payload[2],payload[3]);
    payload_length = 4;

    GaiaBuildAndSendSynch(transport, vendor_id, GAIA_COMMAND_GET_APPLICATION_VERSION | GAIA_ACK_MASK, GAIA_STATUS_SUCCESS, payload_length, payload);
}

void tym_send_switch_eq_preset(uint16 vendor_id, uint8 size_payload, uint8* payload)
{
    #define   TYM_SWITCH_EQ_COMMAND_PAY_LOAD_SIZE (1)
    #define   MAX_EQ_PRESET                       (UCID_USER_EQ_BANK5)
    GAIA_TRANSPORT *transport = GaiaGetTransport();
    uint8 package[8];
    uint16 package_length;
    memset(package,0x0,sizeof(package));

    if(size_payload == 1){
        if(*payload <= MAX_EQ_PRESET)
        {
            Ui_InjectUiInput(ui_input_bell_ui_switch_preset_bank0 + *(payload));
            package[0] = *(payload);
        }else{
            package[0] = get_cur_preset_eq();
        }
    }

    package_length = TYM_SWITCH_EQ_COMMAND_PAY_LOAD_SIZE;
    GaiaBuildAndSendSynch(transport, vendor_id, GAIA_COMMAND_SWITCH_EQ_CONTROL | GAIA_ACK_MASK, GAIA_STATUS_SUCCESS, package_length, package);
}

bool appTYMHandleControlCommand(Task task, const GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    UNUSED(task);
    switch (command->command_id)
    {
    case GAIA_COMMAND_SWITCH_EQ_CONTROL:
        DEBUG_LOG("gaia send switch eq control");
        tym_send_switch_eq_preset(command->vendor_id, command->size_payload, command->payload );
        return TRUE;
    default:
        DEBUG_LOG("appTYMHandleControlCommand Unhandled GAIA_COMMAND 0x%x (%d)",command->command_id,command->command_id);
        return FALSE;
    }
}
#endif
