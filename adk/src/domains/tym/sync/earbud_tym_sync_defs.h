#ifndef __EARBUD_TYM_SYNC_DEFS_H__
#define __EARBUD_TYM_SYNC_DEFS_H__

typedef struct tym_sync_data
{
    uint8 cmd;
    uint8 data;
} tym_sync_data_t;

typedef struct tym_sync_app_configuration
{
    uint8 auto_power_off_cmd;
    uint8 auto_power_off_timer;
    uint8 enable_auto_wear;
    uint8 resversed;
    uint8 custom_ui[8];
}tym_sync_app_configuration_t;

#endif
