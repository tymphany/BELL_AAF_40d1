/*!
\copyright  Copyright (c) 2017 - 2019 TYMPHANY Technologies International, Ltd.
            All Rights Reserved.
            TYMPHANY Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       ltr2678.c
\brief      Support for the ANC stanc3
*/
/* ------------------------ Include ------------------------ */
#ifdef INCLUDE_PROXIMITY
#ifdef HAVE_LTR2678

#include <panic.h>
#include <pio.h>
#include <stdlib.h>
#include <ps.h>
#include <pio_monitor.h>
#include <logging.h>
#include "proximity_config.h"
#include "tym_i2c_control.h"
#include "ltr2678.h"
#include "tym_pin_config.h"
#include "tym_power_control.h"
#include "earbud_tym_util.h"
#include "earbud_tym_sensor.h"
#include "earbud_tym_psid.h"

/* ------------------------ Defines ------------------------ */

#define xprint(x)            DEBUG_LOG(x)
#define xprintf(x, ...)      DEBUG_LOG(x,  __VA_ARGS__)



#define LTR2678_CTRL_ADDR	 (0x23)
#define LTR2678_CLOCK        (100) /*100 KHZ*/
/* ------------------------ Types -------------------------- */

/* --------------- Local Function Prototypes --------------- */
static void ltr2678InterruptHandler(Task task, MessageId id, Message msg);
void detectWearable(proximityTaskData *proximity);
bool _i2c_ltr2678_write(uint8 *write_data, uint16 write_size);
bool _i2c_ltr2678_read(uint8 read_addr, uint8 *read_data, uint16 read_size);
bool _ltr2678_ps_enable(uint8 enable);
bool _ltr2678_ps_set_threshold(uint16 high, uint16 low);
bool _ltr2678_ps_set_offset(uint16 offset);
bool _ltr2678Enable(proximityConfig *config);
void _ltr2678Disable(proximityConfig *config);
uint16 _ltr2678_ps_read(void);
/* --------------- Static Global Variables ----------------- */
struct __proximity_config proximity_config = {
    .threshold_low = PS_THRES_LOW,
    .threshold_high = PS_THRES_UP,
    .offset = 0,    
    .interrupt = IR_PAUSE_PIN,
    .init = FALSE,
};

/*!< Task information for proximity sensor */
proximityTaskData app_proximity;
/* ------------------ Local Function ----------------------- */
/*! \brief ltr2678 i2c write */
bool _i2c_ltr2678_write(uint8 *write_data, uint16 write_size)
{ 
    if(!tym_i2c_write(LTR2678_CTRL_ADDR,write_data,write_size))
    {
        xprint("error ltr i2c w");
        return FALSE; /* error */ 
    }    
	return TRUE;    
}

/*! \brief ltr2678 i2c read */
bool _i2c_ltr2678_read(uint8 read_addr, uint8 *read_data, uint16 read_size)
{
    if(!tym_i2c_read(LTR2678_CTRL_ADDR, read_addr, read_data, read_size))
    {
        xprint("error ltr i2c r");
        return FALSE; /* error */ 
    }
    return TRUE;     
}
/*! \brief ltr2678 detect wearable */
void detectWearable(proximityTaskData *proximity)
{
    uint8 ltr_data_buffer[2];
    uint8 ps_status;
    uint16 ps_val;
    if(!_i2c_ltr2678_read(LTR2678_PS_STATUS,ltr_data_buffer,1))
        return;
    ps_val = _ltr2678_ps_read();
    ps_status = ltr_data_buffer[0];
    if(ps_status & FTN_BIT)
    {
        xprint("Wear");
        proximity->state->proximity = proximity_state_in_proximity;
        TaskList_MessageSendId(proximity->clients, PROXIMITY_MESSAGE_IN_PROXIMITY);
    }
    else if(ps_status & NTF_BIT)    
    {
        xprint("Not Wear");
        proximity->state->proximity = proximity_state_not_in_proximity;
        TaskList_MessageSendId(proximity->clients, PROXIMITY_MESSAGE_NOT_IN_PROXIMITY);
    }    
    else if(ps_val > proximity->config->threshold_high)
    {
        xprint("Wear"); 
        proximity->state->proximity = proximity_state_in_proximity;
        TaskList_MessageSendId(proximity->clients, PROXIMITY_MESSAGE_IN_PROXIMITY);
    }    
    else
    {   
        xprint("Not Wear");
        proximity->state->proximity = proximity_state_not_in_proximity;
        TaskList_MessageSendId(proximity->clients, PROXIMITY_MESSAGE_NOT_IN_PROXIMITY);
    }
}

/*! \brief ltr2678 prox enable */
bool _ltr2678_ps_enable(uint8 enable)
{
    uint8 regdata = 0;
    uint8 ltr_data_buffer[4];	
  
    if(!_i2c_ltr2678_read(LTR2678_PS_CTRL,ltr_data_buffer,1))
        return FALSE;
    regdata = ltr_data_buffer[0];
    if (enable != 0) {
        if (PS_USE_OFFSET)
	    regdata = ((regdata | 0x18) & 0x3E) | 0x02;
	    else
        regdata = ((regdata | 0x10) & 0x36) | 0x02;;
    }
    else {
        regdata &= 0xfd;
    }

    if (PS_16BIT == 1)
	    regdata |= 0x20;
    else
	    regdata &= 0xdf;
    //enable FTN/NTF
    regdata |= 0x04;
    ltr_data_buffer[0] = LTR2678_PS_CTRL;
    ltr_data_buffer[1] = regdata;
    if(!_i2c_ltr2678_write(ltr_data_buffer,2))
        return FALSE;     
        
    return TRUE;
}

/*! \brief ltr2678 set high/low threshold */
bool _ltr2678_ps_set_threshold(uint16 high, uint16 low)
{
    uint8 ltr_data_buffer[4];
    
    ltr_data_buffer[0] = LTR2678_PS_THRES_UP_0;  
    ltr_data_buffer[1] =  high & 0x00FF;         
    if(!_i2c_ltr2678_write(ltr_data_buffer,2))
        return FALSE; 
    ltr_data_buffer[0] = LTR2678_PS_THRES_UP_1;  
    ltr_data_buffer[1] =  (high >> 8) & 0x00FF;         
    if(!_i2c_ltr2678_write(ltr_data_buffer,2))
        return FALSE;  
    
    ltr_data_buffer[0] = LTR2678_PS_THRES_LOW_0;  
    ltr_data_buffer[1] =  low & 0x00FF;        
    if(!_i2c_ltr2678_write(ltr_data_buffer,2))
        return FALSE; 
    ltr_data_buffer[0] = LTR2678_PS_THRES_LOW_1;  
    ltr_data_buffer[1] =  (low >> 8) & 0x00FF;
    if(!_i2c_ltr2678_write(ltr_data_buffer,2))
        return FALSE;                    
        
    return TRUE;
}

/*! \brief ltr2678 set cut offset */
bool _ltr2678_ps_set_offset(uint16 offset)
{
    uint8 ltr_data_buffer[4];

    ltr_data_buffer[0] = LTR2678_PS_CAN_0;  
    ltr_data_buffer[1] =  offset & 0x00FF;         
    if(!_i2c_ltr2678_write(ltr_data_buffer,2))
        return FALSE;    

    ltr_data_buffer[0] = LTR2678_PS_CAN_1;  
    ltr_data_buffer[1] =  (offset >> 8) & 0x00FF;         
    if(!_i2c_ltr2678_write(ltr_data_buffer,2))
        return FALSE;             

    return TRUE;
}


/*! \brief Handle the proximity interrupt */
static void ltr2678InterruptHandler(Task task, MessageId id, Message msg)
{
    proximityTaskData *proximity = (proximityTaskData *) task;
    switch(id)
    {
        case MESSAGE_PIO_CHANGED:
        {
            const MessagePioChanged *mpc = (const MessagePioChanged *)msg;
            uint32 state = ((uint32)mpc->state16to31 << 16) + mpc->state;
            const proximityConfig *config = proximity->config;   
            if (mpc->bank == PIO2BANK(config->interrupt))
            {
                if (~state & PIO2MASK(config->interrupt))
                {
                    detectWearable(proximity);
                }
            }
        }
            break;
        default:
            break;
    }            
}

/*! \brief enable ltr2678 by config */
bool _ltr2678Enable(proximityConfig *config)
{
    proximityTaskData *prox = ProximityGetTaskData();
    uint8 ltr_data_buffer[4];
    uint16 thr[3];
    int irps,custom_config_len = 0;
    uint8 led_config = 0x76;
    uint8 time_config = 0x03;
    uint16 custom_config[1];
    
    if(config->init == TRUE)
        return TRUE;      
        
    /* Setup Interrupt as input with weak pull up */

    if (tym_i2cdevice_init(LTR2678_CTRL_ADDR, LTR2678_CLOCK, i2c_device_prox_id) == FALSE)
    {
        xprint("ltr572 i2c device init failed");
        return FALSE;
    }         
    custom_config_len = PsRetrieve(PSID_IRCONFIG,custom_config,1);
    if(custom_config_len != 0)
    {
        led_config = (custom_config[0] >> 8);
        time_config =  (custom_config[0] & 0xff);
    }
    
    /* LTR2678_LED_DRIVE */
    ltr_data_buffer[0] = LTR2678_LED_DRIVE;
    ltr_data_buffer[1] = 0x04;
    if(!_i2c_ltr2678_write(ltr_data_buffer,2))
        return FALSE;
    
    /* LTR2678_MAIN_CTRL */
    ltr_data_buffer[0] = LTR2678_MAIN_CTRL;
    ltr_data_buffer[1] = 0x18;
    if(!_i2c_ltr2678_write(ltr_data_buffer,2))
        return FALSE;   
    /* LTR2678_IR_AMBIENT*/
    ltr_data_buffer[0] = LTR2678_IR_AMBIENT;
    ltr_data_buffer[1] = 0x00;
    if(!_i2c_ltr2678_write(ltr_data_buffer,2))
        return FALSE;
    /* LTR2678_DSS_CTRL*/
    ltr_data_buffer[0] = LTR2678_DSS_CTRL;
    ltr_data_buffer[1] = 0x10;
    if(!_i2c_ltr2678_write(ltr_data_buffer,2))
        return FALSE;       
   
    /* LTR2678_PS_LED*/
    ltr_data_buffer[0] = LTR2678_PS_LED;
    if(custom_config_len == 0)   
        ltr_data_buffer[1] = 0x76;// 16us & 7mA
    else
        ltr_data_buffer[1] = led_config;// 16us & 7mA    
    if(!_i2c_ltr2678_write(ltr_data_buffer,2))
        return FALSE; 
  
    /* LTR2678_PS_PULSES*/
    ltr_data_buffer[0] = LTR2678_PS_PULSES;
    ltr_data_buffer[1] = 0x85;// 6 pulses & 4n averaging  
    if(!_i2c_ltr2678_write(ltr_data_buffer,2))
        return FALSE; 
    
    /* LTR2678_PS_MEAS_RATE*/
    ltr_data_buffer[0] = LTR2678_PS_MEAS_RATE;
    if(custom_config_len == 0)       
        ltr_data_buffer[1] = 0x03;// 50ms time
    else
        ltr_data_buffer[1] = time_config;    
    if(!_i2c_ltr2678_write(ltr_data_buffer,2))
        return FALSE; 
    //interrupt orig          
    /*for interrup work mode support */
    if (PS_INTERRUPT_MODE)
    {
        /* LTR2678_PS_MEAS_RATE*/
        ltr_data_buffer[0] = LTR2678_INTERRUPT;
        ltr_data_buffer[1] = 0x81;
        if(!_i2c_ltr2678_write(ltr_data_buffer,2))
            return FALSE;     

        ltr_data_buffer[0] = LTR2678_INTERRUPT_PST;
        ltr_data_buffer[1] = 0x10;
        if(!_i2c_ltr2678_write(ltr_data_buffer,2))
            return FALSE;
    }      
                
    irps = PsRetrieve(PSID_IRTH,thr,3);           
    if(irps == 3)
    {
        DEBUG_LOG("IR PSkey threshold %d,%d,offset %d",thr[0],thr[1],thr[2]);
        config->threshold_high = thr[0];
    	config->threshold_low = thr[1];
    	config->offset = thr[2];    
    }
    else
    {
        thr[0] = config->threshold_high;
        thr[1] = config->threshold_low;
        thr[2] = config->offset;
        PsStore(PSID_IRTH,thr,3);
    }    	
    	
    
    _ltr2678_ps_set_threshold(config->threshold_high, config->threshold_low);
    _ltr2678_ps_set_offset(config->offset);	
    setupPIOInputAndInterrupt(config->interrupt);
    /*=== init finish ===*/        
    _ltr2678_ps_enable(TRUE);     
    detectWearable(prox);
    config->init = TRUE; 
  
    return TRUE;
}

/*! \brief disable ltr2678 by config */
void _ltr2678Disable(proximityConfig *config)
{   
    if(config->init == TRUE)
    {
        config->init = FALSE;
        /* Disable interrupt and set weak pull down */
         setupPIODisableInterrupt(config->interrupt);   
         _ltr2678_ps_enable(FALSE); 
         tym_i2cdevice_deinit(i2c_device_prox_id); 
    }       
}

/*! \brief read prox value */
uint16 _ltr2678_ps_read(void)
{
    uint8 ps_low,ps_high;
    uint8 ltr_data_buffer[4];
    uint16 psdata;  
    if(proximity_config.init == FALSE)
        return 0;    
    if(!_i2c_ltr2678_read(LTR2678_PS_DATA_0,ltr_data_buffer,1))
        return 0;
    ps_low = ltr_data_buffer[0];      
    if(!_i2c_ltr2678_read(LTR2678_PS_DATA_1,ltr_data_buffer,1))
        return 0;          
    ps_high = ltr_data_buffer[0];   
    psdata = ((ps_high << 8) + ps_low);
    return psdata;    
}

/* ----------------- Global Function ----------------------- */
/*! \brief Register proximity sensor to the task */
bool appProximityClientRegister(Task task)
{
    proximityTaskData *prox = ProximityGetTaskData();

    if (NULL == prox->clients)
    {
        proximityConfig *config = appConfigProximity();
        prox->config = config;
        prox->clients = TaskList_Create();
        prox->state = PanicUnlessNew(proximityState);
        prox->state->proximity = proximity_state_unknown;
        tym_power_on(i2c_device_prox_id);
        if(_ltr2678Enable(config))
        {                
            /* Register for interrupt events */
            prox->task.handler = ltr2678InterruptHandler;
            PioMonitorRegisterTask(&prox->task, config->interrupt);
        }
        else
        {
            /*no IR sensor */
            xprint("ltr572 init failed");
            prox->state->proximity = proximity_state_not_in_proximity;
        }    
    }
    /* Send initial message to client */
    switch (prox->state->proximity)
    {
        case proximity_state_in_proximity:
            MessageSend(task, PROXIMITY_MESSAGE_IN_PROXIMITY, NULL);
            break;
        case proximity_state_not_in_proximity:
            MessageSend(task, PROXIMITY_MESSAGE_NOT_IN_PROXIMITY, NULL);
            break;
        case proximity_state_unknown:
        default:
            /* The client will be informed after the first interrupt */
            break;
    }

    return TaskList_AddTask(prox->clients, task);
}

/*! \brief Unregister proximity sensor to the task */
void appProximityClientUnregister(Task task)
{
    proximityTaskData *prox = ProximityGetTaskData();
    if(prox->clients != NULL)
    {    
        TaskList_RemoveTask(prox->clients, task);
        if (0 == TaskList_Size(prox->clients))
        {
            TaskList_Destroy(prox->clients);
            prox->clients = NULL;
            if(prox->state != NULL)
                free(prox->state);
            xprint("appProximityClientUnregister");
            _ltr2678Disable(prox->config);
            tym_power_off(i2c_device_prox_id);
            /* Unregister for interrupt events */
            PioMonitorUnregisterTask(&prox->task, prox->config->interrupt);
        }
    }
}

void appProximityPowerOn(void)
{
    proximityConfig *config = appConfigProximity();    
    tym_power_on(i2c_device_prox_id);
    _ltr2678Enable(config);       
}

void appProximityPowerOff(void)
{
    proximityConfig *config = appConfigProximity();  
    _ltr2678Disable(config);
    tym_power_off(i2c_device_prox_id);    
}

uint16 appProximityDataRead(void)
{
    return _ltr2678_ps_read();
}

void appProximityDataConfig(uint8 led_config,uint8 time_config)
{
    uint16 config[1];
    DEBUG_LOG("set config %d,%d",led_config,time_config);
    config[0] = (led_config << 8) | (time_config);
    PsStore(PSID_IRCONFIG, config, 1);
}

uint8 appProximityStatus(void)
{
    proximityTaskData *prox = ProximityGetTaskData();    
    uint8 wearstatus = notwear;
    if(prox->state != NULL)
    {    
        if(prox->state->proximity == proximity_state_in_proximity)
            wearstatus = wear;
    }
           
    return wearstatus;    
}

#endif/*HAVE_LTR2678*/
#endif/*INCLUDE_PROXIMITY*/

