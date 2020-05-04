/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation for GAA Battery API
*/

#include <logging.h>
#include "gsound_target_battery.h"
#include "battery_monitor.h"
#include "charger_monitor.h"

typedef struct
{
    TaskData task;
} gaa_battery_task;

static gaa_battery_task gaa_battery_data;
static GSoundStatus (*gaa_battery_ready)(const GSoundBatteryInfo *battery_info) = NULL;

static void gaa_TargetBatteryUpdate(uint8 battery_level)
{
    if (gaa_battery_ready)
    {
        GSoundBatteryInfo battery_info;
        battery_info.percent_full =(int)battery_level;
        battery_info.charging = appChargerIsConnected();
        DEBUG_LOG("gaa_TargetBatteryUpdate %d %i", battery_info.charging, battery_info.percent_full);
        /* Tell the Gaa library the new battery state. */
        gaa_battery_ready(&battery_info);
    }
    else
        DEBUG_LOG("gaa_TargetBatteryUpdate not initialised");
}

static void gaa_BatteryMessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    switch(id)
    {
        case MESSAGE_BATTERY_LEVEL_UPDATE_PERCENT:
        {
            const MESSAGE_BATTERY_LEVEL_UPDATE_PERCENT_T *battery_level = message;
            gaa_TargetBatteryUpdate(battery_level->percent);
            break;
        }
        default:
            break;
    }
}

static void gaa_RegisterBatteryClient(void)
{
    batteryRegistrationForm battery_registration_form;
    battery_registration_form.task =&gaa_battery_data.task;
    battery_registration_form.representation = battery_level_repres_percent;
    battery_registration_form.hysteresis = 1;
    appBatteryRegister(&battery_registration_form);
}

GSoundStatus GSoundTargetBatteryInit(GSoundBatteryInterface *interface)
{
    DEBUG_LOG("GSoundTargetBatteryInit %p", interface->gsound_battery_ready);
    gaa_battery_ready = interface->gsound_battery_ready;
    gaa_battery_data.task.handler= gaa_BatteryMessageHandler;
    gaa_RegisterBatteryClient();
    return GSOUND_STATUS_OK;
}
