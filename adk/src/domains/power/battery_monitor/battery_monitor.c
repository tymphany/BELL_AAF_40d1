/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       battery_monitor.c
\brief      Battery monitoring
*/

#include <adc.h>

#include "app_task.h"
#include "battery_monitor.h"
#include "battery_monitor_config.h"
#include "init.h"
#include "hydra_macros.h"
#include "panic.h"
#include "unexpected_message.h"

#include <logging.h>
#include <stdlib.h>
#ifdef ENABLE_TYM_PLATFORM
#include "ui.h"
#include "ps.h"
#include "earbud_tym_psid.h"
#include "charger_monitor.h"
#include "state_proxy.h"
#endif
#ifndef FAKE_BATTERY_LEVEL
#define FAKE_BATTERY_LEVEL (0)
#endif
uint16 fake_battery_level = FAKE_BATTERY_LEVEL;

#ifdef QCC3020_FF_ENTRY_LEVEL_AA
#define CHARGED_BATTERY_FULL_OFFSET_mV   (200)
#else
#define CHARGED_BATTERY_FULL_OFFSET_mV   (0)
#endif

/*! Whilst the filter is being filled read at this rate */
#define BATTERY_READ_PERIOD_INITIAL (0)

/*! \brief Battery component task data. */
batteryTaskData app_battery;

#define MESSAGE_BATTERY_PROCESS_READING     0x40

/*! Enumerated type for messages sent within the headset battery
    handler only. */
enum headset_battery_internal_messages
{
        /*! Message sent to trigger an intermittent battery measurement */
    MESSAGE_BATTERY_INTERNAL_MEASUREMENT_TRIGGER = 1,
#ifdef ENABLE_TYM_PLATFORM
        /*! Message sent to trigger an intermittent battery predict */
    MESSAGE_BATTERY_INTERNAL_PREDICT_TRIGGER = 2,
#endif
    MESSAGE_BATTERY_TEST_PROCESS_READING = MESSAGE_BATTERY_PROCESS_READING,
};

#ifdef ENABLE_TYM_PLATFORM
#define BATT_TABLE_NUM       20
uint16 disch_table[BATT_TABLE_NUM] = {3520, 3538, 3560, 3584, 3601, 3615, 3629, 3644, 3659, 3676, 3696, 3718, 3744, 3778, 3810, 3846, 3894, 3930, 3966, 3992};
#endif
/*! TRUE when the filter is filled with results */
#define FILTER_IS_FULL(battery) ((battery)->filter.index >= BATTERY_FILTER_LEN)

/*! Add a client to the list of clients */
static bool appBatteryClientAdd(batteryTaskData *battery, batteryRegistrationForm *form)
{
    batteryRegisteredClient *new = calloc(1, sizeof(*new));
    if (new)
    {
        new->form = *form;
        new->next = battery->client_list;
        battery->client_list = new;
        return TRUE;
    }
    return FALSE;
}

/*! Remove a client from the list of clients */
static void appBatteryClientRemove(batteryTaskData *battery, Task task)
{
    batteryRegisteredClient **head;
    for (head = &battery->client_list; *head != NULL; head = &(*head)->next)
    {
        if ((*head)->form.task == task)
        {
            batteryRegisteredClient *toremove = *head;
            *head = (*head)->next;
            free(toremove);
            break;
        }
    }
}

static uint8 toPercentage(uint16 voltage)
{
    uint16 critical = appConfigBatteryVoltageCritical();
    /* When unplug charger, voltage will drop so battery never reported full*/
    /* Define the offset value to avoid impression that battery never full */
    uint16 charged = appConfigBatteryFullyCharged() - CHARGED_BATTERY_FULL_OFFSET_mV;
#ifdef ENABLE_TYM_PLATFORM
    uint8 percent = 0;
    int i;
#endif
    if (voltage < critical)
        voltage = critical;
    else if (voltage > charged)
        voltage = charged;
#ifdef ENABLE_TYM_PLATFORM
    for(i = (BATT_TABLE_NUM-1);i >= 0;i--)
    {
        if(voltage >= disch_table[i])
        {
            percent = (i+1)*5;
            break;
        }
    }
    return percent;
#else
    return (100UL * (uint32)(voltage - critical)) / (uint32)(charged - critical);
#endif
}

static battery_level_state toState(uint16 voltage)
{
    if (FILTER_IS_FULL(GetBattery()))
    {
        if (voltage < appConfigBatteryVoltageCritical())
            return battery_level_too_low;
        if (voltage < appConfigBatteryVoltageLow())
            return battery_level_critical;
        if (voltage < appConfigBatteryVoltageOk())
            return battery_level_low;

        return battery_level_ok;
    }

    return battery_level_unknown;
}

/* TRUE if the current value is less than the threshold taking into account hysteresis */
static bool ltThreshold(uint16 current, uint16 threshold, uint16 hysteresis)
{
    return current < (threshold - hysteresis);
}

/* TRUE if the current value is greater than the threshold taking into account hysteresis. */
static bool gtThreshold(uint16 current, uint16 threshold, uint16 hysteresis)
{
    return current > (threshold + hysteresis);
}

/* TRUE if the current value is outside the threshold taking into account hysteresis */
static bool thresholdExceeded(uint16 current, uint16 threshold, uint16 hysteresis)
{
    return ltThreshold(current, threshold, hysteresis) ||
           gtThreshold(current, threshold, hysteresis);
}

/* Determine if the transition should be made between last_state and
   new_state. For the transition to happen, the current battery voltage must
   exceed the state defined voltage level by the amount of hysteresis.

   If the state has jumped two levels, then we assume the level has jumped and
   this is acceptable.
 */
static bool stateUpdateIsRequired(battery_level_state last_state,
                                  battery_level_state new_state,
                                  uint16 voltage,
                                  uint16 hysteresis)
{
    if (   new_state == battery_level_unknown
        || new_state == last_state)
    {
        return FALSE;
    }

    if (last_state == battery_level_unknown)
    {
        return TRUE;
    }

    switch (last_state)
    {
        case battery_level_too_low:
            switch (new_state)
            {
                case battery_level_critical:
                    return gtThreshold(voltage, appConfigBatteryVoltageCritical(), hysteresis);
                case battery_level_low:
                case battery_level_ok:
                    return TRUE;
                default:
                    break;
            }
            break;

        case battery_level_critical:
            switch (new_state)
            {
                case battery_level_too_low:
                    return ltThreshold(voltage, appConfigBatteryVoltageCritical(), hysteresis);
                case battery_level_low:
                    return gtThreshold(voltage, appConfigBatteryVoltageLow(), hysteresis);
                case battery_level_ok:
                    return TRUE;
                default:
                    break;
            }
            break;

        case battery_level_low:
            switch (new_state)
            {
                case battery_level_too_low:
                    return TRUE;
                case battery_level_critical:
                    return ltThreshold(voltage, appConfigBatteryVoltageLow(), hysteresis);
                case battery_level_ok:
                    return gtThreshold(voltage, appConfigBatteryVoltageOk(), hysteresis);
                default:
                    break;
            }
            break;

        case battery_level_ok:
            switch (new_state)
            {
                case battery_level_too_low:
                case battery_level_critical:
                    return TRUE;
                case battery_level_low:
                    return ltThreshold(voltage, appConfigBatteryVoltageOk(), hysteresis);
                default:
                    break;
            }
            break;

        default:
            break;
    }
    return FALSE;
}

/*! Iterate through the list of clients, sending battery level messages when
    the representation criteria is met */
static void appBatteryServiceClients(batteryTaskData *battery)
{
    batteryRegisteredClient *client = NULL;
    uint16 voltage = appBatteryGetVoltage();
    for (client = battery->client_list; client != NULL; client = client->next)
    {
        uint16 hysteresis = client->form.hysteresis;
        switch (client->form.representation)
        {
            case battery_level_repres_voltage:
                if (thresholdExceeded(voltage, client->last.voltage, hysteresis))
                {
                    MESSAGE_MAKE(msg, MESSAGE_BATTERY_LEVEL_UPDATE_VOLTAGE_T);
                    msg->voltage_mv = voltage;
                    client->last.voltage = voltage;
                    MessageSend(client->form.task, MESSAGE_BATTERY_LEVEL_UPDATE_VOLTAGE, msg);
                }
            break;
            case battery_level_repres_percent:
            {
                uint8 percent = toPercentage(voltage);
                if (thresholdExceeded(percent, client->last.percent, hysteresis))
                {
                    MESSAGE_MAKE(msg, MESSAGE_BATTERY_LEVEL_UPDATE_PERCENT_T);
                    msg->percent = percent;
                    client->last.percent = percent;
                    MessageSend(client->form.task, MESSAGE_BATTERY_LEVEL_UPDATE_PERCENT, msg);
                }
            }
            break;
            case battery_level_repres_state:
            {
                battery_level_state new_state = toState(voltage);
                if (stateUpdateIsRequired(client->last.state, new_state, voltage, hysteresis))
                {
#ifdef ENABLE_TYM_PLATFORM /* === play battery low prompt === */
                    if(client->last.state > battery_level_critical)
                    {
                        if(new_state == battery_level_critical)
                        {
                            if(StateProxy_IsInCase() == FALSE)
                                Ui_InjectUiInput(ui_input_prompt_battery_low);
                        }
                    }
#endif
                    MESSAGE_MAKE(msg, MESSAGE_BATTERY_LEVEL_UPDATE_STATE_T);
                    msg->state = new_state;
                    client->last.state = new_state;
                    MessageSend(client->form.task, MESSAGE_BATTERY_LEVEL_UPDATE_STATE, msg);
                }
            }
            break;
        }
    }
}

static void appBatteryScheduleNextMeasurement(batteryTaskData *battery)
{
    uint32 delay = FILTER_IS_FULL(battery) ?
                        battery->period :
                        BATTERY_READ_PERIOD_INITIAL;
    MessageSendLater(&battery->task, MESSAGE_BATTERY_INTERNAL_MEASUREMENT_TRIGGER,
                        NULL, delay);
}

/*! Return TRUE if a new voltage is available,with enough samples that the result
    should be stable. This waits for the filter to be full. */
static bool appBatteryAdcResultHandler(batteryTaskData *battery, MessageAdcResult* result)
{
    uint16 reading = result->reading;

    switch (result->adc_source)
    {
        case adcsel_pmu_vbat_sns:
        {
            uint32 index = battery->filter.index & BATTERY_FILTER_MASK;
            uint16 vbatt_mv = (uint16)((uint32)VmReadVrefConstant() * reading / battery->vref_raw);

            battery->filter.accumulator -= battery->filter.buf[index];
            battery->filter.buf[index] = vbatt_mv;
            battery->filter.accumulator += vbatt_mv;
            battery->filter.index++;
            /* See the logic in appBatteryGetVoltage():
               0<=index<BATTERY_FILTER_LEN is only used when filling the filter.
               When incrementing to BATTERY_FILTER_LEN, initialisation is complete.
               When wrapping to 0, jump over the initialisation index range. */
            if (battery->filter.index == BATTERY_FILTER_LEN)
            {
#ifdef ENABLE_TYM_PLATFORM
                MessageSend(&battery->task, MESSAGE_BATTERY_INTERNAL_PREDICT_TRIGGER, NULL);
#endif
                MessageSend(Init_GetInitTask(), MESSAGE_BATTERY_INIT_CFM, NULL);
            }
            if (battery->filter.index == 0)
            {
                battery->filter.index = BATTERY_FILTER_LEN;
            }
            appBatteryScheduleNextMeasurement(battery);
            return FILTER_IS_FULL(battery);
        }

        case adcsel_vref_hq_buff:
            battery->vref_raw = reading;
            break;

        default:
            DEBUG_LOG("appBatteryAdcResultHandler unexpected source - %d",result->adc_source);
            break;
    }
    return FALSE;
}

static void appBatteryHandleMessage(Task task, MessageId id, Message message)
{
    batteryTaskData *battery = (batteryTaskData *)task;
    if (battery->period != 0)
    {
        switch (id)
        {
            case MESSAGE_ADC_RESULT:
                if (appBatteryAdcResultHandler(battery, (MessageAdcResult*)message))
                {
                    appBatteryServiceClients(battery);
                }
                break;

            case MESSAGE_BATTERY_TEST_PROCESS_READING:
                appBatteryServiceClients(battery);
                break;

            case MESSAGE_BATTERY_INTERNAL_MEASUREMENT_TRIGGER:
                /* Start immediate battery reading, note vref is read first */
                AdcReadRequest(&battery->task, adcsel_vref_hq_buff, 0, 0);
                AdcReadRequest(&battery->task, adcsel_pmu_vbat_sns, 0, 0);
                break;
#ifdef ENABLE_TYM_PLATFORM
            case MESSAGE_BATTERY_INTERNAL_PREDICT_TRIGGER:
                appBatteryGetPredictVoltage();
                MessageSendLater(&battery->task, MESSAGE_BATTERY_INTERNAL_PREDICT_TRIGGER, NULL, D_SEC(30));
                break;
#endif
            default:
                /* An unexpected message has arrived - must handle it */
                UnexpectedMessage_HandleMessage(id);
                break;
        }
    }
}

bool appBatteryInit(Task init_task)
{
    DEBUG_LOG("appBatteryMonInit");
    batteryTaskData *battery = GetBattery();
    memset(battery, 0, sizeof(*battery));

    /* Set up task handler */
    battery->task.handler = appBatteryHandleMessage;
    battery->period = appConfigBatteryReadPeriodMs();

    appBatteryScheduleNextMeasurement(battery);

    Init_SetInitTask(init_task);
    return TRUE;
}

void appBatterySetPeriod(uint16 period)
{
    batteryTaskData *battery = GetBattery();
    DEBUG_LOG("appBatteryMonPeriod %d", period);
    if (period == 0)
    {
        /* Reset the filter data */
        memset(&battery->filter, 0, sizeof(battery->filter));
    }
    else if (battery->period == 0)
    {
        /* Restart */
        appBatteryScheduleNextMeasurement(battery);
    }
    battery->period = period;
}
#ifdef ENABLE_TYM_PLATFORM
/*brief update tymphany predict voltage to PSKEY*/
void updatePredictVoltToPSKEY(void)
{
    uint16 batt_pskey[2];
    batteryTaskData *battery = GetBattery();
    if(battery->predict_volt != 0)
    {
        batt_pskey[0] = battery->predict_volt;
        batt_pskey[1] = battery->lock;
        PsStore(PSID_BATT, batt_pskey, 2);
    }
}

/*brief get tymphany predict voltage */
uint16 appBatteryGetVoltage(void)
{
    batteryTaskData *battery = GetBattery();
    /*get battery predict */
    return battery->predict_volt;
}

/*brief predict voltage according Qualcomm current voltage */
void appBatteryGetPredictVoltage(void)
{
    batteryTaskData *battery = GetBattery();
    uint16 voltage = appBatteryGetQualcommVoltage();
    uint16 batt_pskey[2];
    bool charging = appChargerIsCharging();
    bool skip = 0;

    if(battery->predict_volt == 0)
    {
        if(PsRetrieve(PSID_BATT, 0, 0) != 0)
        {
            PsRetrieve(PSID_BATT, batt_pskey, 2);
            voltage = batt_pskey[0];
            battery->lock = batt_pskey[1] & 0x01;
            DEBUG_LOG("get batt pksey lock %d",battery->lock);
        }
        battery->predict_volt = voltage;
        skip = 1;
    }

    if((battery->lock == 0) && (charging == 0))
    {
        if((voltage > 5) && (skip == 0))
        {
            DEBUG_LOG("wait Qual %d,predict %d.balance",voltage,battery->predict_volt);
            if((battery->predict_volt <= (voltage + 5)) && (battery->predict_volt >= (voltage - 5)))
            {
                battery->lock = 1;
            }
            battery->predict_volt = voltage;
        }
    }

    if(charging == TRUE)
    {
        if(battery->predict_volt < 4200)
        {
            /*voltage 40%*/
            if(battery->predict_volt >= disch_table[7])
                battery->predict_volt += 2;
            else
                battery->predict_volt += 3;
        }
    }
    else
    {
        if(battery->predict_volt > voltage)
        {
            /*voltage 80%-30%*/
            if((battery->predict_volt >= disch_table[5]) && (battery->predict_volt <= disch_table[15]))
                battery->predict_volt -= 1;
            else
                battery->predict_volt -= 2;
        }
    }
    DEBUG_LOG("Qual %d,predict %d,charging %d",voltage,battery->predict_volt,charging);
}

/*brief Qualcomm reference voltage*/
uint16 appBatteryGetQualcommVoltage(void)
{
    if (fake_battery_level)
    {
        return fake_battery_level;
    }
    else
    {
        batteryTaskData *battery = GetBattery();
        uint16 index = battery->filter.index;
        uint32 accumulator = battery->filter.accumulator;

        if (FILTER_IS_FULL(battery))
        {
            return accumulator / BATTERY_FILTER_LEN;
        }
        return index == 0 ? 0 : accumulator / index;
    }
}

void appBatteryLowTest(void)
{
    stateUpdateIsRequired(battery_level_ok,battery_level_critical,4150,50);
}
#else

uint16 appBatteryGetVoltage(void)
{
    if (fake_battery_level)
    {
        return fake_battery_level;
    }
    else
    {
        batteryTaskData *battery = GetBattery();
        uint16 index = battery->filter.index;
        uint32 accumulator = battery->filter.accumulator;

        if (FILTER_IS_FULL(battery))
        {
            return accumulator / BATTERY_FILTER_LEN;
        }
        return index == 0 ? 0 : accumulator / index;
    }
}
#endif


battery_level_state appBatteryGetState(void)
{
    uint16 voltage = appBatteryGetVoltage();

    return toState(voltage);
}


uint8 appBatteryConvertLevelToPercentage(uint16 level_mv)
{
    return toPercentage(level_mv);
}


uint8 appBatteryGetPercent(void)
{
    uint16 voltage_mv = appBatteryGetVoltage();

    return toPercentage(voltage_mv);
}

bool appBatteryRegister(batteryRegistrationForm *client)
{
    batteryTaskData *battery = GetBattery();
    if (appBatteryClientAdd(battery, client))
    {
        appBatteryServiceClients(battery);
        return TRUE;
    }
    return FALSE;
}

void appBatteryTestSetFakeLevel(uint16 voltage)
{
    fake_battery_level = voltage;
    MessageSend(&GetBattery()->task, MESSAGE_BATTERY_PROCESS_READING, NULL);
}

extern void appBatteryUnregister(Task task)
{
    batteryTaskData *battery = GetBattery();
    appBatteryClientRemove(battery, task);
}
