/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       phy_state.c
\brief      Manage physical state of an Earbud.
*/

#include "phy_state.h"
#include "phy_state_config.h"

#include "adk_log.h"
#include "proximity.h"
#include "acceleration.h"

#include <charger_monitor.h>

#include <panic.h>
#ifdef ENABLE_TYM_PLATFORM
#include <logical_input_switch.h>
#include <input_event_manager.h>
#include <ps.h>
#include "ui.h"
#include "tym_anc_config.h"
#include "tym_touch_config.h"
#include "earbud_tym_util.h"
#include "tym_power_control.h"
#include "earbud_tym_factory.h"
#include "earbud_sm.h"
#include "tym_anc.h"
#include "earbud_tym_cc_communication.h"
#include "hfp_profile.h"
#include "scofwd_profile.h"
#include "1_button.h"
#include "ui_prompts.h"
#include "earbud_tym_sync.h"
#include "anc_state_manager.h"
#include "state_proxy.h"
#include "pio_monitor.h"
#include "audio_curation.h"
#include "tws_topology_private.h"
#include "tws_topology_rule_events.h"
#include "earbud_tym_sensor.h"
#include "handset_service.h"
#include "earbud_tym_psid.h"
#include "earbud_tym_gaia.h"
#include "handset_service_protected.h"
#include "earbud_test.h"
#include "multidevice.h"
#endif
/*! Message creation macro for phyiscal state module. */
#define MAKE_PHYSTATE_MESSAGE(TYPE) TYPE##_T *message = PanicUnlessNew(TYPE##_T);

//!@{ @name Bits in lock used to wait for all initial measurements to complete.
#define PHY_STATE_LOCK_EAR 1
#define PHY_STATE_LOCK_MOTION 2
#define PHY_STATE_LOCK_CASE 4
//!@}

/*!< Physical state of the Earbud. */
phyStateTaskData    app_phy_state;

/*! Clear bit in the lock */
#define appPhyStateClearLockBit(bit) \
do {                                 \
    PhyStateGetTaskData()->lock &= ~bit;  \
} while(0)

#ifdef ENABLE_TYM_PLATFORM
uint8 appPhyStateGetCustomUiId(uint8 act);
void appPhyStateTapx1(void);
void appPhyStateCustomIdTapx1(uint8 act);
void appPhyStateTapx2(void);
void appPhyStateCustomIdTapx2(uint8 act);
void appPhyStateTapx3(void);
void appPhyStateSwipeL(uint8 act);
void appPhyStateSwipeR(uint8 act);
/*! \brief Handle connected prompt check in ear. */
void appPhyStateInEarPromptCheck(void);
#endif
void appPhyStateSetState(phyStateTaskData* phy_state, phyState new_state);

/*! \brief Send a PHY_STATE_CHANGED_IND message to all registered client tasks.
 */
static void appPhyStateMsgSendStateChangedInd(phyState state, phy_state_event event)
{
    MAKE_PHYSTATE_MESSAGE(PHY_STATE_CHANGED_IND);
    message->new_state = state;
    message->event = event;

    /* start notification limit timer if configured */
    if (appPhyStateNotificationsLimitMs())
    {
        MessageSendLater(PhyStateGetTask(), PHY_STATE_INTERNAL_TIMEOUT_NOTIFICATION_LIMIT,
                         NULL, appPhyStateNotificationsLimitMs());
    }

    TaskList_MessageSend(TaskList_GetFlexibleBaseTaskList(PhyStateGetClientTasks()), PHY_STATE_CHANGED_IND, message);
}

/*! \brief Perform actions on entering PHY_STATE_IN_CASE state. */
static void appPhyStateEnterInCase(void)
{
    DEBUG_LOG("appPhyStateEnterInCase");
#ifdef ENABLE_TYM_PLATFORM
    //two earbud in case,report bt pairing to charging case
    if(StateProxy_IsPeerInCase() == TRUE)
    {
        if(tymGetBTStatus() == btPairing)
        {
            tymSyncdata(btStatusCmd,btPairing);
        }
    }
    if(StateProxy_IsStandbyMode())
    {
        DEBUG_LOG("report standby mode");
        reportSleepStandbyStatus(FALSE);
    }    
    /*In case mute stanc3*/
    stanc3_audiomute(TRUE); 
#endif
    appPhyStateMsgSendStateChangedInd(PHY_STATE_IN_CASE, phy_state_event_in_case);
}

/*! \brief Perform actions on entering PHY_STATE_OUT_OF_EAR state. */
static void appPhyStateEnterOutOfEar(void)
{
    DEBUG_LOG("appPhyStateEnterOutOfEar");
}

/*! \brief Perform actions on entering PHY_STATE_IN_EAR state. */
static void appPhyStateEnterInEar(void)
{
    DEBUG_LOG("appPhyStateEnterInEar");
    appPhyStateMsgSendStateChangedInd(PHY_STATE_IN_EAR, phy_state_event_in_ear);
}

/*! \brief Perform actions on entering PHY_STATE_OUT_OF_EAR_AT_REST state. */
static void appPhyStateEnterOutOfEarAtRest(void)
{
    DEBUG_LOG("appPhyStateEnterOutOfEarAtRest");
    appPhyStateMsgSendStateChangedInd(PHY_STATE_OUT_OF_EAR_AT_REST, phy_state_event_not_in_motion);
}

/*! \brief Perform actions on exiting PHY_STATE_UNKNOWN state. */
static void appPhyStateExitUnknown(void)
{
}

/*! \brief Perform actions on exiting PHY_STATE_IN_CASE state. */
static void appPhyStateExitInCase(void)
{
    DEBUG_LOG("appPhyStateExitInCase");
    appPhyStateMsgSendStateChangedInd(PHY_STATE_OUT_OF_EAR, phy_state_event_out_of_case);
#ifdef ENABLE_TYM_PLATFORM /*for exit in case unmute stanc3*/
    stanc3_audiomute(FALSE); 
#endif    
}

/*! \brief Perform actions on exiting PHY_STATE_OUT_OF_EAR state. */
static void appPhyStateExitOutOfEar(void)
{
    DEBUG_LOG("appPhyStateExitOutOfEar");
}

/*! \brief Perform actions on exiting PHY_STATE_IN_EAR state. */
static void appPhyStateExitInEar(void)
{
    DEBUG_LOG("appPhyStateExitInEar");
    appPhyStateMsgSendStateChangedInd(PHY_STATE_OUT_OF_EAR, phy_state_event_out_of_ear);
}

/*! \brief Perform actions on exiting PHY_STATE_OUT_OF_EAR_AT_REST state. */
static void appPhyStateExitOutOfEarAtRest(void)
{
    appPhyStateMsgSendStateChangedInd(PHY_STATE_OUT_OF_EAR, phy_state_event_in_motion);
}

static void appPhyStateHandleBadState(phyState phy_state)
{
    UNUSED(phy_state);
    DEBUG_LOG("appPhyStateHandleBadState %d", phy_state);
    Panic();
}

/*! \brief Handle notification that Earbud is now in the case. */
static void appPhyStateHandleInternalInCaseEvent(void)
{
    phyStateTaskData *phy_state = PhyStateGetTaskData();

    switch (phy_state->state)
    {
        /* Already in the case, why are we getting told again? */
        case PHY_STATE_IN_CASE:
            break;

        /* Transition through PHY_STATE_OUT_OF_EAR before transitioning
           to PHY_STATE_IN_CASE */
        case PHY_STATE_OUT_OF_EAR_AT_REST:
        case PHY_STATE_IN_EAR:
            appPhyStateSetState(PhyStateGetTaskData(), PHY_STATE_OUT_OF_EAR);
            /* FALLTHROUGH */

        case PHY_STATE_UNKNOWN:
        case PHY_STATE_OUT_OF_EAR:
            appPhyStateSetState(PhyStateGetTaskData(), PHY_STATE_IN_CASE);
            break;

        default:
            appPhyStateHandleBadState(phy_state->state);
            break;
    }
}

/*! \brief Handle notification that Earbud is now out of the case. */
static void appPhyStateHandleInternalOutOfCaseEvent(void)
{
    phyStateTaskData *phy_state = PhyStateGetTaskData();

    switch (phy_state->state)
    {
        /* Already out of the case, why are we being told again? */
        case PHY_STATE_OUT_OF_EAR:
        case PHY_STATE_OUT_OF_EAR_AT_REST:
        case PHY_STATE_IN_EAR:
            break;

        /* Transition based on the stored sensor states */
        case PHY_STATE_IN_CASE:
        case PHY_STATE_UNKNOWN:
            appPhyStateSetState(PhyStateGetTaskData(), PHY_STATE_OUT_OF_EAR);
            if (phy_state->in_proximity)
            {
                appPhyStateSetState(PhyStateGetTaskData(), PHY_STATE_IN_EAR);
#ifdef ENABLE_TYM_PLATFORM
                appPhyStateInEarPromptCheck();
#endif                
            }
            else if (!phy_state->in_motion)
            {
                appPhyStateSetState(PhyStateGetTaskData(), PHY_STATE_OUT_OF_EAR_AT_REST);
            }
            break;

        default:
            appPhyStateHandleBadState(phy_state->state);
            break;
    }
}

/*! \brief Handle notification that Earbud is now in ear. */
static void appPhyStateHandleInternalInEarEvent(void)
{
    phyStateTaskData *phy_state = PhyStateGetTaskData();

    /* Save state to use in determining state on further events */
    phy_state->in_proximity = TRUE;

    switch (phy_state->state)
    {
        /* Until there is an out of case event, the state should remain in-case */
        case PHY_STATE_IN_CASE:
            break;

        /* Already out of the case, why are we being told again? */
        case PHY_STATE_IN_EAR:
            break;

        /* Transition through PHY_STATE_OUT_OF_EAR before transitioning
           to PHY_STATE_IN_EAR */
        case PHY_STATE_OUT_OF_EAR_AT_REST:
            appPhyStateSetState(PhyStateGetTaskData(), PHY_STATE_OUT_OF_EAR);
            // FALLTHROUGH

        case PHY_STATE_UNKNOWN:
        case PHY_STATE_OUT_OF_EAR:
            appPhyStateSetState(PhyStateGetTaskData(), PHY_STATE_IN_EAR);
#ifdef ENABLE_TYM_PLATFORM
            appPhyStateInEarPromptCheck();
#endif            
            break;

        default:
            appPhyStateHandleBadState(phy_state->state);
            break;
    }
}

/*! \brief Handle notification that Earbud is now out of the ear. */
static void appPhyStateHandleInternalOutOfEarEvent(void)
{
    phyStateTaskData *phy_state = PhyStateGetTaskData();

    /* Save state to use in determining state on further events */
    phy_state->in_proximity = FALSE;

    switch (phy_state->state)
    {
        /* Until there is an out of case event, the state should remain in-case */
        case PHY_STATE_IN_CASE:
            break;

        /* Already out of ear, why are we being told again? */
        case PHY_STATE_OUT_OF_EAR:
        case PHY_STATE_OUT_OF_EAR_AT_REST:
            break;

        case PHY_STATE_UNKNOWN:
        case PHY_STATE_IN_EAR:
            appPhyStateSetState(PhyStateGetTaskData(), PHY_STATE_OUT_OF_EAR);
            if (!phy_state->in_motion)
            {
                appPhyStateSetState(PhyStateGetTaskData(), PHY_STATE_OUT_OF_EAR_AT_REST);
            }
            break;

        default:
            appPhyStateHandleBadState(phy_state->state);
            break;
    }
}

/*! \brief Handle notification that Earbud is now moving */
static void appPhyStateHandleInternalMotionEvent(void)
{
    phyStateTaskData *phy_state = PhyStateGetTaskData();

    /* Save motion state to use in determining state on further events */
    phy_state->in_motion = TRUE;

    switch (phy_state->state)
    {
        /* Don't care */
        case PHY_STATE_OUT_OF_EAR:
        case PHY_STATE_IN_CASE:
        case PHY_STATE_IN_EAR:
            break;

        /* Motion event alone does not provide enough information to determine correct state */
        case PHY_STATE_UNKNOWN:
            break;

        /* Earbud was moved while not in ear, when previously not in motion */
        case PHY_STATE_OUT_OF_EAR_AT_REST:
            appPhyStateSetState(PhyStateGetTaskData(), PHY_STATE_OUT_OF_EAR);
            break;

        default:
            appPhyStateHandleBadState(phy_state->state);
            break;
    }
}

/*! \brief Handle notification that Earbud is now not moving. */
static void appPhyStateHandleInternalNotInMotionEvent(void)
{
    phyStateTaskData *phy_state = PhyStateGetTaskData();
    /* Save motion state to use in determining state on further events */
    phy_state->in_motion = FALSE;

    switch (phy_state->state)
    {
        /* Don't care */
        case PHY_STATE_OUT_OF_EAR_AT_REST:
        case PHY_STATE_IN_CASE:
        case PHY_STATE_IN_EAR:
            break;

        /* Motion event alone does not provide enough information to determine correct state */
        case PHY_STATE_UNKNOWN:
            break;

        /* Earbud out of ear and stopped moving */
        case PHY_STATE_OUT_OF_EAR:
            appPhyStateSetState(PhyStateGetTaskData(), PHY_STATE_OUT_OF_EAR_AT_REST);
            break;

        default:
            appPhyStateHandleBadState(phy_state->state);
            break;
    }
}

/*! \brief When notification limit timer fires, progress the state machine if current
           state doesn't equal reported state.
*/
static void appPhyStateHandleInternalTimeoutNotificationLimit(void)
{
    phyStateTaskData *phy_state = PhyStateGetTaskData();
    
    if (phy_state->state != phy_state->reported_state)
    {
        appPhyStateSetState(phy_state, phy_state->state);
    }
}

/*! \brief Physical State module message handler. */
static void appPhyStateHandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    switch (id)
    {
        case PHY_STATE_INTERNAL_IN_CASE_EVENT:
            appPhyStateHandleInternalInCaseEvent();
            break;
        case PHY_STATE_INTERNAL_OUT_OF_CASE_EVENT:
            appPhyStateHandleInternalOutOfCaseEvent();
            break;
        case PHY_STATE_INTERNAL_IN_EAR_EVENT:
            appPhyStateHandleInternalInEarEvent();
            break;
        case PHY_STATE_INTERNAL_OUT_OF_EAR_EVENT:
            appPhyStateHandleInternalOutOfEarEvent();
            break;
        case PHY_STATE_INTERNAL_MOTION:
            appPhyStateHandleInternalMotionEvent();
            break;
        case PHY_STATE_INTERNAL_NOT_IN_MOTION:
            appPhyStateHandleInternalNotInMotionEvent();
            break;

        case PHY_STATE_INTERNAL_TIMEOUT_NOTIFICATION_LIMIT:
            appPhyStateHandleInternalTimeoutNotificationLimit();
            break;

        case CHARGER_MESSAGE_ATTACHED:
            appPhyStateInCaseEvent();
            break;
        case CHARGER_MESSAGE_DETACHED:
            appPhyStateOutOfCaseEvent();
            break;
        case CHARGER_MESSAGE_CHARGING_OK:
        case CHARGER_MESSAGE_CHARGING_LOW:
            /* Consume this frequently occuring message with no operation required. */
            break;

        case ACCELEROMETER_MESSAGE_IN_MOTION:
            appPhyStateMotionEvent();
            break;
        case ACCELEROMETER_MESSAGE_NOT_IN_MOTION:
            appPhyStateNotInMotionEvent();
            break;
        case PROXIMITY_MESSAGE_IN_PROXIMITY:
            appPhyStateInEarEvent();
            break;
        case PROXIMITY_MESSAGE_NOT_IN_PROXIMITY:
            appPhyStateOutOfEarEvent();
            break;
#ifdef ENABLE_TYM_PLATFORM
        case TOUCH_MESSAGE_TAPx1:
            DEBUG_LOG("TOUCH_MESSAGE_TAPx1");
            appPhyStateTapx1();
            //LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_toggle_play_pause);
            break;
        case TOUCH_MESSAGE_TAPx2:
            DEBUG_LOG("TOUCH_MESSAGE_TAPx2");
            appPhyStateTapx2();
            break;
        case TOUCH_MESSAGE_TAPx3:
            DEBUG_LOG("TOUCH_MESSAGE_TAPx3");
            appPhyStateTapx3();
            break;
        case TOUCH_MESSAGE_SWIPEL:
            DEBUG_LOG("TOUCH_MESSAGE_SWIPEL");
            if(Multidevice_IsLeft()) /*left earbud run left function */
            {
                appPhyStateSwipeL(uiseq_left_swipe);
                //MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_FORWARD, NULL);
            }
            else
            {
                appPhyStateSwipeL(uiseq_right_swipe);
                //MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_VOLUME_UP, NULL);
            }
            break;
        case TOUCH_MESSAGE_SWIPER:
            DEBUG_LOG("TOUCH_MESSAGE_SWIPER");
            if(Multidevice_IsLeft()) /*left earbud run left function */
            {
                appPhyStateSwipeR(uiseq_left_swipe);
                //MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_BACKWARD, NULL);
            }
            else
            {
                appPhyStateSwipeR(uiseq_right_swipe);
                //MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_VOLUME_DOWN, NULL);
            }
            break;
        case TOUCH_MESSAGE_HOLD2S:
            DEBUG_LOG("TOUCH_MESSAGE_HOLD2S,appHfpIsCallIncoming %d,ScoFwdIsCallIncoming %d",appHfpIsCallIncoming(),ScoFwdIsCallIncoming());
            DEBUG_LOG("TOUCH_MESSAGE_HOLD2S,appHfpIsCallActive %d,ScoFwdIsStreaming %d,appHfpIsCallOutgoing %d",appHfpIsCallActive(),ScoFwdIsStreaming(),appHfpIsCallOutgoing());
            if((appHfpIsCallIncoming() == TRUE)|| (ScoFwdIsCallIncoming() == TRUE))
            {
                LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_voice_call_reject);
                DEBUG_LOG("call ui_input_voice_call_reject");
            }
            else if((appHfpIsCallActive() == TRUE) || (ScoFwdIsStreaming() == TRUE))
            {
                //MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_TAPX2, NULL);
                LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_voice_call_hang_up);
                DEBUG_LOG("call ui_input_voice_call_hang_up");
            }
            else
            {    
                LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_va_presshold);
            }
            break;
        case TOUCH_MESSAGE_HOLD5S:
            DEBUG_LOG("TOUCH_MESSAGE_HOLD5S");
            if(StateProxy_IsInCase() == TRUE)
            {
                MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_HANDSET_PAIRING, NULL);
            }
            else
            {
                if(Multidevice_IsLeft())
                    tymSyncdata(noCasePairCmd,TYM_LEFT_EARBUD|1);
                else
                    tymSyncdata(noCasePairCmd,TYM_RIGHT_EARBUD|1);
            }
            break;
        case TOUCH_MESSAGE_HOLD10S:
            DEBUG_LOG("TOUCH_MESSAGE_HOLD10S");
            MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_DELETE_HANDSET, NULL);
            break;
        case TOUCH_MESSAGE_HOLD2SEND:
            DEBUG_LOG("TOUCH_MESSAGE_HOLD2SEND");
            /*va release*/
            LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_va_6);
            break;
        case TOUCH_MESSAGE_HOLD5SEND:
            DEBUG_LOG("TOUCH_MESSAGE_HOLD5SEND");
            if(Multidevice_IsLeft())
                tymSyncdata(noCasePairCmd,TYM_LEFT_EARBUD);
            else
                tymSyncdata(noCasePairCmd,TYM_RIGHT_EARBUD);
            break;
         case PHY_STATE_INTERNAL_TIMEOUT_SLEEPMODE:
            DEBUG_LOG("Sleep mode timeout");
            if(getFactoryModeEnable() == FALSE) /*not factory mode */
            {
                if(StateProxy_IsSleepMode() == FALSE)
                {
                    //appPhyStateMsgSendStateChangedInd(appPhyStateGetState(), phy_state_event_enter_sleepmode);
                    tymSyncdata(sleepStandbyModeCmd, phy_state_event_enter_sleepmode);
                }
            }
            break;
         case PHY_STATE_INTERNAL_TIMEOUT_STANDBYMODE:
            DEBUG_LOG("Standby mode timeout");
            if(getFactoryModeEnable() == FALSE) /*not factory mode */
            {
                tymSyncdata(sleepStandbyModeCmd, phy_state_event_enter_standbymode);
            }
            break;
#endif
        default:
            DEBUG_LOG("Unknown message received, id:%d", id);
            break;
    }
}

/*! \brief Register a task for notification of changes in state.
 */
void appPhyStateRegisterClient(Task client_task)
{
    DEBUG_LOG("appPhyStateRegisterClient %p", client_task);

    TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(PhyStateGetClientTasks()), client_task);
}

void appPhyStateUnregisterClient(Task client_task)
{
    DEBUG_LOG("appPhyStateUnregisterClient %p", client_task);

    TaskList_RemoveTask(TaskList_GetFlexibleBaseTaskList(PhyStateGetClientTasks()), client_task);
}

bool appPhyStateInit(Task init_task)
{
    phyStateTaskData* phy_state = PhyStateGetTaskData();
    DEBUG_LOG("appPhyStateInit");

    phy_state->task.handler = appPhyStateHandleMessage;
    phy_state->state = PHY_STATE_UNKNOWN;
    phy_state->reported_state = PHY_STATE_UNKNOWN;
    TaskList_InitialiseWithCapacity(PhyStateGetClientTasks(), PHY_STATE_CLIENT_TASK_LIST_INIT_CAPACITY);
    phy_state->in_motion = FALSE;
    phy_state->in_proximity = FALSE;
#ifdef ENABLE_TYM_PLATFORM
    phy_state->poweron = TRUE;
    phy_state->trigger_sleepmode = FALSE;
    phy_state->trigger_standbymode = FALSE;
#endif
/* Not registering as a client of the charger means no charger state messages
will be received. This means the in-case state can never be entered */
#ifndef DISABLE_IN_CASE_PHY_STATE
    appChargerClientRegister(&phy_state->task);
#else
    /* Without charger messages, remain out of case */
    MessageSend(&phy_state->task, CHARGER_MESSAGE_DETACHED, NULL);
#endif
#ifdef ENABLE_TYM_PLATFORM
    if(getFactoryModeTopEnable() == TRUE)
    {
        DEBUG_LOG("ByPass Prox/anc register");
    }
    else
    {
        if (!appProximityClientRegister(&phy_state->task))
        {
            DEBUG_LOG("appPhyStateRegisterClient unable to register with proximity");
            /* Without proximity detection, assume the device is always in-ear */
            MessageSend(&phy_state->task, PROXIMITY_MESSAGE_IN_PROXIMITY, NULL);
        }

        if (!appAncClientRegister(&phy_state->task))
        {
            DEBUG_LOG("appPhyStateRegisterClient unable to register with anc");
        }
        else
        {
            DEBUG_LOG("appPhyStateRegisterClient register with anc");
        }
    }
#else
    if (!appProximityClientRegister(&phy_state->task))
    {
        DEBUG_LOG("appPhyStateRegisterClient unable to register with proximity");
        /* Without proximity detection, assume the device is always in-ear */
        MessageSend(&phy_state->task, PROXIMITY_MESSAGE_IN_PROXIMITY, NULL);
    }
#endif
    if (!appAccelerometerClientRegister(&phy_state->task))
    {
        DEBUG_LOG("appPhyStateRegisterClient unable to register with accelerometer");
        /* Without accelerometer motion detection, assume the device is always moving */
        MessageSend(&phy_state->task, ACCELEROMETER_MESSAGE_IN_MOTION, NULL);
    }
#ifdef ENABLE_TYM_PLATFORM
    if (!appTouchClientRegister(&phy_state->task))
    {
        DEBUG_LOG("appPhyStateRegisterClient unable to register with touch");
    }
    else
    {
        DEBUG_LOG("appPhyStateRegisterClient register with touch");
    }
#endif
    /* Supported sensors will send initial messages when the first measurement is made */
    phy_state->lock = PHY_STATE_LOCK_EAR | PHY_STATE_LOCK_MOTION | PHY_STATE_LOCK_CASE;
    MessageSendConditionally(init_task, PHY_STATE_INIT_CFM, NULL, &phy_state->lock);

#ifdef ENABLE_TYM_PLATFORM
    appPhyStateMsgSendStateChangedInd(PHY_STATE_IN_CASE, phy_state_event_user_poweron);
    setSystemReady(TRUE);
    appPhySateAppConfiguration();
#endif
    return TRUE;
}

void appPhyStatePrepareToEnterDormant(void)
{
#ifdef ENABLE_TYM_PLATFORM
    DEBUG_LOG("appPhyStatePrepareToEnterDormant");
 
    if(getFactoryModeTopEnable() == TRUE)
    {
        DEBUG_LOG("ByPass Prox/anc register");
    }
    else
    {
        AncStateManager_Disable();
        appAncPowerOff();
        appProximityPowerOff();
        //appAncClientUnregister(&phy_state->task);
        //appProximityClientUnregister(&phy_state->task);
    }
    appTouchPowerOff(); /*touch power is i2c power so last power off*/
    //appTouchClientUnregister(&phy_state->task);
    setPSPresetEQ();
    if((getFactoryModeEnable() == TRUE) || (getFactoryModeTopEnable() == TRUE))
        setFactoryModeStatus(factory_disable);

#else
    phyStateTaskData* phy_state = PhyStateGetTaskData();
    DEBUG_LOG("appPhyStatePrepareToEnterDormant");
    appProximityClientUnregister(&phy_state->task);
    appAccelerometerClientUnregister(&phy_state->task);
#endif
}


/*! \brief Handle state transitions.

    \param  phy_state   Pointer to the task data for this module.
    \param  new_state   State to set.
 */
void appPhyStateSetState(phyStateTaskData* phy_state, phyState new_state)
{
    int32 due;

    DEBUG_LOG_STATE("appPhyStateSetState current %d reported %d new %d", phy_state->state, phy_state->reported_state, new_state);

    /* always update true state of the device */
    PhyStateGetTaskData()->state = new_state;

    /* If we're within the notification limit timer, don't perform any state transition that
     * would notify clients, when the limit expires this state transition will be kicked
     * again */
    if (MessagePendingFirst(PhyStateGetTask(), PHY_STATE_INTERNAL_TIMEOUT_NOTIFICATION_LIMIT, &due))
    {
        return;
    }

    /* The state machine reflects what has been reported to clients, so transitions are
     * based on reported_state not state */
    switch (phy_state->reported_state)
    {
        case PHY_STATE_UNKNOWN:
            appPhyStateExitUnknown();
            break;
        case PHY_STATE_IN_CASE:
            appPhyStateExitInCase();
            break;
        case PHY_STATE_OUT_OF_EAR:
            appPhyStateExitOutOfEar();
            break;
        case PHY_STATE_OUT_OF_EAR_AT_REST:
            appPhyStateExitOutOfEarAtRest();
            break;
        case PHY_STATE_IN_EAR:
            appPhyStateExitInEar();
            break;
        default:
            appPhyStateHandleBadState(phy_state->reported_state);
            break;
    }

    /* Take advantage of the structure of the phy_state FSM to update reported state,
     * as all transitions go through PHY_STATE_OUT_OF_EAR and we report all transitions
     * one step at a time. */
    /* PHY_STATE_UNKNOWN
        - Initialising, set new_state
       PHY_STATE_OUT_OF_EAR
        - Can only move to to the latest state, set new_state
       PHY_STATE_IN_CASE
       PHY_STATE_OUT_OF_EAR_AT_REST
       PHY_STATE_IN_EAR
        - Otherwise we were in PHY_STATE_IN_CASE, PHY_STATE_IN_EAR or PHY_STATE_OUT_OF_EAR_AT_REST
          and are now either in PHY_STATE_OUT_OF_EAR or a different one of those 3 but
          having passed through PHY_STATE_OUT_OF_EAR, either way that is the next 
          reported state. */
    if ((phy_state->reported_state == PHY_STATE_UNKNOWN) || (phy_state->reported_state == PHY_STATE_OUT_OF_EAR))
    {
        phy_state->reported_state = new_state;
    }
    else
    {
        phy_state->reported_state = PHY_STATE_OUT_OF_EAR;
    }

    switch (phy_state->reported_state)
    {
        case PHY_STATE_IN_CASE:
            appPhyStateEnterInCase();
            break;
        case PHY_STATE_OUT_OF_EAR:
            appPhyStateEnterOutOfEar();
            break;
        case PHY_STATE_OUT_OF_EAR_AT_REST:
            appPhyStateEnterOutOfEarAtRest();
            break;
        case PHY_STATE_IN_EAR:
            appPhyStateEnterInEar();
            break;
        default:
            appPhyStateHandleBadState(phy_state->reported_state);
            break;
    }

}

/*! \brief Get the current physical state of the device.
    \return phyState Current physical state of the device.
*/
phyState appPhyStateGetState(void)
{
    phyStateTaskData* phy_state = PhyStateGetTaskData();
    return phy_state->state;
}

/*! \brief Check whether in/out of ear detection is supported.
    \return bool TRUE if in/out of ear detection is supported. */
bool appPhyStateIsInEarDetectionSupported(void)
{
    return appProximityIsClientRegistered(&(PhyStateGetTaskData()->task));
}

bool appPhyStateIsOutOfCase(void)
{
    phyState physical_state = appPhyStateGetState();

    switch(physical_state)
    {
            /* treat unknown as in case. This is a transient state that
               should not be encountered. If the "wrong" value is returned
               then there will be subsequent events when the correct state
               is known. */
        case PHY_STATE_UNKNOWN:
        case PHY_STATE_IN_CASE:
            return FALSE;

        case PHY_STATE_OUT_OF_EAR:
        case PHY_STATE_OUT_OF_EAR_AT_REST:
        case PHY_STATE_IN_EAR:
            return TRUE;

        default:
            DEBUG_LOG("appPhyStateIsOutOfCase. Unexpected phyState:%d", physical_state);
            Panic();
    }
    return FALSE;
}


/*! \brief Handle notification that Earbud is now in the case. */
void appPhyStateInCaseEvent(void)
{
    phyStateTaskData* phy_state = PhyStateGetTaskData();
    MessageCancelAll(&phy_state->task, PHY_STATE_INTERNAL_IN_CASE_EVENT);
    MessageSend(&phy_state->task, PHY_STATE_INTERNAL_IN_CASE_EVENT, NULL);
    appPhyStateClearLockBit(PHY_STATE_LOCK_CASE);
}

/*! \brief Handle notification that Earbud is now out of the case. */
void appPhyStateOutOfCaseEvent(void)
{
    phyStateTaskData* phy_state = PhyStateGetTaskData();
    MessageCancelAll(&phy_state->task, PHY_STATE_INTERNAL_OUT_OF_CASE_EVENT);
    MessageSend(&phy_state->task, PHY_STATE_INTERNAL_OUT_OF_CASE_EVENT, NULL);
    appPhyStateClearLockBit(PHY_STATE_LOCK_CASE);
}

/*! \brief Handle notification that Earbud is now in ear. */
void appPhyStateInEarEvent(void)
{
    phyStateTaskData* phy_state = PhyStateGetTaskData();
    MessageCancelAll(&phy_state->task, PHY_STATE_INTERNAL_IN_EAR_EVENT);
    MessageSend(&phy_state->task, PHY_STATE_INTERNAL_IN_EAR_EVENT, NULL);
    appPhyStateClearLockBit(PHY_STATE_LOCK_EAR);
}

/*! \brief Handle notification that Earbud is now out of the ear. */
void appPhyStateOutOfEarEvent(void)
{
    phyStateTaskData* phy_state = PhyStateGetTaskData();
    MessageCancelAll(&phy_state->task, PHY_STATE_INTERNAL_OUT_OF_EAR_EVENT);
    MessageSend(&phy_state->task, PHY_STATE_INTERNAL_OUT_OF_EAR_EVENT, NULL);
    appPhyStateClearLockBit(PHY_STATE_LOCK_EAR);
}

/*! \brief Handle notification that Earbud is now moving */
void appPhyStateMotionEvent(void)
{
    phyStateTaskData* phy_state = PhyStateGetTaskData();
    MessageCancelAll(&phy_state->task, PHY_STATE_INTERNAL_MOTION);
    MessageSend(&phy_state->task, PHY_STATE_INTERNAL_MOTION, NULL);
    appPhyStateClearLockBit(PHY_STATE_LOCK_MOTION);
}

/*! \brief Handle notification that Earbud is now not moving. */
void appPhyStateNotInMotionEvent(void)
{
    phyStateTaskData* phy_state = PhyStateGetTaskData();
    MessageCancelAll(&phy_state->task, PHY_STATE_INTERNAL_NOT_IN_MOTION);
    MessageSend(&phy_state->task, PHY_STATE_INTERNAL_NOT_IN_MOTION, NULL);
    appPhyStateClearLockBit(PHY_STATE_LOCK_MOTION);
}

#ifdef ENABLE_TYM_PLATFORM
/*typedef enum{
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
}custom_ui_seq; */
void appPhySateAppConfiguration(void)
{
    tym_sync_app_configuration_t *app_set = TymGet_AppSetting();
    if(PsRetrieve(PSID_APPCONFIG, 0, 0) == 0) //no data run initial information
    {
        app_set->auto_power_off_cmd = 0x01;//app power off configure 10 min
        app_set->auto_power_off_timer = 10;//30 min;
        app_set->enable_auto_wear = 1; //default no wear pause
        app_set->enable_auto_play = 0; //add default value auto play is zero
        app_set->ambient_ext_anc = 0;//default ambient set stanc3 off
        app_set->custom_ui[uiseq_left_swipe] = uifunc_track;
        app_set->custom_ui[uiseq_right_swipe] = uifunc_vol;
        app_set->custom_ui[uiseq_left_tapx1] = uifunc_play_pause_with_amb;
        app_set->custom_ui[uiseq_right_tapx1] = uifunc_play_pause_with_amb;
        app_set->custom_ui[uiseq_left_tapx2] = uifunc_anc_amb;
#ifdef INCLUDE_GAA        
        app_set->custom_ui[uiseq_right_tapx2] = uifunc_google_notification;
#else
        app_set->custom_ui[uiseq_right_tapx2] = uifunc_anc_amb;
#endif        
        app_set->custom_ui[uiseq_left_tapx3] = uifunc_disable;
        app_set->custom_ui[uiseq_right_tapx3] = uifunc_disable;
        PsStore(PSID_APPCONFIG, app_set, PS_SIZE_ADJ(sizeof(tym_sync_app_configuration_t)));
    }
    else
    {
        PsRetrieve(PSID_APPCONFIG, app_set, PS_SIZE_ADJ(sizeof(tym_sync_app_configuration_t)));
    }
}

/*! \brief Handle notification that Earbud is now in the case for OTA mode. */
void appPhyStateInCaseEventForOTA(void)
{
    DEBUG_LOG("appPhyStateInCaseEventForOTA");
    phyStateTaskData* phy_state = PhyStateGetTaskData();
    phy_state->ota_incase = TRUE;
    MessageCancelAll(&phy_state->task, PHY_STATE_INTERNAL_IN_CASE_EVENT);
    MessageSend(&phy_state->task, PHY_STATE_INTERNAL_IN_CASE_EVENT, NULL);
    appPhyStateClearLockBit(PHY_STATE_LOCK_CASE);
}

/*! \brief Handle connected prompt check in ear. */
void appPhyStateInEarPromptCheck(void)
{
    DEBUG_LOG("appPhyStateInEarPromptCheck btstatus %d,Prompt %d",tymGetBTStatus(),Prompts_GetConnectedStatus());
    /*connected & no play connected prompts , request connected prompts play*/
    if((tymGetBTStatus() == btConnected) || (tymGetBTStatus() == btPairingSuccessful))
    {
        if(Prompts_GetConnectedStatus() == FALSE)
            Ui_InjectUiInput(ui_input_prompt_connected_check);
    }
}

/*! \brief Handle tym phy state power on. */
void appPhyStatePowerOnEvent(void)
{
    phyStateTaskData* phy_state = PhyStateGetTaskData();
    /*add patch for SSP, if anc calibration is no recovery, force recovery it*/
    if(phy_state->anc_cal == TRUE)
    {
        phy_state->anc_cal = FALSE;
        disable_i2c_for_cal(0);//power on
        if(checkANCVerifyValue())
            dumpANCWriteToPSKey();
    }
    phy_state->poweron = TRUE;
    appPhyStateMsgSendStateChangedInd(appPhyStateGetState(), phy_state_event_user_poweron);
}

/*! \brief Handle tym phy state power off. */
void appPhyStatePowerOffEvent(void)
{
    phyStateTaskData* phy_state = PhyStateGetTaskData();
    phy_state->poweron = FALSE;
    appPhyStateCancelTriggerSleepMode();
    appPhyStateCancelTriggerStandbyMode();
    if(appSmIsPrimary())
    {    
        appExitHandsetPairing();
        tymSyncdata(btStatusCmd,btConnectable);   //power off sync bt status connectable
    }
    else
    {
        reportBtStatus(btConnectable); /*power off maybe can't receive btConnectable , force clear bt status*/
    }    
    appPhyStateMsgSendStateChangedInd(appPhyStateGetState(), phy_state_event_user_poweroff);
    if(getFactoryModeEnable() == TRUE)
    {
        DEBUG_LOG("bt status %d",tymGetBTStatus());
        if((tymGetBTStatus() == btConnected) || (tymGetBTStatus() == btPairingSuccessful))
        {
            appDisconnectAll();
        }
        if(tymGetBTStatus() == btPairing)
        {
            appExitHandsetPairing();
        }
    }
}

/*! \brief Handle get tym power on status. */
bool appPhyStateGetPowerState(void)
{
    phyStateTaskData* phy_state = PhyStateGetTaskData();
    return  phy_state->poweron;
}

/*! \brief Handle ANC calibration for factory. */
void appPhyStateAncCalibration(void)
{
    phyStateTaskData* phy_state = PhyStateGetTaskData();
    if(getFactoryModeEnable() == TRUE)
    {
        if(phy_state->anc_cal == FALSE)
        {
            phy_state->anc_cal = TRUE;
            /*disable I2C communication*/
            disable_i2c_for_cal(1);
            reportDoAncCalibration();
        }
        else
        {
            phy_state->anc_cal = FALSE;
            /*enable I2C communication*/
            /*read ANC register and write to PSKEY*/
            disable_i2c_for_cal(0);
            if(checkANCVerifyValue())
                dumpANCWriteToPSKey();
        }
    }
}

static void appPhyTapANCEvent(void)
{
    if((appHfpIsCallIncoming() == TRUE)|| (ScoFwdIsCallIncoming() == TRUE) || (appHfpIsCallActive() == TRUE) || (ScoFwdIsStreaming() == TRUE))
    {
        DEBUG_LOG("HFP working,don't switch ANC");
    }
    else
    {
        MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_TAP_ANC, NULL);
    }        
}

/*! \brief based on action (left/right) tapx1,tapx2,tapx3,swipeL,swipeR get custom ui functin id*/
uint8 appPhyStateGetCustomUiId(uint8 act)
{
    tym_sync_app_configuration_t *app_set = TymGet_AppSetting();
    uint8 funcid = uifunc_disable;
    if(act < uiseq_maximum)
    {
        funcid = app_set->custom_ui[act];
    }
    return funcid;
}


/*! \brief tapx1 action based on custom ui id*/
void appPhyStateCustomIdTapx1(uint8 act)
{
    uint8 id = appPhyStateGetCustomUiId(act);
    if(id == uifunc_play_pause_with_amb)
    {
#ifdef INCLUDE_GAA        
        LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_va_cancel_ambient);
#else
        LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_bell_ui_pp_ambient);//first ambient check,play ->ambient
        LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_toggle_play_pause);
#endif        
    }
    else if(id == uifunc_play_pause)
    {
#ifdef INCLUDE_GAA          
        LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_va_cancel);
#else
        LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_toggle_play_pause);
#endif        
    }
}

/*! \brief tapx1 ui*/
void appPhyStateTapx1(void)
{
    //Incomming
    DEBUG_LOG("appHfpGetState() %d",appHfpGetState());
    if((appHfpIsCallIncoming() == TRUE)|| (ScoFwdIsCallIncoming() == TRUE))
    {
        DEBUG_LOG("Have Call InComing");
        MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_TAPX1, NULL);
    }
    else
    {
        if(Multidevice_IsLeft())
        {
            appPhyStateCustomIdTapx1(uiseq_left_tapx1);
            //LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_va_cancel_ambient);
        }
        else
        {
            appPhyStateCustomIdTapx1(uiseq_right_tapx1);
            //LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_va_cancel_ambient);
        }
    }

}

/*! \brief tapx2 for cutom UI id */
void appPhyStateCustomIdTapx2(uint8 act)
{
    uint8 id = appPhyStateGetCustomUiId(act);
    if(id == uifunc_anc_amb) /* switch between ambient and ANC */
    {
        //MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_TAP_ANC, NULL);
        appPhyTapANCEvent();
    }
    else if(id == uifunc_google_notification) /* google bisto notification */
    {
        MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_TAP_BISTO, NULL);
    }
    else if(id == uifunc_track) /*track , next track */
    {
        MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_FORWARD, NULL);
    }
}

/*! \brief tapx2 ui*/
void appPhyStateTapx2(void)
{
    uint8 touchPadMode = tymGetTouchPadMode();
    if(touchPadMode == standbyPad)
    {
        appUserPowerOn();
        return;
    }
    else if(touchPadMode == sleepPad)
    {
        appPhyStateCancelTriggerSleepMode();
        appPhyStateCancelTriggerStandbyMode();
        tymSyncdata(sleepStandbyModeCmd, phy_state_event_leave_sleepmode);
        return;
    }
    /* HFP on Incomming */
    /*if((appHfpIsCallIncoming() == TRUE)|| (ScoFwdIsCallIncoming() == TRUE))
    {
        MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_TAPX2, NULL);
    }
    else if((appHfpIsCallActive() == TRUE) || (ScoFwdIsStreaming() == TRUE))
    {
        MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_TAPX2, NULL);
    }*/
    else if(Multidevice_IsLeft())
    {
        appPhyStateCustomIdTapx2(uiseq_left_tapx2);
        //ANC on/off
        //MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_TAP_ANC, NULL);
    }
    else //right
    {
        appPhyStateCustomIdTapx2(uiseq_right_tapx2);
        //MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_TAP_BISTO, NULL);
    }
}

/*! \brief tapx3 ui*/
void appPhyStateTapx3(void)
{
    bool left = Multidevice_IsLeft();
    uint8 id = uifunc_disable;
    if(left)
    {
        id = appPhyStateGetCustomUiId(uiseq_left_tapx3);
    }
    else
    {
        id = appPhyStateGetCustomUiId(uiseq_right_tapx3);
    }

    if(id == uifunc_track)
    {
        MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_BACKWARD, NULL);
    }
}

/*! \brief swipe left ui */
void appPhyStateSwipeL(uint8 act)
{
    uint8 id = appPhyStateGetCustomUiId(act);
    if((appHfpIsCallIncoming() == TRUE)|| (ScoFwdIsCallIncoming() == TRUE))
    {
        MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_VOLUME_UP, NULL);
    }
    else if((appHfpIsCallActive() == TRUE) || (ScoFwdIsStreaming() == TRUE))
    {
        MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_VOLUME_UP, NULL);
    }
    else if(id == uifunc_vol) /* volume => volume up */
    {
        MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_VOLUME_UP, NULL);
    }
    else if(id == uifunc_track) /* track ==> next track */
    {
        MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_FORWARD, NULL);
    }
    else if(id == uifunc_anc_amb) /*anc, anc ambient */
    {
        //MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_TAP_ANC, NULL);
        appPhyTapANCEvent();
    }
}

/*! \brief swipe right ui */
void appPhyStateSwipeR(uint8 act)
{
    uint8 id = appPhyStateGetCustomUiId(act);
    if((appHfpIsCallIncoming() == TRUE)|| (ScoFwdIsCallIncoming() == TRUE))
    {
        MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_VOLUME_DOWN, NULL);
    }
    else if((appHfpIsCallActive() == TRUE) || (ScoFwdIsStreaming() == TRUE))
    {
        MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_VOLUME_DOWN, NULL);
    }
    else if(id == uifunc_vol) /* volume => volume down */
    {
        MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_VOLUME_DOWN, NULL);
    }
    else if(id == uifunc_track) /* track ==> previous track */
    {
        MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_BACKWARD, NULL);
    }
    else if(id == uifunc_anc_amb) /*anc, anc ambient */
    {
        //MessageSend(LogicalInputSwitch_GetTask(), APP_BUTTON_TAP_ANC, NULL);
        appPhyTapANCEvent();
    }
}

bool appPhySleepModeIsTriggerCount(void)
{
    phyStateTaskData* phy_state = PhyStateGetTaskData();
    return phy_state->trigger_sleepmode;
}

bool appPhyStandbyModeIsTriggerCount(void)
{
    phyStateTaskData* phy_state = PhyStateGetTaskData();
    return phy_state->trigger_standbymode;
}

bool appPhyStandbyModeInCaseIsTriggerCount(void)
{
    phyStateTaskData* phy_state = PhyStateGetTaskData();
    return phy_state->trigger_standbymodeincase;
}

/*! \brief start count to sleep mode */
void appPhyStateTriggerSleepMode(void)
{
    phyStateTaskData* phy_state = PhyStateGetTaskData();
    DEBUG_LOG("appPhyStateTriggerSleepMode");
    if(appSmIsPrimary() == FALSE)
        return;
    if(phy_state->trigger_sleepmode == FALSE)
    {
        DEBUG_LOG("appPhyStateTriggerSleepMode wait timeout");
        phy_state->trigger_sleepmode = TRUE;
        MessageSendLater(PhyStateGetTask(),PHY_STATE_INTERNAL_TIMEOUT_SLEEPMODE, NULL, D_MIN(10));
        //MessageSendLater(PhyStateGetTask(),PHY_STATE_INTERNAL_TIMEOUT_SLEEPMODE, NULL, D_MIN(1));
    }
}

void appPhyStateCancelTriggerSleepMode(void)
{
    phyStateTaskData* phy_state = PhyStateGetTaskData();
    if(phy_state->trigger_sleepmode == TRUE)
    {
        DEBUG_LOG("appPhyStateCancelTriggerSleepMode");
        phy_state->trigger_sleepmode = FALSE;
        MessageCancelFirst(PhyStateGetTask(),PHY_STATE_INTERNAL_TIMEOUT_SLEEPMODE);
    }
}

void appPhyStateTriggerStandbyMode(void)
{
    phyStateTaskData* phy_state = PhyStateGetTaskData();
    tym_sync_app_configuration_t *app_set = TymGet_AppSetting();
    if(appSmIsPrimary() == FALSE)
        return;
    if(app_set->auto_power_off_timer == 0xff) //cancel
        return;
    if(phy_state->trigger_standbymode == FALSE)
    {
        DEBUG_LOG("appPhyStateTriggerStandbyMode wait timeout");
        phy_state->trigger_standbymode = TRUE;
        MessageSendLater(PhyStateGetTask(),PHY_STATE_INTERNAL_TIMEOUT_STANDBYMODE, NULL, D_MIN(app_set->auto_power_off_timer));
        //MessageSendLater(PhyStateGetTask(),PHY_STATE_INTERNAL_TIMEOUT_STANDBYMODE, NULL, D_MIN(1));
    }
}


void appPhyStateTriggerStandbyModeInCase(void)
{
    phyStateTaskData* phy_state = PhyStateGetTaskData();
    if(appSmIsPrimary() == FALSE)
        return;
    if(phy_state->trigger_standbymode == FALSE)
    {
        DEBUG_LOG("appPhyStateTriggerStandbyModeInCase wait timeout");
        phy_state->trigger_standbymode = TRUE;
        phy_state->trigger_standbymodeincase = TRUE;
        MessageSendLater(PhyStateGetTask(),PHY_STATE_INTERNAL_TIMEOUT_STANDBYMODE, NULL, D_MIN(10));
        //MessageSendLater(PhyStateGetTask(),PHY_STATE_INTERNAL_TIMEOUT_STANDBYMODE, NULL, D_MIN(1));        
    }
}

void appPhyStateCancelTriggerStandbyMode(void)
{
    phyStateTaskData* phy_state = PhyStateGetTaskData();
    if(phy_state->trigger_standbymode == TRUE)
    {
        DEBUG_LOG("appPhyStateCancelTriggerStandbyMode");
        phy_state->trigger_standbymode = FALSE;
        phy_state->trigger_standbymodeincase = FALSE;
        MessageCancelFirst(PhyStateGetTask(),PHY_STATE_INTERNAL_TIMEOUT_STANDBYMODE);
    }
}

void appPhyUpdateSleepStandbyMode(void)
{
    phyStateTaskData* phy_state = PhyStateGetTaskData();
    bool inEar = StateProxy_IsInEar();
    bool peerInEar = StateProxy_IsPeerInEar();
    bool peerInCase = StateProxy_IsPeerInCase();
    bool inCase = StateProxy_IsInCase();    
    bool sleepMode = StateProxy_IsSleepMode();
    bool standbyMode = StateProxy_IsStandbyMode();
    bool record_peerincase = phy_state->record_peerincase;
    bool record_incase = phy_state->record_incase; 
    
    bool poweron = StateProxy_IsPowerOn();
    bool primary = appSmIsPrimary();
    uint8 btstatus = tymGetBTStatus();
    
    phy_state->record_incase = inCase;
    phy_state->record_peerincase = peerInCase;

    if(primary == FALSE)
        return; /* only check mode change from master*/
    if(poweron == FALSE)
        return; /*power off don't check mode*/
    if(btstatus == startOTA)
        return; /* OTA start don't check mode*/    
    /*two bud in case trigger standby mode 10 min*/
    if(standbyMode == FALSE) //not standby
    {
        if((inCase == TRUE) && (peerInCase == TRUE))
        {
            //cancel sleep/standby counter, re-counter 10min
            if(phy_state->trigger_standbymodeincase == FALSE)
            {    
                if(sleepMode)
                {
                    tymSyncdata(sleepStandbyModeCmd, phy_state_event_leave_sleepmode);   
                }    
                appPhyStateCancelTriggerSleepMode();
                appPhyStateCancelTriggerStandbyMode();
                appPhyStateTriggerStandbyModeInCase();
            }
        }
        else if(sleepMode)
        {
            if((peerInCase != record_peerincase) || (inCase != record_incase))
            {
                if(peerInCase || inCase)
                {
                    //leave sleep mode when insert one of buds to charging case              
                    tymSyncdata(sleepStandbyModeCmd, phy_state_event_leave_sleepmode); 
                }    
            } 
            else if(inEar || peerInEar)
            {
                tymSyncdata(sleepStandbyModeCmd, phy_state_event_leave_sleepmode); 
            }    
        }
        else if(inEar || peerInEar)
        {
            appPhyStateCancelTriggerSleepMode();
            appPhyStateCancelTriggerStandbyMode();
        }
        else
        {
            if((sleepMode == FALSE) && (standbyMode == FALSE))
            {                
                //appPhyStateTriggerSleepMode();
                //appPhyStateCancelTriggerStandbyMode();
                /*for customer new specification, remove sleep mode, start trigger standby mode*/
                appPhyStateCancelTriggerSleepMode();
                if(phy_state->trigger_standbymodeincase == TRUE)
                    appPhyStateCancelTriggerStandbyMode();//for re-counter in case standby
                appPhyStateTriggerStandbyMode();
            }
        }                
    }        
}

void appPhyChangeSleepStandbyMode(phy_state_event phyState)
{
    bdaddr handset_addr;
    DEBUG_LOG("sleep/standby mode change %d",phyState);
    appPhyStateMsgSendStateChangedInd(appPhyStateGetState(), phyState);

    if(appSmIsPrimary())
    {
        if(phyState == phy_state_event_enter_sleepmode)
        {
            //disconnect phone
            if (appDeviceIsHandsetConnected())
            {
                EarbudTest_DisconnectHandset();
            }
            else if(tymGetBTStatus() == btPairing)
            {
                //cancel pairing
                appExitHandsetPairing();
            }
            Ui_InjectUiInput(ui_input_prompt_poweroff);
        }
        else if(phyState == phy_state_event_leave_sleepmode)
        {
            //re-connect phone
            if (appDeviceGetHandsetBdAddr(&handset_addr))
            {
                EarbudTest_ConnectHandset();
            }
            //twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_ROLE_SWITCH);
            Ui_InjectUiInput(ui_input_prompt_poweron);
        }
        else if(phyState == phy_state_event_enter_standbymode)
        {
            DEBUG_LOG("enter standby mode");
        }
    }
    switch(phyState)
    {
        case phy_state_event_leave_sleepmode:
            setSystemReady(TRUE);
            break;
        case phy_state_event_leave_standbymode:
            break;
        case phy_state_event_enter_sleepmode:
            appPhyStateCancelTriggerSleepMode(); //for next time to trigger sleep mode timeout
            appPhyStateTriggerStandbyMode();
            BellUiAncControl(ui_input_bell_ui_anc_off);
            reportSleepStandbyStatus(TRUE);
            setSystemReady(FALSE);
            break;
        case phy_state_event_enter_standbymode:
            appPhyStateCancelTriggerSleepMode();
            appPhyStateCancelTriggerStandbyMode();
            BellUiAncControl(ui_input_bell_ui_anc_off);
            reportSleepStandbyStatus(TRUE);
            appUserStandbyModeRequest();
            break;
        default:
            break;
    }


}

void appPhySetPowerOffMode(uint8 mode)
{
    tym_sync_app_configuration_t *app_set = TymGet_AppSetting();
    app_set->auto_power_off_cmd = mode;
    if(mode == uifunc_poweroff_disable)
    {
       app_set->auto_power_off_timer = 0xff;
       MessageCancelAll(PhyStateGetTask(),PHY_STATE_INTERNAL_TIMEOUT_STANDBYMODE);
    }
    else if(mode == uifunc_poweroff_10m)
    {
        app_set->auto_power_off_timer = 10;
    }
    else if(mode == uifunc_poweroff_30m)
    {
        app_set->auto_power_off_timer = 30;
    }
    else if(mode == uifunc_poweroff_60m)
    {
        app_set->auto_power_off_timer = 60;
    }
    else if(mode == uifunc_poweroff_now)
    {
        tymSyncdata(sleepStandbyModeCmd, phy_state_event_enter_standbymode);
    }

}

uint8 appPhyGetPowerOffMode(void)
{
    tym_sync_app_configuration_t *app_set = TymGet_AppSetting();
    return app_set->auto_power_off_cmd;
}

void appPhyStatePrepareToEnterStandbyMode(void)
{
    //phyStateTaskData* phy_state = PhyStateGetTaskData();

    DEBUG_LOG("appPhyStatePrepareToEnterStandbyMode");
    if(getFactoryModeTopEnable() == TRUE)
    {
        DEBUG_LOG("ByPass Prox/anc register");
    }
    else
    {
        //appAncClientUnregister(&phy_state->task);
        //appProximityClientUnregister(&phy_state->task);
        appAncPowerOff();
        AncStateManager_Disable();
        appProximityPowerOff();
    }
    if((getFactoryModeEnable() == TRUE) || (getFactoryModeTopEnable() == TRUE))
        setFactoryModeStatus(factory_disable);
    updateTouchPadMode();//for standby mode
}


void appPhyStateLeaveDormant(void)
{
    DEBUG_LOG("appPhyStateLeaveDormant");
    if(getFactoryModeTopEnable() == TRUE)
    {
        DEBUG_LOG("ByPass Prox/anc register");
    }
    else
    {
        appProximityPowerOn();
        AncStateManager_Enable();
        appAncPowerOn();
    }
    appTouchPowerOn();
}
#endif


