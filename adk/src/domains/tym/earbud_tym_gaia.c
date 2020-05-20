/* --------------------------------------------------------------------------
 * ------- Header files--- --------------------------------------------------
 * --------------------------------------------------------------------------
 */
#include <stdio.h>
#include <byte_utils.h>
#include <vmtypes.h>
#include "gaia.h"
#include "logging.h"
#include "ps.h"
#include "connection.h"
#include "earbud_tym_psid.h"
#include "earbud_tym_gaia.h"
#include "local_name.h"
#include "power_manager.h"
#include "gaia_framework.h"
#include "tym_anc.h"
#include "ui.h"
#include "ui_inputs.h"
#include "earbud_tym_sync.h"
#include "audio_curation.h"
#include "phy_state.h"
#include "earbud_config.h"
#include "state_proxy.h"
#include "battery_monitor.h"
#include "peer_signalling.h"
#include "earbud_config.h"
#include "proximity.h"
#include "multidevice.h"
#include "kymera_private.h"
#include "earbud_tym_psid.h"
#include "logical_input_switch.h"
#include "1_button.h"
/* Todo
#include "sink_tym_anc.h"
#include "sink_tym_main.h"
#include "sink_tym_app_interface.h"
#include "sink_tym_tws.h"
#include "sink_tym_config.h"
#include "sink_tym_status_output.h"
#include "ltr572.h"
#include "ltr2678.h"
*/

/* --------------------------------------------------------------------------
 * ------- Private typedef --------------------------------------------------
 * --------------------------------------------------------------------------
 */

 /* --------------------------------------------------------------------------
 * ------- Private define --------------------------------------------------
 * --------------------------------------------------------------------------
 */

/* --------------------------------------------------------------------------
 * ------- Private macro ----------------------------------------------------
 * --------------------------------------------------------------------------
 */
 
/* --------------------------------------------------------------------------
 * ------- Private variables ------------------------------------------------
 * --------------------------------------------------------------------------
 */ 
tym_sync_app_configuration_t app_config_setting; 
/* --------------------------------------------------------------------------
 * ------- Private function prototypes --------------------------------------
 * --------------------------------------------------------------------------
 */ 
void bell_gaia_rename(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_set_anc_state(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_get_anc_state(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_set_ambient_state(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_get_ambient_state(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_set_ambient_level(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_get_ambient_level(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_set_voice_state(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_get_voice_state(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_set_voice_level(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_get_voice_level(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_reset_anc_state(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_get_anc_feature(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_set_autopoweroff(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_get_autopoweroff(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_set_earbudcustom(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_get_earbudcustom(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_set_smartassistant(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_get_smartassistant(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_get_batterylevel(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_get_twsstatus(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_set_autowear(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_get_autowear(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_get_twslinkstate(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_get_psval(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_set_ir_config(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_get_play_pause_status(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_get_fw_version(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_get_serial_number(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_set_findme(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_set_audio_contorl(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_set_eq_control(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_get_eq_contorl(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_no_operation(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_set_auto_play(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_get_auto_play(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_set_ambient_ext_anc(GAIA_UNHANDLED_COMMAND_IND_T *command);
void bell_gaia_get_ambient_ext_anc(GAIA_UNHANDLED_COMMAND_IND_T *command);

 /* -------------------------------------------------------------------------
 * ------- Private functions ------------------------------------------------
 * --------------------------------------------------------------------------
 */
void bell_gaia_rename(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    int max_len = 15,len = command->size_payload;
    //Add rename function at chip init
    if((command->size_payload < 64) && (command->size_payload > 0))
    {
        uint8 btname[64];
        memset(btname, 0x0, sizeof(btname));
        if(len > max_len)
            len = max_len;            
        memcpy(btname, command->payload, len);
        LocalName_ChangeByApp(btname, len);
        /* send response */
        tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_SUCCESS);
        appPowerRebootWaitSec(1);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_set_anc_state(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    if((command->size_payload == 1) && ((command->payload[0] == 0) || (command->payload[0] == 1)))
    {
        if(command->payload[0] == 0)
        {
             Ui_InjectUiInput(ui_input_bell_ui_anc_off);
        }
        else
        {
             Ui_InjectUiInput(ui_input_bell_ui_anc_on);
        }
        /* send response */
        tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_SUCCESS);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_get_anc_state(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    uint16 payload_len = 1;
    uint8 payload[payload_len];
    tymAncTaskData *tymAnc = TymAncGetTaskData();    
    if(command->size_payload == 0)
    {
        if(tymAnc->curAncMode == ancon)
            payload[0] = 1;
        else
            payload[0] = 0;
        tym_gaia_send_response(command->command_id, GAIA_STATUS_SUCCESS, payload_len ,payload);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_set_ambient_state(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    if((command->size_payload == 1) && ((command->payload[0] == 0) || (command->payload[0] == 1)))
    {
        if(command->payload[0] == 0)
        {
             Ui_InjectUiInput(ui_input_bell_ui_ambient_off);
        }
        else
        {
             Ui_InjectUiInput(ui_input_bell_ui_ambient_on);
        }
        /* send response */
        tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_SUCCESS);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_get_ambient_state(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    uint16 payload_len = 1;
    uint8 payload[payload_len];
    tymAncTaskData *tymAnc = TymAncGetTaskData();
    if(command->size_payload == 0)
    {
        if(tymAnc->curAncMode == ambient)
            payload[0] = 1;
        else
            payload[0] = 0;
        tym_gaia_send_response(command->command_id, GAIA_STATUS_SUCCESS, payload_len, payload);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_set_ambient_level(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    uint16 level;
    tymAncTaskData *tymAnc = TymAncGetTaskData();
    if(command->size_payload == 2)
    {
        level = (command->payload[0] << 8) | command->payload[1];
        if(level >=1 && level <=5)
        {    
            tymSyncdata(ambientLevelCmd, level & 0xff); //sync level to two earbud
            if(tymAnc->curAncMode == ambient)
                setupAmbientLevel();
            /* send response */    
            tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_SUCCESS);
        }
        else
        {
            DEBUG_LOG("over level %d",level);
            tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_INVALID_PARAMETER);
        }        
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_INVALID_PARAMETER);
    }

}

void bell_gaia_get_ambient_level(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    uint16 payload_len = 2;
    uint8 payload[payload_len];
    uint16 level;
    tymAncTaskData *tymAnc = TymAncGetTaskData();
    if(command->size_payload == 0)
    {
        level = tymAnc->ambientLevel;
        payload[0] = level >> 8;
        payload[1] = level & 0xff;
        tym_gaia_send_response(command->command_id, GAIA_STATUS_SUCCESS,payload_len , payload);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_set_voice_state(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    if((command->size_payload == 1) && ((command->payload[0] == 0) || (command->payload[0] == 1)))
    {
        if(command->payload[0] == 0)
        {
             Ui_InjectUiInput(ui_input_bell_ui_speech_off);
        }
        else
        {
             Ui_InjectUiInput(ui_input_bell_ui_speech_on);
        }
        /* send response */
        tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_SUCCESS);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_get_voice_state(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    uint16 payload_len = 1;
    uint8 payload[payload_len];
    tymAncTaskData *tymAnc = TymAncGetTaskData();
    if(command->size_payload == 0)
    {
        if(tymAnc->curAncMode == speech)
            payload[0] = 1;
        else
            payload[0] = 0;
        tym_gaia_send_response(command->command_id, GAIA_STATUS_SUCCESS, payload_len, payload);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_set_voice_level(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    uint16 level;
    tymAncTaskData *tymAnc = TymAncGetTaskData();
    if(command->size_payload == 2)
    {
        level = (command->payload[0] << 8) | command->payload[1];
        if(level >=1 && level <=5)
        {
            tymSyncdata(speechLevelCmd, level & 0xff); //sync level to two earbud
            if(tymAnc->curAncMode == speech)
                setupSpeechLevel();
            /* send response */
            tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_SUCCESS);
        }
        else
        {
            DEBUG_LOG("over level %d",level);
            tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_INVALID_PARAMETER);
        }
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_get_voice_level(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    uint16 payload_len = 2;
    uint8 payload[payload_len];
    uint16 level;
    tymAncTaskData *tymAnc = TymAncGetTaskData();
    if(command->size_payload == 0)
    {
        level = tymAnc->speechLevel;
        payload[0] = level >> 8;
        payload[1] = level & 0xff;
        tym_gaia_send_response(command->command_id, GAIA_STATUS_SUCCESS,payload_len , payload);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_reset_anc_state(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    if(command->size_payload == 0)
    {
        Ui_InjectUiInput(ui_input_bell_ui_anc_off);
        /* send response */
        tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_SUCCESS);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_get_anc_feature(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    uint16 payload_len = 1;
    uint8 payload[payload_len];
    tymAncTaskData *tymAnc = TymAncGetTaskData();
    uint8 ancmode = tymAnc->curAncMode;
    if(command->size_payload == 0)
    {
        if(ancmode == ancoff)
            payload[0] = 0x0;
        else if(ancmode == ancon)
            payload[0] = 0x08;
        else if(ancmode == ambient)
            payload[0] = 0x04;
        else if(ancmode == speech)
            payload[0] = 0x02;
        else
            payload[0] = 0x0;
        /* send response */
        tym_gaia_send_response(command->command_id, GAIA_STATUS_SUCCESS, payload_len , payload);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_set_autopoweroff(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    uint8 poweroff_setting = command->payload[0];
    if((command->size_payload == 1) && ((poweroff_setting == 0) || (poweroff_setting == 1)||(poweroff_setting == 2) || (poweroff_setting == 4) || (poweroff_setting == 0x80)))
    {
        appPhySetPowerOffMode(command->payload[0]);
        /* send response */
        tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_SUCCESS);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_get_autopoweroff(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    uint16 payload_len = 1;
    uint8 payload[payload_len];
    if(command->size_payload == 0)
    {
        payload[0] = appPhyGetPowerOffMode();
        /* send response */
        tym_gaia_send_response(command->command_id, GAIA_STATUS_SUCCESS, payload_len, payload);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_set_earbudcustom(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    int seq;
    tym_sync_app_configuration_t *app_set = TymGet_AppSetting();
    if(command->size_payload == 8)
    {
        //check seq
        for(seq = 0;seq < uiseq_maximum;seq++)
        {
            if(command->payload[seq] == uifunc_dont_change)
                continue;
            app_set->custom_ui[seq] = command->payload[seq];
        }
        if(app_set->custom_ui[uiseq_left_tapx2] == uifunc_track)
            app_set->custom_ui[uiseq_left_tapx3] = uifunc_track;
        else
            app_set->custom_ui[uiseq_left_tapx3] = uifunc_disable;

        if(app_set->custom_ui[uiseq_right_tapx2] == uifunc_track)
            app_set->custom_ui[uiseq_right_tapx3] = uifunc_track;
        else
            app_set->custom_ui[uiseq_right_tapx3] = uifunc_disable;
        PsStore(PSID_APPCONFIG, app_set, PS_SIZE_ADJ(sizeof(tym_sync_app_configuration_t)));
        tymSyncAppConfiguration(app_set);

        tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_SUCCESS);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_INVALID_PARAMETER);
    }

}

void bell_gaia_get_earbudcustom(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    int i;
    tym_sync_app_configuration_t *app_set = TymGet_AppSetting();
    uint8 payload[uiseq_maximum];
    if(command->size_payload == 0)
    {
        for(i = 0;i < uiseq_maximum;i++)
            payload[i] = app_set->custom_ui[i];
        /* send response */
        tym_gaia_send_response(command->command_id, GAIA_STATUS_SUCCESS, uiseq_maximum, payload);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_set_smartassistant(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    if(command->size_payload == 1)
    {
        if((command->payload[0] == 0x0) || (command->payload[0] == 0x02))
        {
            tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_NOT_SUPPORTED);
        }
        else if(command->payload[0] == 0x1)
        {
            /* send response */
            tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_SUCCESS);
        }
        else
        {
            tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_INVALID_PARAMETER);
        }
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_INVALID_PARAMETER);
    }

}

void bell_gaia_get_smartassistant(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    uint16 payload_len = 1;
    uint8 payload[payload_len];
    if(command->size_payload == 0)
    {
        payload[0] = 0x1;
        /* send response */
        tym_gaia_send_response(command->command_id, GAIA_STATUS_SUCCESS, payload_len, payload);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_get_batterylevel(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    uint16 payload_len = 2;
    uint16 left_batt,right_batt;
    uint8 payload[payload_len];
    if(command->size_payload == 0)
    {
        if(Multidevice_IsLeft() == TRUE)
        {
            //local : left
            StateProxy_GetLocalAndRemoteBatteryLevels(&left_batt, &right_batt); //local,peer
        }
        else
        {
            //local : right
            StateProxy_GetLocalAndRemoteBatteryLevels(&right_batt, &left_batt); //local,peer
        }
        payload[0] = appBatteryConvertLevelToPercentage(left_batt);
        payload[1] = appBatteryConvertLevelToPercentage(right_batt);
        tym_gaia_send_response(command->command_id, GAIA_STATUS_SUCCESS, payload_len, payload);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_get_twsstatus(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    bool left = Multidevice_IsLeft();
    //bool primary = appSmIsPrimary();
    bool connected = appPeerSigIsConnected();   
    uint16 payload_len = 2;
    uint8 payload[payload_len];
    if(command->size_payload == 0)
    {
        if(connected == FALSE)
        {
            if(left == TRUE)
            {
                payload[0] = 0x01;
                payload[1] = 0xff;
            }
            else
            {
                payload[0] = 0xff;
                payload[1] = 0x01;
            }        
        }
        else
        {
            if(left == TRUE)
            {
                payload[0] = 0x01;
                payload[1] = 0x00;
            }
            else
            {
                payload[0] = 0x00;
                payload[1] = 0x01;
            }        
        }
        tym_gaia_send_response(command->command_id, GAIA_STATUS_SUCCESS, payload_len, payload);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_set_autowear(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    tym_sync_app_configuration_t *app_set = TymGet_AppSetting();    
    if(command->size_payload == 1)
    {
        if((command->payload[0] == 0x0) || (command->payload[0] == 0x01))
        {
            app_set->enable_auto_wear = command->payload[0];
            PsStore(PSID_APPCONFIG, app_set, PS_SIZE_ADJ(sizeof(tym_sync_app_configuration_t)));
            tymSyncAppConfiguration(app_set);
            /* send response */
            tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_SUCCESS);
        }
        else
        {
            tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_INVALID_PARAMETER);
        }
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_get_autowear(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    uint16 payload_len = 1;
    uint8 payload[payload_len];
    tym_sync_app_configuration_t *app_set = TymGet_AppSetting();     
    if(command->size_payload == 0)
    {
        payload[0] = app_set->enable_auto_wear;
        tym_gaia_send_response(command->command_id, GAIA_STATUS_SUCCESS, payload_len, payload);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_get_twslinkstate(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    bool connected = appPeerSigIsConnected();   
    uint16 payload_len = 1;
    uint8 payload[payload_len];
    if(command->size_payload == 0)
    {
        if(connected)
            payload[0] = 1;
        else
            payload[0] = 0;    
        tym_gaia_send_response(command->command_id, GAIA_STATUS_SUCCESS, payload_len, payload);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_get_psval(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    uint16 payload_len = 2;
    uint8 payload[payload_len];
    uint16 read;
    if(command->size_payload == 0)
    {
        read= appProximityDataRead();

        payload[0] = (read >> 8);
        payload[1] = (read & 0xff);
        tym_gaia_send_response(command->command_id, GAIA_STATUS_SUCCESS,payload_len , payload);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_INVALID_PARAMETER);
    }

}

void bell_gaia_set_ir_config(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    if(command->size_payload == 2)
    {
        appProximityDataConfig(command->payload[0],command->payload[1]);
        tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_SUCCESS);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_get_play_pause_status(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    uint16 payload_len = 1;
    uint8 payload[payload_len];
    bool playing = (appAvPlayStatus() == avrcp_play_status_playing);
    if(command->size_payload == 0)
    {
        if(playing == TRUE)
            payload[0] = 1;
        else
            payload[0] = 0;
        tym_gaia_send_response(command->command_id, GAIA_STATUS_SUCCESS, payload_len, payload);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_set_eq_control(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    if(command->size_payload == 1)
    {
        tym_send_switch_eq_preset(command->vendor_id, command->size_payload, command->payload);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_get_eq_contorl(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    uint16 payload_len = 1;
    uint8 payload[payload_len];

    if(command->size_payload == 0)
    {
        payload[0] = get_cur_preset_eq();
        tym_gaia_send_response(command->command_id, GAIA_STATUS_SUCCESS, payload_len, payload);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_no_operation(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_SUCCESS);
}

void bell_gaia_get_fw_version(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    gaia_send_application_version(command->vendor_id, command->command_id);
}

void bell_gaia_get_serial_number(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    #define PSKEY_SN_STRING             0x02C3
    uint16 payload_len;
    uint8 payload[10] = {0};

    if(command->size_payload == 0)
    {
        payload_len = PsFullRetrieve(PSKEY_SN_STRING, NULL, 0);
        if(payload_len)
        {
            PsFullRetrieve(PSKEY_SN_STRING, payload, payload_len);
            DEBUG_LOG("sn %s\n",payload);
            tym_gaia_send_response(command->command_id, GAIA_STATUS_SUCCESS, payload_len*2, payload);
        }
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_set_findme(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    if(command->size_payload == 1)
    {
        DEBUG_LOG("FindMe %x\n",command->payload[0]);
        appKymeraSetPromptVol(command->payload[0]);
        tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_SUCCESS);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_set_audio_contorl(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    if(command->size_payload == 1)
    {
        DEBUG_LOG("Audio contorl %x\n",command->payload[0]);
        switch(command->payload[0])
        {
            case BELL_GAIA_AUDIO_CONTROL_VOL_UP:
                MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_VOLUME_UP, NULL);
                tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_SUCCESS);
                break;
            case BELL_GAIA_AUDIO_CONTROL_VOL_DOWN:
                MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_VOLUME_DOWN, NULL);
                tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_SUCCESS);
                break;
            case BELL_GAIA_AUDIO_CONTROL_PLAY:
                Ui_InjectUiInput(ui_input_play);
                tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_SUCCESS);
                break;
            case BELL_GAIA_AUDIO_CONTROL_PAUSE:
                Ui_InjectUiInput(ui_input_pause);
                tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_SUCCESS);
                break;
            case BELL_GAIA_AUDIO_CONTROL_Stop:
                Ui_InjectUiInput(ui_input_stop_av_connection);
                tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_SUCCESS);
                break;
            case BELL_GAIA_AUDIO_CONTROL_SKIP_FORWARD:
                Ui_InjectUiInput(ui_input_av_forward);
                tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_SUCCESS);
                break;
            case BELL_GAIA_AUDIO_CONTROL_BACK_FORWARD:
                Ui_InjectUiInput(ui_input_av_backward);
                tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_SUCCESS);
                break;
            case BELL_GAIA_AUDIO_CONTROL_MIC_MUTE:
            case BELL_GAIA_AUDIO_CONTROL_MIC_UNMUTE:
            default:
                tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_NOT_SUPPORTED);
            break;
        }
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_set_auto_play(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    tym_sync_app_configuration_t *app_set = TymGet_AppSetting();
    if(command->size_payload == 1)
    {
        if((command->payload[0] == 0x0) || (command->payload[0] == 0x01))
        {
            app_set->enable_auto_play = command->payload[0];
            PsStore(PSID_APPCONFIG, app_set, PS_SIZE_ADJ(sizeof(tym_sync_app_configuration_t)));
            tymSyncAppConfiguration(app_set);
            /* send response */
            tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_SUCCESS);
        }else
            tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_INVALID_PARAMETER);

    }
    else
    {
        tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_get_auto_play(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    tym_sync_app_configuration_t *app_set = TymGet_AppSetting();
    uint16 payload_len = 1;
    uint8 payload[payload_len];

    if(command->size_payload == 0)
    {
        payload[0] = app_set->enable_auto_play;
        tym_gaia_send_response(command->command_id, GAIA_STATUS_SUCCESS, payload_len, payload);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_INVALID_PARAMETER);
    }
}

void bell_gaia_set_ambient_ext_anc(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    tym_sync_app_configuration_t *app_set = TymGet_AppSetting();
    if(command->size_payload == 1)
    {
        if((command->payload[0] == 0x0) || (command->payload[0] == 0x01))
        {
            app_set->ambient_ext_anc = command->payload[0];
            PsStore(PSID_APPCONFIG, app_set, PS_SIZE_ADJ(sizeof(tym_sync_app_configuration_t)));
            tymSyncAppConfiguration(app_set);
            /* send response */
            tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_SUCCESS);
        }else
            tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_INVALID_PARAMETER);

    }
    else
    {
        tym_gaia_send_simple_response(command->command_id, GAIA_STATUS_INVALID_PARAMETER);
    }    
}

void bell_gaia_get_ambient_ext_anc(GAIA_UNHANDLED_COMMAND_IND_T *command)
{
    tym_sync_app_configuration_t *app_set = TymGet_AppSetting();
    uint16 payload_len = 1;
    uint8 payload[payload_len];

    if(command->size_payload == 0)
    {
        payload[0] = app_set->ambient_ext_anc;
        tym_gaia_send_response(command->command_id, GAIA_STATUS_SUCCESS, payload_len, payload);
    }
    else
    {
        tym_gaia_send_simple_response(command->command_id,GAIA_STATUS_INVALID_PARAMETER);
    }   
}
 /* -------------------------------------------------------------------------
 * ------- Public functions  ------------------------------------------------
 * --------------------------------------------------------------------------
 */   
bool _bell_GAIAMessageHandle(Task task, const GAIA_UNHANDLED_COMMAND_IND_T *message)
{
    UNUSED(task);
    GAIA_UNHANDLED_COMMAND_IND_T *command = (GAIA_UNHANDLED_COMMAND_IND_T *)message;
    uint16 command_id = command->command_id;
    bool handled = TRUE;

    switch(command_id)
    {
        case BELL_GAIA_RENAME_COMMAND:
            bell_gaia_rename(command);
            break;
        case BELL_GAIA_SET_ANC_STATE_COMMAND:
            bell_gaia_set_anc_state(command);
            break;
        case BELL_GAIA_GET_ANC_STATE_COMMAND:
            bell_gaia_get_anc_state(command);
            break;
        case BELL_GAIA_SET_AMBIENT_MODE_COMMAND:
            bell_gaia_set_ambient_state(command);
            break;
        case BELL_GAIA_GET_AMBIENT_MODE_COMMAND:
            bell_gaia_get_ambient_state(command);
            break;
        case BELL_GAIA_SET_AMBIENT_LEVEL_COMMAND:
            bell_gaia_set_ambient_level(command);
            break;
        case BELL_GAIA_GET_AMBIENT_LEVEL_COMMAND:
            bell_gaia_get_ambient_level(command);
            break;
        case BELL_GAIA_SET_SPEECH_STATE_COMMAND:
            bell_gaia_set_voice_state(command);
            break;
        case BELL_GAIA_GET_SPEECH_STATE_COMMAND:
            bell_gaia_get_voice_state(command);
            break;
        case BELL_GAIA_RESET_ANC_FEATURE_COMMAND:
            bell_gaia_reset_anc_state(command);
            break;
        case BELL_GAIA_GET_ANC_FEATURE_COMMAND:
            bell_gaia_get_anc_feature(command);
            break;
        case BELL_GAIA_SET_AUTO_POWEROFF_COMMAND:
            bell_gaia_set_autopoweroff(command);
            break;
        case BELL_GAIA_GET_AUTO_POWEROFF_COMMAND:
            bell_gaia_get_autopoweroff(command);
            break;
        case BELL_GAIA_GET_EARBUDS_CUSTOM:
            bell_gaia_get_earbudcustom(command);
            break;
        case BELL_GAIA_SET_EARBUDS_CUSTOM:
            bell_gaia_set_earbudcustom(command);
            break;
        case BELL_GAIA_SET_SMART_ASSISTANT_COMMAND:
            bell_gaia_set_smartassistant(command);
            break;
        case BELL_GAIA_GET_SMART_ASSISTANT_COMMAND:
            bell_gaia_get_smartassistant(command);
            break;
        case BELL_GAIA_SET_SPEECH_LEVEL_COMMAND:
            bell_gaia_set_voice_level(command);
            break;
        case BELL_GAIA_GET_SPEECH_LEVEL_COMMAND:
            bell_gaia_get_voice_level(command);
            break;
        case BELL_GAIA_GET_BATTERY_LEVEL_COMMAND:
            bell_gaia_get_batterylevel(command);
            break;
        case BELL_GAIA_GET_TWS_STATUS_COMMAND:
            bell_gaia_get_twsstatus(command);
            break;
        case BELL_GAIA_SET_AUTOWEAR_COMMAND:
            bell_gaia_set_autowear(command);
            break;
        case BELL_GAIA_GET_AUTOWEAR_COMMAND:
            bell_gaia_get_autowear(command);
            break;
        case BELL_GAIA_GET_PLAYPAUSE_STATUS_COMMAND:
            bell_gaia_get_play_pause_status(command);
            break;    
        case BELL_GAIA_GET_FIRMWARE_VERSION_COMMAND:
            bell_gaia_get_fw_version(command);
            break;
        case BELL_GAIA_GET_SERIAL_NUMBER_COMMAND:
            bell_gaia_get_serial_number(command);
            break;
        case BELL_GAIA_SET_FINDME_COMMAND:
            bell_gaia_set_findme(command);
            break;
        case BELL_GAIA_SER_AUDIO_CONTROL_COMMAND:
            bell_gaia_set_audio_contorl(command);
            break;
        case BELL_GAIA_SET_EQ_CONTROL_COMMAND:
            bell_gaia_set_eq_control(command);
            break;
        case BELL_GAIA_GET_EQ_CONTROL_COMMAND:
            bell_gaia_get_eq_contorl(command);
            break;
        case BELL_GAIA_NO_OPERATION_COMMAND:
            bell_gaia_no_operation(command);
            break;
        case BELL_GAIA_GET_TWSLINK_COMMAND:
            bell_gaia_get_twslinkstate(command);
            break;
        case BELL_GAIA_GET_PSVAL_COMMAND:
            bell_gaia_get_psval(command);
            break;
        case BELL_GAIA_SET_IR_COMMAND:
            bell_gaia_set_ir_config(command);
            break;
        case BELL_GAIA_SET_AUTO_PLAY_COMMAND:
            bell_gaia_set_auto_play(command);
            break;
        case BELL_GAIA_GET_AUTO_PLAY_COMMAND:
            bell_gaia_get_auto_play(command);
            break;   
        case BELL_GAIA_SET_AMBIENT_EXT_ANC_COMMAND:
            bell_gaia_set_ambient_ext_anc(command);            
            break;
        case BELL_GAIA_GET_AMBIENT_EXT_ANC_COMMAND:
            bell_gaia_get_ambient_ext_anc(command);
            break;   
        default:
            tym_gaia_send_simple_response(command_id,GAIA_STATUS_NOT_SUPPORTED);
            handled = FALSE;
            break;
    }

    return handled;
}


/* --------------------------------------------------------------------------
 * ------- Public functions -------------------------------------------------
 * --------------------------------------------------------------------------
 */   
void bell_gaia_anc_notify_event(uint16 notifyid,uint8 val)
{
    uint16 payload_size = 2;
    uint8 payload[payload_size];
    uint16 level = 0;
    tymAncTaskData *tymAnc = TymAncGetTaskData();

    if(notifyid == BELL_GAIA_ANC_NOTIFY)
    {
        tym_gaia_send_notification(notifyid, val, 0, NULL);
    }
    else    
    {
        if(val == 0)
        {    
            level = 0;
        }    
        else    
        {    
            if(notifyid == BELL_GAIA_AMBIENT_NOTIFY)
                level = tymAnc->ambientLevel;
            if(notifyid == BELL_GAIA_SPEECH_NOTIFY)
                level = tymAnc->speechLevel;
        }     
        payload[0] = level >> 8;
        payload[1] = (level & 0xff);
        tym_gaia_send_notification(notifyid, val, payload_size, payload);
    }    
}

void bell_gaia_battery_notify_event(void)
{
    bool left = Multidevice_IsLeft();
    uint16 payload_size = 1;
    uint8 payload[payload_size];
    uint8 val;
    uint16 left_batt,right_batt;
    if(left == TRUE) //master left
    {
        //local : left
        StateProxy_GetLocalAndRemoteBatteryLevels(&left_batt, &right_batt); //local,peer
    }
    else //master right
    {
        //local : right
        StateProxy_GetLocalAndRemoteBatteryLevels(&right_batt, &left_batt); //local,peer
    }
    val = appBatteryConvertLevelToPercentage(left_batt);
    payload[0] = appBatteryConvertLevelToPercentage(right_batt);
    tym_gaia_send_notification(BELL_GAIA_BATTERY_NOTIFY, val, payload_size, payload);
}

void bell_gaia_tws_link_notify_event(void)
{
    bool left = Multidevice_IsLeft();
    bool connected = appPeerSigIsConnected();  
    uint8 val;
    uint16 payload_size = 1;
    uint8 payload[payload_size];
    //tws none,slave
    if(left == TRUE)
    {    
        val = 0;//master left , 0
    }    
    else
    {    
        val = 1;//master right , 1
    }
    
    payload[0] = connected;

    tym_gaia_send_notification(BELL_GAIA_TWS_LINK_STATUS_NOTIFY, val, payload_size, payload);
}

void bell_gaia_play_pause_notify_event(uint8 val)
{
    tym_gaia_send_notification(BELL_GAIA_PLAY_PAUSE_NOTIFY, val, 0, NULL);    
}

void bell_gaia_tws_touch_notify_event(uint8 event)
{
    tym_gaia_send_notification(BELL_GAIA_TWS_TOUCH_NOTIFY, event, 0, NULL);
}

void bell_gaia_set_report_battery_notify(void)
{  
    bell_gaia_battery_notify_event();
    MessageSendLater(GaiaGetTask(), GAIA_BATT_REPORT_IND, NULL, D_MIN(5)); //5 min report battery   
}

void bell_gaia_connect_event(void)
{
    /*report tws status */
    bell_gaia_tws_link_notify_event();
    /*report battery*/
    bell_gaia_set_report_battery_notify();
}

void bell_gaia_disconnect_event(void)
{
    MessageCancelAll(GaiaGetTask(), GAIA_BATT_REPORT_IND); //cancel all battery report
}
/* Not implement :
 * 1. custom ui
 * 2. gaia init report battery and tws status
 * 3. 5 min report battery
 * 4. disconnect cancel battery report
 */

