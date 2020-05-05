/*!
\copyright  Copyright (c) 2017 - 2019 TYMPHANY Technologies International, Ltd.
            All Rights Reserved.
            TYMPHANY Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       tym_i2c_control.h
\brief      Support for the i2c control interface
*/

#ifndef __TYM_POWER_CONTROL__
#define __TYM_POWER_CONTROL__


/*! The high level configuration for taking measurement */
struct __power_control_config
{
    /*! The power initial device */    
    uint8 power_init_dev;
};
typedef enum {
  CH_USB  = 0,
  CH_UART = 1
} xe_tymsetCPtype;

typedef enum {
  GPIOtype = 0,
  UARTtype = 1
} xe_tymComMode;

extern void uart_data_stream_init(uint16 PIO_UART_TX, uint16 PIO_UART_RX, int BRD);

void tym_power_on(uint8 dev);
void tym_power_off(uint8 dev);
void setCommunPort(xe_tymsetCPtype enable);
void tymGPIOInit(void);
#endif /* __TYM_POWER_CONTROL__ */
