#ifndef __EARBUD_TYM_GAIA_H__
#define __EARBUD_TYM_GAIA_H__
#include "gaia_framework.h"

#define  BELL_VENDOR_QTIL                  (0x0664)
/*follow Bell Customer GAIA app specification ver 20 page 11. custom ui define */
typedef enum{
    uifunc_anc_amb = 0,
    uifunc_play_pause,
    uifunc_play_pause_with_amb,
    uifunc_vol,
    uifunc_track,
    uifunc_google_notification,
    uifunc_stop_google_assistant,
    uifunc_battery_level,
    uifunc_mute,
    uifunc_gaming,
    uifunc_movie,
    uifunc_dont_change = 0x80,
    uifunc_disable = 0xff,           
}custom_ui_func;

/*follow Bell Customer GAIA app specification ver page 9. custom ui define */
typedef enum{
   uiseq_left_swipe,
   uiseq_right_swipe,
   uiseq_left_tapx1,
   uiseq_right_tapx1,
   uiseq_left_tapx2,   
   uiseq_right_tapx2,
   uiseq_left_tapx3,
   uiseq_right_tapx3,
   uiseq_maximum,                       
}custom_ui_seq;

/*follow Bell Customer GAIA app specification ver page 9. custom ui define */
typedef enum{
   uifunc_poweroff_disable,
   uifunc_poweroff_10m,
   uifunc_poweroff_30m = 2,
   uifunc_poweroff_60m = 4,   
   uifunc_poweroff_now = 0x80,      
   uifunc_poweroff_maximum,                       
}custom_power_off_command;

typedef enum {
    BELL_GAIA_RENAME_COMMAND = 0x0001,
    BELL_GAIA_SET_ANC_STATE_COMMAND,
    BELL_GAIA_GET_ANC_STATE_COMMAND,
    BELL_GAIA_SET_AMBIENT_MODE_COMMAND,
    BELL_GAIA_GET_AMBIENT_MODE_COMMAND,//0x5
    BELL_GAIA_SET_AMBIENT_LEVEL_COMMAND,
    BELL_GAIA_GET_AMBIENT_LEVEL_COMMAND,
    BELL_GAIA_SET_SPEECH_STATE_COMMAND,
    BELL_GAIA_GET_SPEECH_STATE_COMMAND = 0x0009,
    BELL_GAIA_RESET_ANC_FEATURE_COMMAND = 0x0010,
    BELL_GAIA_GET_EARBUDS_CUSTOM = 0x0011,
    BELL_GAIA_SET_EARBUDS_CUSTOM = 0x0012,
    BELL_GAIA_GET_ANC_FEATURE_COMMAND = 0x0015,
    BELL_GAIA_SET_AUTO_POWEROFF_COMMAND = 0x001a,
    BELL_GAIA_GET_AUTO_POWEROFF_COMMAND = 0x001b,
    BELL_GAIA_SET_SMART_ASSISTANT_COMMAND = 0x001d,
    BELL_GAIA_GET_SMART_ASSISTANT_COMMAND = 0x001e,  
    BELL_GAIA_SET_SPEECH_LEVEL_COMMAND = 0x001f,
    BELL_GAIA_GET_SPEECH_LEVEL_COMMAND = 0x0020,
    BELL_GAIA_SET_AUTOWEAR_COMMAND = 0x0021,
    BELL_GAIA_GET_AUTOWEAR_COMMAND = 0x0022,
    BELL_GAIA_GET_TWS_STATUS_COMMAND = 0x0023,
    BELL_GAIA_GET_BATTERY_LEVEL_COMMAND = 0x0024,
    BELL_GAIA_GET_PLAYPAUSE_STATUS_COMMAND = 0x0027,
    BELL_GAIA_SET_EQ_CONTROL_COMMAND = 0x0214,
    BELL_GAIA_GET_EQ_CONTROL_COMMAND = 0x0294,
    BELL_GAIA_NO_OPERATION_COMMAND = 0x0700,
    
    BELL_GAIA_GET_TWSLINK_COMMAND = 0x0802,
    BELL_GAIA_GET_PSVAL_COMMAND = 0x0803,
    BELL_GAIA_SET_IR_COMMAND = 0x0804,
    BELL_GAIA_SET_AMBIENT_EXT_ANC_COMMAND = 0x0805, //Todo : Temp command id
    BELL_GAIA_GET_AMBIENT_EXT_ANC_COMMAND = 0x0806, //Todo : Temp command id
    BELL_GAIA_SET_AUTO_PLAY_COMMAND = 0x0487, //Todo : Temp command id
    BELL_GAIA_GET_AUTO_PLAY_COMMAND = 0x0426, //Todo : Temp command id
}BELL_GAIA_COMMAND_E;


typedef enum {
    BELL_GAIA_ANC_NOTIFY  = 0x0040,
    BELL_GAIA_SPEECH_NOTIFY,
    BELL_GAIA_AMBIENT_NOTIFY,
    BELL_GAIA_HEAR_THROUGH,
    BELL_GAIA_BATTERY_NOTIFY = 0x0044,
    BELL_GAIA_TWS_LINK_STATUS_NOTIFY,
    BELL_GAIA_PLAY_PAUSE_NOTIFY,
    BELL_GAIA_TWS_TOUCH_NOTIFY = 0x0080,
}BELL_GAIA_NOTIFY_E;

/*
 TaskData *tym_getBEellGAIATask(void);
*/
bool _bell_GAIAMessageHandle(Task task, const GAIA_UNHANDLED_COMMAND_IND_T *message);
void bell_gaia_anc_notify_event(uint16 event,uint8 val);
void bell_gaia_battery_notify_event(void);
void bell_gaia_tws_link_notify_event(void);
void bell_gaia_tws_touch_notify_event(uint8 event);
void bell_gaia_play_pause_notify_event(uint8 val);
void bell_gaia_set_report_battery_notify(void);
void bell_gaia_disconnect_event(void);
void bell_gaia_connect_event(void);
#endif
