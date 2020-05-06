/*!
\copyright  Copyright (c) 2017 - 2019 TYMPHANY Technologies International, Ltd.
            All Rights Reserved.
            TYMPHANY Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       iqs263.c
\brief      Support for touch sensor
*/
/* ------------------------ Include ------------------------ */
#include <panic.h>
#include <pio.h>
#include <stdlib.h>
#include <pio_common.h>
#include <pio_monitor.h>
#include "tym_touch_config.h"
#include "tym_i2c_control.h"
#include "iqs263.h"
#include "iqs263_init.h"
#include "tym_power_control.h"
#include "tym_pin_config.h"
#include "earbud_tym_util.h"
#include "earbud_tym_sensor.h"
#include "earbud_tym_factory.h"
#include "earbud_tym_cc_communication.h"
#include "earbud_tym_sync.h"
#include "logging.h"
#include "state_proxy.h"
#include "earbud_config.h"
/* ------------------------ Defines ------------------------ */

#define xprint(x)            DEBUG_LOG(x)
#define xprintf(x, ...)      DEBUG_LOG(x,  __VA_ARGS__)

#define IQS_CTRL_ADDR			(0x45)
#define IQS_CLOCK               (100)       /*100 KHZ*/
#define POLL_TIMEOUT            (250*1000) /* 250ms */
#define COLLECT_TAP_TIME        (300)       /* 300ms */
#define DEBOUND_TIME            (50)        /* 50ms */
#define EVENT_MODE              (0x40)
/* ------------------------ Types -------------------------- */
typedef enum iqs263CtrlAction
{
    tapAction,
    swipeLAction,
    swipeRAction,
}iqs263CtrlAction_t;
/* --------------- Local Function Prototypes --------------- */
bool _waitRdyLow(void);
bool _waitRdyHigh(void);
bool _iqs263Enable(touchConfig *config);
void _iqs263Disable(touchConfig *config);
void _updateIQSEvent(void);
bool _i2c_iqs263_burst_write(uint8 *write_data, uint16 write_size);
bool _i2c_iqs263_burst_read(uint8 read_addr, uint8 *read_data, uint16 read_size);
static void iqs263InterruptHandler(Task task, MessageId id, Message msg);
void _processInterruptPin(touchConfig *tconfig,const MessagePioChanged *mpc);
void _processTAPEvent(void);
static void _iqsProcessMessageHandler ( Task pTask, MessageId pId, Message pMessage );
static const TaskData iqsProcessTask = { _iqsProcessMessageHandler };
void holdEndOperation(void);
void holdStartOperation(void);
void swipeLeftAction(void);
void swipeRightAction(void);
void cancelTouchPadTimer(void);
void setTouchPadTimer(void);
void runSwipeFunction(uint8 swipe);
/* --------------- Static Global Variables ----------------- */
struct __touch_config touch_config = {
    .rdy_interrupt  = IQS_RDY_PIN,
    .hold_interrupt = IQS_HOLD_PIN,
    .chan1threshold = 0,
    .eventSwipe     = 0,
    .tapCnt         = 0,
    .eventTap       = FALSE,
    .eventHold      = FALSE,
    .debound        = FALSE,
    .holdPio        = FALSE,
    .readyPio       = FALSE,
    .init           = FALSE,
    .hold2sTrigger  = FALSE,
    .hold5sTrigger  = FALSE, 
};

tymTouchTaskData app_tymtouch;
/* ------------------ Local Function ----------------------- */
bool _waitRdyLow(void)
{
    bool ret = FALSE;
    uint32 clock = VmGetTimerTime();
    uint32 count = 0;    
    uint32 curr_clock;

    while(1)
    {   
        if(PioCommonGetPio(IQS_RDY_PIN) == FALSE)
        {
            ret = TRUE;    
            break;
        }
        else
        {
            count++;                    
            if(count > 200)
            {
                curr_clock = VmGetTimerTime();
                if(checktimeout(clock,curr_clock,POLL_TIMEOUT))
                {
                    DEBUG_LOG("Check L VmGet %d,clock %d",curr_clock,clock);
                    break;
                }    
            }
        }               
    }
    return ret;  
}

bool _waitRdyHigh(void)
{
    bool ret = FALSE;
    uint32 clock = VmGetTimerTime(); 
    uint32 count = 0;    
    uint32 curr_clock;

    while(1)
    {   
        if(PioCommonGetPio(IQS_RDY_PIN) == TRUE)
        {
            ret = TRUE;    
            break;
        }
        else
        {
            count++;
            if(count > 200)
            {
                curr_clock = VmGetTimerTime();
                if(checktimeout(clock,curr_clock,POLL_TIMEOUT))
                {
                    xprintf("Check H VmGet %d,clock %d",curr_clock,clock);
                    break;
                }    
            }   
        }                          
    }
    return ret;  
}

/***************************************************************************
 * Description: Writes data to the iqs263
   Input: write_data - the data to be written to the iqs263
          write_size - This is number of write bytes to the iqs263
 **************************************************************************/
bool _i2c_iqs263_burst_write(uint8 *write_data, uint16 write_size)
{    
    if(!_waitRdyLow())
        return FALSE;
    if(!tym_i2c_write(IQS_CTRL_ADDR,write_data,write_size))
    {
        xprint("error iqs263 w");
        return FALSE; /* error */ 
    }    

    if(!_waitRdyHigh())
        return FALSE;
	
    return TRUE;
}



/***************************************************************************
 * Description: Reads data from the iqs263        
   Input: read_addr - 8-bit register address
          read_data - the data to be read from the iqs263
          read_size - This is number of read bytes from the iqs263
 **************************************************************************/
bool _i2c_iqs263_burst_read(uint8 read_addr, uint8 *read_data, uint16 read_size)
{
    if(!_waitRdyLow())
        return FALSE;
 
    if(!tym_i2c_read(IQS_CTRL_ADDR, read_addr, read_data, read_size))
    {
        xprint("error iqs263 read");
        return FALSE; /* error */ 
    } 
	 
    if(!_waitRdyHigh())
        return FALSE;
	
    return TRUE;
}

/*! \brief enable iqs263 */
bool _iqs263Enable(touchConfig *config)
{
	uint8 data_buffer[16];
	    
    if(config->init == TRUE)
        return TRUE;
    /* Setup Interrupt as input with weak pull up */
    setupPIOInputAndInterrupt(config->rdy_interrupt);
    setupPIOInputAndInterrupt(config->hold_interrupt);    
            
    if (tym_i2cdevice_init(IQS_CTRL_ADDR,IQS_CLOCK,i2c_device_touch_id) == FALSE)
    {
        xprint("iqs263 i2c device init failed\n");
        return FALSE;
    }       
    /* Switch the IQS263 into projection mode - if necessary */
    data_buffer[0] = SYS_FLAGS;
    data_buffer[1] = (0x80 | SYSTEM_FLAGS_VAL);
    //xprintf(("projection mode\r\n"));
    if(!_i2c_iqs263_burst_write(&data_buffer[0], 2))
        return FALSE;    
   /*  Set active channels */
    data_buffer[0] = ACTIVE_CHANNELS;
    data_buffer[1] = ACTIVE_CHS;
    //xprintf(("active channels\r\n"));
    if(!_i2c_iqs263_burst_write(&data_buffer[0], 2))
        return FALSE;
    
    /* Set the BASE value for each channel */
    data_buffer[0] = MULTIPLIERS;
    data_buffer[1] = MULTIPLIERS_CH0;
    data_buffer[2] = MULTIPLIERS_CH1;
    data_buffer[3] = MULTIPLIERS_CH2;
    data_buffer[4] = MULTIPLIERS_CH3;
    data_buffer[5] = BASE_VAL1;
    data_buffer[6] = BASE_VAL2;
    //xprintf(("BASE value for each channel\r\n"));
    if(!_i2c_iqs263_burst_write(&data_buffer[0], 7))
        return FALSE;
    /* Setup Compensation (PCC) */
    data_buffer[0] = COMPENSATION;
    data_buffer[1] = COMPENSATION_CH0;
    data_buffer[2] = COMPENSATION_CH1;
    data_buffer[3] = COMPENSATION_CH2;
    data_buffer[4] = COMPENSATION_CH3;
    //xprintf(("Setup Compensation\r\n"));
    if(!_i2c_iqs263_burst_write(&data_buffer[0], 5))
        return FALSE;
    
    /* Setup prox settings */
    data_buffer[0] = PROX_SETTINGS;
    data_buffer[1] = PROXSETTINGS0_VAL;
    data_buffer[2] = PROXSETTINGS1_VAL;
    data_buffer[3] = PROXSETTINGS2_VAL;
    data_buffer[4] = PROXSETTINGS3_VAL;
    data_buffer[5] = EVENT_MASK_VAL;
    //xprintf(("Setup event set 0x%x\r\n",EVENT_MASK_VAL));
    if(!_i2c_iqs263_burst_write(&data_buffer[0], 6))
        return FALSE;
    
    /* Setup touch and prox thresholds for each channel */
    data_buffer[0] = THRESHOLDS;
    data_buffer[1] = PROX_THRESHOLD;
    data_buffer[2] = TOUCH_THRESHOLD_CH1;
    data_buffer[3] = TOUCH_THRESHOLD_CH2; 
    data_buffer[4] = TOUCH_THRESHOLD_CH3;
    data_buffer[5] = MOVEMENT_THRESHOLD;
    data_buffer[6] = RESEED_BLOCK;
    data_buffer[7] = HALT_TIME;
    data_buffer[8] = I2C_TIMEOUT;
    //xprintf(("Setup touch and prox thresholds\r\n"));
    if(!_i2c_iqs263_burst_write(&data_buffer[0], 9))
        return FALSE;
    
    /* Set timings and the ATI targets (Target Counts) on the IQS263 */
    data_buffer[0] = TIMINGS_AND_TARGETS;
    data_buffer[1] = LOW_POWER;
    data_buffer[2] = ATI_TARGET_TOUCH;
    data_buffer[3] = ATI_TARGET_PROX;
    //xprintf(("Setup ATI targets\r\n"));
    if(!_i2c_iqs263_burst_write(&data_buffer[0], 4))
        return FALSE;
    
    /* Set gesture timers on IQS263 */
    data_buffer[0] = GESTURE_TIMERS;
    data_buffer[1] = TAP_TIMER;
    data_buffer[2] = FLICK_TIMER;
    data_buffer[3] = FLICK_THRESHOLD;
    //xprintf(("Setup gesture timers\r\n"));
    if(!_i2c_iqs263_burst_write(&data_buffer[0], 4))
        return FALSE;
    
    /* Redo ATI */
    data_buffer[0] = PROX_SETTINGS;
    data_buffer[1] = (0x10 | PROXSETTINGS0_VAL);
    //xprintf(("Redo ATI\r\n"));
    if(!_i2c_iqs263_burst_write(&data_buffer[0], 2))
        return FALSE;
    
    /* Wait untill the ATI algorithm is done */
    do
    {
        _i2c_iqs263_burst_read(SYS_FLAGS, &data_buffer[0], 1);
    }
    while(data_buffer[0] & 0x04);

/* ===============================================================
 *  before event mode , get prox chan1 threshold for factory
 */	
    _i2c_iqs263_burst_read(THRESHOLDS, &data_buffer[0], 1);
    config->chan1threshold = data_buffer[0];
/*
 * ==============================================================
 */ 	

    /* Setup prox settings */
    data_buffer[0] = PROX_SETTINGS;
    data_buffer[1] = PROXSETTINGS0_VAL;
    data_buffer[2] = PROXSETTINGS1_VAL | EVENT_MODE;;
    if(!_i2c_iqs263_burst_write(&data_buffer[0], 3))
        return FALSE;   

    if(PioCommonGetPio(IQS_RDY_PIN) == FALSE)
    {
        config->readyPio = FALSE;
    }
    else
    {
        config->readyPio = TRUE;
    }
     
    if(PioCommonGetPio(IQS_HOLD_PIN) == FALSE)
    {
        holdStartOperation();
        config->holdPio = FALSE;
    }
    else
    {
        holdEndOperation();
        config->holdPio = TRUE;
    }                
    config->init = TRUE;
    
    return TRUE;    
}

/*! \brief disable iqs263 */
void _iqs263Disable(touchConfig *config)
{
    config->init = FALSE; 
    /* Disable interrupt and set weak pull down */
    setupPIODisableInterrupt(config->hold_interrupt);    
    setupPIODisableInterrupt(config->rdy_interrupt);          
}

/*! \brief process interrupt pin to trigger event */
void _processInterruptPin(touchConfig *tconfig,const MessagePioChanged *mpc)
{
    touchConfig *config = appConfigTouch();    
    uint32 state = ((uint32)mpc->state16to31 << 16) + mpc->state;
    if(config->init == FALSE)
        return;
    //hold pin    
    if (mpc->bank == PIO2BANK(tconfig->hold_interrupt))
    {
        if (~state & PIO2MASK(tconfig->hold_interrupt))
        {
            if(tconfig->holdPio)
                holdStartOperation();
            tconfig->holdPio = FALSE;
        }
        else
        {                       
            holdEndOperation();                   
            tconfig->holdPio = TRUE;
        }                    
    }      
    //ready pin 
    if (mpc->bank == PIO2BANK(tconfig->rdy_interrupt))
    {
        if (~state & PIO2MASK(tconfig->rdy_interrupt))
        {                   
            if(tconfig->readyPio)
                MessageSend ( (TaskData *)&iqsProcessTask, iqs263_event, 0);
            tconfig->readyPio = FALSE;
        }
        else
        {
            tconfig->readyPio= TRUE;
        }    
    }
         
}
/*! \brief Handle the ready interrupt */
static void iqs263InterruptHandler(Task task, MessageId id, Message msg)
{
    tymTouchTaskData *touch = (tymTouchTaskData *) task;
    touchConfig *tconfig = touch->config;
    switch(id)
    {
        case MESSAGE_PIO_CHANGED:
        {
            const MessagePioChanged *mpc = (const MessagePioChanged *)msg;   
            _processInterruptPin(tconfig,mpc);
            break;
        }
        default:
            break;
        }         
    }


/*! \brief get iqs263 event tap/swipe left/swipe right */
void _updateIQSEvent(void)
{
    uint8 iqs_sys_bytes[2];
    uint8 iqs_events;
    touchConfig *tconfig = appConfigTouch();

    _i2c_iqs263_burst_read(SYS_FLAGS,&iqs_sys_bytes[0],2);
    iqs_events = iqs_sys_bytes[1];
    if(getFactoryModeEnable() == TRUE)
    {    
       updateFactoryTouchEvent(iqs_events);  
    }
    else
    {    
    /******************************* FLICK (RIGHT) ****************************/
        if(iqs_events & 0x80)
        {
            xprint("Flick Right Event");
            swipeRightAction();
        }
        /******************************* FLICK (LEFT) *****************************/
        else if(iqs_events & 0x40)
        {
            xprint("Flick Left Event");
            swipeLeftAction();  
        }
        /********************************* TAP ************************************/
        else if(iqs_events & 0x20)
        {					
            if(tconfig->eventHold == FALSE)
            {   
                DEBUG_LOG("tap no hold , error return");
                return;
            }
            if(tconfig->eventSwipe == 0)
            {
                tconfig->eventTap = TRUE;
            }            
        }
    } 
} 

/*! \brief prcess iqs263 tap event to tigger  */
void _processTAPEvent(void)
{
    touchConfig *tconfig = appConfigTouch();
    tymTouchTaskData *tymtouch = TymTouchGetTaskData();    
    uint8 touchPadMode = tymGetTouchPadMode();
    uint8 tapCnt = tconfig->tapCnt;
    tconfig->tapCnt = 0;
    if(touchPadMode != normalPad)
    {
        if((touchPadMode >= sleepPad) && (tapCnt > 1))
        {
            DEBUG_LOG("tap > 1, standbyPad");
            TaskList_MessageSendId(tymtouch->clients, TOUCH_MESSAGE_TAPx2);
        }    
        DEBUG_LOG("tap != normalPad, bye");
        return;    
    }        
    if(tapCnt == 1)
    {
        TaskList_MessageSendId(tymtouch->clients, TOUCH_MESSAGE_TAPx1);
    }
    else if(tapCnt == 2)
    {
        TaskList_MessageSendId(tymtouch->clients, TOUCH_MESSAGE_TAPx2);
    }
    else //cnt > 3
    {
        TaskList_MessageSendId(tymtouch->clients, TOUCH_MESSAGE_TAPx3);        
    }

}


/*! \brief Process iqs event */
static void _iqsProcessMessageHandler ( Task pTask, MessageId pId, Message pMessage )
{
    UNUSED(pTask);
    UNUSED(pMessage);
    bool noCasePairing = FALSE;
    uint8 touchPadMode = tymGetTouchPadMode();
    tymTouchTaskData *tymtouch = TymTouchGetTaskData(); 
    touchConfig *tconfig = appConfigTouch();
        
    if(pId == iqs263_event)
    {
        _updateIQSEvent();  
    }
    else if(pId == iqs263_tap)        
    {
        _processTAPEvent();
    }
    else if(pId == iqs263_debound)
    {
        tconfig->debound = FALSE;
    }  
    else if(pId == iqs263_hold2s)
    {
        if(touchPadMode == normalPad)
        {    
            DEBUG_LOG("hold 2s event");
            tconfig->hold2sTrigger = TRUE;
            TaskList_MessageSendId(tymtouch->clients, TOUCH_MESSAGE_HOLD2S);
        }    
    }
    else if(pId == iqs263_hold5s)
    {
        /*filter current touchPad mode*/
        if((touchPadMode == normalPad) && (StateProxy_IsPeerInEar() == TRUE) && (StateProxy_IsInEar() == TRUE))
        {    
            noCasePairing = TRUE;
            tconfig->hold5sTrigger = TRUE;
        }    
        if((touchPadMode == btPairingPad) || (noCasePairing  == TRUE))
        {    
            DEBUG_LOG("hold 5s event");
            TaskList_MessageSendId(tymtouch->clients, TOUCH_MESSAGE_HOLD5S);
        }
    }
    else if(pId == iqs263_hold10s)
    {
        if(touchPadMode == restoreDefaultPad)
        {    
            DEBUG_LOG("hold 10s event");
            TaskList_MessageSendId(tymtouch->clients, TOUCH_MESSAGE_HOLD10S);            
        }
    }           
}

/*! \brief hold End Operation */
void holdEndOperation(void)
{
    touchConfig *tconfig = appConfigTouch();
    uint8 swipeAction = 0;
    if(tconfig->eventHold == FALSE)
    {
        DEBUG_LOG("holdend error, eventhold %d",tconfig->eventHold);
        return;
    }    
    tconfig->eventHold = FALSE;
    cancelTouchPadTimer();
    if(tconfig->hold2sTrigger == TRUE)
    {
		tconfig->hold2sTrigger = FALSE; 
		//TaskList_MessageSendId(tymtouch->clients, TOUCH_MESSAGE_HOLD2SEND);
		DEBUG_LOG("&&&&& send hold 2s end &&&&&&&&");
		MessageSend(PhyStateGetTask(), TOUCH_MESSAGE_HOLD2SEND, NULL);
    }    
    if(tconfig->hold5sTrigger == TRUE)
    {
		tconfig->hold5sTrigger = FALSE; 
		//TaskList_MessageSendId(tymtouch->clients, TOUCH_MESSAGE_HOLD5SEND);
        MessageSend(PhyStateGetTask(), TOUCH_MESSAGE_HOLD5SEND, NULL);
    }  
    if((tconfig->eventTap == FALSE) && (tconfig->eventSwipe == 0))
    {
        tconfig->tapCnt = 0;
        tconfig->eventHold = FALSE;
        tconfig->eventSwipe = 0;
        return;
    }    
    if(tconfig->eventTap)
        tconfig->tapCnt += 1;
    swipeAction = tconfig->eventSwipe;
    if(swipeAction)
    {
       runSwipeFunction(swipeAction);
       tconfig->eventSwipe = 0;
       tconfig->tapCnt = 0;
    }
    else if(tconfig->tapCnt)
    {
        if(tconfig->tapCnt >= 3)
        {
            _processTAPEvent();
        }
        else
        {
            //create tap timer
            MessageSendLater( (TaskData *)&iqsProcessTask, iqs263_tap, 0, COLLECT_TAP_TIME);
        }        
    }
    tconfig->debound = TRUE; 
    MessageSendLater( (TaskData *)&iqsProcessTask, iqs263_debound, 0, DEBOUND_TIME);            
}

/*! \brief hold Start Operation */
void holdStartOperation(void)
{
    touchConfig *tconfig = appConfigTouch();    
    if((tconfig->eventHold == TRUE) || (tconfig->debound == TRUE))
    {
        DEBUG_LOG("holdstart error, eventHold %d,debound %d",tconfig->eventHold,tconfig->debound);
        return;
    }        
    tconfig->eventHold = TRUE;
    /*hold start, start tap collect again*/
    MessageCancelAll( (TaskData *)&iqsProcessTask, iqs263_tap);
    setTouchPadTimer();
    tconfig->eventTap = FALSE;
    tconfig->eventSwipe = 0;
}

/*! \brief Swipe Left Action */
void swipeLeftAction(void)
{
    touchConfig *tconfig = appConfigTouch();    
    if(tconfig->eventHold == FALSE)
    {
        DEBUG_LOG("swipe Left error,return");
        return;
    }    
    tconfig->eventSwipe = swipeLAction;
    tconfig->eventTap = FALSE;
}

/*! \brief Swipe Right Action */
void swipeRightAction(void)
{
    touchConfig *tconfig = appConfigTouch();    
    if(tconfig->eventHold == FALSE)
    {
        DEBUG_LOG("swipe Right error,return");    
        return;
    }
    tconfig->eventSwipe = swipeRAction;
    tconfig->eventTap = FALSE;    
}

/*! \brief cancel Touch pad timer */
void cancelTouchPadTimer(void)
{
	//DEBUG_LOG("cancel hold timer");
    MessageCancelAll ((TaskData *) &iqsProcessTask, iqs263_hold2s);
    MessageCancelAll ((TaskData *) &iqsProcessTask, iqs263_hold5s);
    MessageCancelAll ((TaskData *) &iqsProcessTask, iqs263_hold10s);
}

/*! \brief Setup Touch need follow touch pad mode */
void setTouchPadTimer(void)
{
    uint8 touchPadMode;
    touchPadMode = tymGetTouchPadMode();
    DEBUG_LOG("TouchPad Mode %d",touchPadMode);

    if((StateProxy_IsInCase() == TRUE) && (touchPadMode != restoreDefaultPad) )
    {
        DEBUG_LOG("hold 5s");  
        MessageSendLater ( (TaskData *)&iqsProcessTask, iqs263_hold5s, 0, D_SEC(4.5));        
    }    
    else if(touchPadMode == normalPad)
    {
        MessageSendLater ( (TaskData *)&iqsProcessTask, iqs263_hold2s, 0, D_SEC(2));
        if((StateProxy_IsPeerInEar() == TRUE) && (StateProxy_IsInEar() == TRUE))//link wear,local wear
    	{
            DEBUG_LOG("hold 5s");
            MessageSendLater ( (TaskData *)&iqsProcessTask, iqs263_hold5s, 0, D_SEC(4.5));
    	}        
    }
    else if(touchPadMode == restoreDefaultPad)                
    {
        DEBUG_LOG("hold 10s");
        MessageSendLater ( (TaskData *)&iqsProcessTask, iqs263_hold10s, 0, D_SEC(9.5));
    }                
}

/*! \brief according swipe left/right call mapping function */
void runSwipeFunction(uint8 swipe)
{
    uint8 touchPadMode;
    tymTouchTaskData *tymtouch = TymTouchGetTaskData();
    touchPadMode = tymGetTouchPadMode();
    if(touchPadMode != normalPad)
    {
        DEBUG_LOG("touchPadMode not normalPad");
        return;    
    } 
    if(swipe == swipeLAction)
    {
    	DEBUG_LOG("call swipe left");
        if(appConfigIsLeft()) /*left earbud need invert*/
        {
            TaskList_MessageSendId(tymtouch->clients, TOUCH_MESSAGE_SWIPER);            
        }
        else
        {
            TaskList_MessageSendId(tymtouch->clients, TOUCH_MESSAGE_SWIPEL);
        }        
    }
    else if(swipe == swipeRAction)
    {
    	DEBUG_LOG("call swipe right");
        if(appConfigIsLeft()) /*left earbud need invert*/
        {
            TaskList_MessageSendId(tymtouch->clients, TOUCH_MESSAGE_SWIPEL);            
        }
        else
        {
            TaskList_MessageSendId(tymtouch->clients, TOUCH_MESSAGE_SWIPER);
        }   	
    }  
}
/* ----------------- Global Function ----------------------- */
/*! \brief Register touch to the task */
bool appTouchClientRegister(Task task)
{
    tymTouchTaskData *tymtouch = TymTouchGetTaskData();

    if (NULL == tymtouch->clients)
    {      
        touchConfig *config = appConfigTouch();
        tymtouch->clients = TaskList_Create();
        tymtouch->config = config;
        tym_power_on(i2c_device_touch_id);  
        if(_iqs263Enable(config))
        {
            /* Register for interrupt events */
            tymtouch->task.handler = iqs263InterruptHandler;
            PioMonitorRegisterTask(&tymtouch->task, config->rdy_interrupt);
            PioMonitorRegisterTask(&tymtouch->task, config->hold_interrupt);
        }    
    }

    return TaskList_AddTask(tymtouch->clients, task);
}

/*! \brief Unregister touch to the task */
void appTouchClientUnregister(Task task)
{
    tymTouchTaskData *tymtouch = TymTouchGetTaskData();
    if(tymtouch->clients != NULL)
    {
        TaskList_RemoveTask(tymtouch->clients, task);
        if (0 == TaskList_Size(tymtouch->clients))
        {
            xprint("appTouchClientUnregister");
            TaskList_Destroy(tymtouch->clients);
            tymtouch->clients = NULL;            
            _iqs263Disable(tymtouch->config);
            cancelTouchPadTimer();
            tym_i2cdevice_deinit(i2c_device_touch_id);            
            tym_power_off(i2c_device_touch_id);
            /* Unregister for interrupt events */
            PioMonitorUnregisterTask(&tymtouch->task, tymtouch->config->rdy_interrupt);
            /* Unregister for interrupt events */
            PioMonitorUnregisterTask(&tymtouch->task, tymtouch->config->hold_interrupt);
        }
    }
}

void appTouchPowerOn(void)
{
    touchConfig *config = appConfigTouch();    
    tym_power_on(i2c_device_touch_id); 
    _iqs263Enable(config);   
}

void appTouchPowerOff(void)
{
    touchConfig *config = appConfigTouch();       
    _iqs263Disable(config);
    cancelTouchPadTimer();    
    tym_power_off(i2c_device_touch_id);    
}
/*! \brief provide information hold level for factory test */
bool getFactoryTouchHoldLevel(void)
{
    return PioCommonGetPio(IQS_HOLD_PIN);
}

/*! \brief provide information Touch Ch1 threshold value for factory test */
uint8 getFactoryTouchCh1threshold(void)
{
    touchConfig *config = appConfigTouch();
	return config->chan1threshold;
}

/*! \brief provide Touch Pad Mode update*/
void updateTouchPadMode(void)
{
    bool inCase = StateProxy_IsInCase();
    bool inEar = StateProxy_IsInEar();
    bool peerInCase = StateProxy_IsPeerInCase();
    bool sleepMode = StateProxy_IsSleepMode();
    bool standbyMode = StateProxy_IsStandbyMode();
    tymTouchTaskData *tymtouch = TymTouchGetTaskData();
    
    if((inCase == TRUE) &&(peerInCase == TRUE))//two earbud in case
    {
        if(tymGetBTStatus() == btPairing)
        {
            tymtouch->touchPadMode = restoreDefaultPad;
            //restoreDefault pad
        }    
        else
        {
            //pairing pad
            tymtouch->touchPadMode = btPairingPad;
        }    
    }
    else if(inEar == TRUE) //normal
    {
        //normal pad
        tymtouch->touchPadMode = normalPad;
    }        
    else
    {
        //ignore pad
        tymtouch->touchPadMode = ignorePad;
    }   
     
    if(sleepMode)
    {
        tymtouch->touchPadMode = sleepPad;        
    }    
    else if(standbyMode)
    {
        tymtouch->touchPadMode = standbyPad;            
    }    
    DEBUG_LOG("InCase %d,InEar %d,PeerInCase %d,PeerInEar %d",StateProxy_IsInCase(),StateProxy_IsInEar(),StateProxy_IsPeerInCase(),StateProxy_IsPeerInEar());
    DEBUG_LOG("Sleep %d,Standby %d",StateProxy_IsSleepMode(),StateProxy_IsStandbyMode());
    DEBUG_LOG("touchPadMode %d",tymtouch->touchPadMode);    
     
}

/*! \brief get Touch Pad Mode for different UI*/
uint8 tymGetTouchPadMode(void)
{
    tymTouchTaskData *tymtouch = TymTouchGetTaskData();
    return tymtouch->touchPadMode;
}
