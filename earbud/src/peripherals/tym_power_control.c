/*!
\copyright  Copyright (c) 2017 - 2019 TYMPHANY Technologies International, Ltd.
            All Rights Reserved.
            TYMPHANY Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       tym_power_control.c
\brief      Support for the power control interface
*/
/* ------------------------ Include ------------------------ */
#include <panic.h>
#include <logging.h>
#include <pio.h>
#include <pio_common.h>
#include "tym_pin_config.h"
#include "tym_power_control.h"
#include "earbud_tym_util.h"
#include "tym_i2c_control.h"
#include "earbud_config.h"
/* ------------------------ Defines ------------------------ */

#define xprint(x)            DEBUG_LOG(x)
#define xprintf(x, ...)      DEBUG_LOGF(x,  __VA_ARGS__)
/*! \brief Returns the PIOs bank number.
    \param pio The pio.
*/
#define PIO2BANK(pio) ((uint16)((pio) / 32))
/*! \brief Returns the PIO bit position mask within a bank.
    \param pio The pio.
*/
#define PIO2MASK(pio) (1UL << ((pio) % 32))


/* ------------------------ Types -------------------------- */
typedef enum{
    GPIO_CTR_IN,
    GPIO_CTR_OUT,
}GPIOCtrl_e;

/* --------------- Local Function Prototypes --------------- */
void _tym_gpio_on(void);
void _tym_gpio_off(void);
void _tym_gpio_off_by_device(void);
void _pinCtrlInOut(uint16 pin,uint8 direction);
/* --------------- Static Global Variables ----------------- */
struct __power_control_config gpower_ctrl_config = {
    .power_init_dev = 0,
};

/* ------------------ Local Function ----------------------- */
/*! \brief control GPIO i/o */
void _pinCtrlInOut(uint16 pin,uint8 direction)
{
    /* param param 1:bank, 2: mask, param 3: bits 
     * param bits  1: SW control,0:HW control
     */    
    volatile uint16 bank = PIO2BANK(pin);
    volatile uint32 mask = PIO2MASK(pin); 
    PioSetMapPins32Bank(bank, mask, mask);
     /** param dir => 1:output,0:input */
    if(direction == GPIO_CTR_IN)
        PioSetDir32Bank(bank, mask, 0);     
    else    
        PioSetDir32Bank(bank, mask, mask);    
}


/*! \brief init tymphany gpio deinitial by device*/
void _tym_gpio_off_by_device(void)
{
    if(gpower_ctrl_config.power_init_dev == i2c_device_touch_id)
        pioDriverPio(ENABLE_PIN,0);         
}
void _tym_gpio_on(void)
{
    pioDriverPio(ENABLE_PIN,1);
    pioDriverPio(TOUCH_POWER,1);
}


/*! \brief init tymphany gpio deinitial all*/
void _tym_gpio_off(void)
{
    pioDriverPio(TOUCH_POWER,0);
    pioDriverPio(ENABLE_PIN,0);
}
/* ----------------- Global Function ----------------------- */
/*! \brief initial power config and gpio config */
void tym_power_on(uint8 dev)
{
    if(gpower_ctrl_config.power_init_dev == 0)
    {
        _tym_gpio_on();
    }
    else
    {
        pioDriverPio(ENABLE_PIN,1);
    }    
    gpower_ctrl_config.power_init_dev |= dev;
}

/*! \brief de-initial I2c config from device */
void tym_power_off(uint8 dev)
{
	gpower_ctrl_config.power_init_dev &= ~dev; 
    if(gpower_ctrl_config.power_init_dev == 0)
    {    
       _tym_gpio_off();
    }
    else
    {
       _tym_gpio_off_by_device();
    }        
}

/* L:USB, H:GPIO */
void setCommunPort(xe_tymsetCPtype enable)
{
    xprintf("setCommunPort %d",enable);

    switch(enable) 
    {
        default:    
        case CH_USB:  pioDriverPio(MODE_SWITCH_PIN, FALSE); break;    
        case CH_UART: pioDriverPio(MODE_SWITCH_PIN, TRUE);  break;
    }
}

/*! \brief init tymphany gpio initial */
void tymGPIOInit(void)
{    
#ifdef ENABLE_UART    
    const xe_tymComMode COMType = UARTtype;
#else
    const xe_tymComMode COMType = GPIOtype;
#endif    
    uint8 debug_port;

    _pinCtrlInOut(ENABLE_PIN,GPIO_CTR_OUT);   
    pioDriverPio(ENABLE_PIN,1);
    
    _pinCtrlInOut(TOUCH_POWER,GPIO_CTR_OUT);    
    pioDriverPio(TOUCH_POWER,1);
    
    _pinCtrlInOut(STATUS_PIN,GPIO_CTR_OUT);
    pioDriverPio(STATUS_PIN, 0);

     _pinCtrlInOut(MODE_SWITCH_PIN,GPIO_CTR_OUT);     
    debug_port = getPSDebugMode();
    if(debug_port & PORTBIT)
    {
        DEBUG_LOG("keep USB");
        setCommunPort(CH_USB);        
    }   
    else
    {     
        setCommunPort(CH_UART);
    }
    switch(COMType)
    {
        case UARTtype: 
        {
            if(appConfigIsLeft())
                uart_data_stream_init(STATUS_PIN, POWER_PIN, 86400);
            else
                uart_data_stream_init(STATUS_PIN, POWER_PIN, 57600);
        } 
        break;    
        case GPIOtype:
        default:     
        {
            _pinCtrlInOut(STATUS_PIN,GPIO_CTR_OUT);
            pioDriverPio(STATUS_PIN, 0);
        }
        break; 
    }  
}
