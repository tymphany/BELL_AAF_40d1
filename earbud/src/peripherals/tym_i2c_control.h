/*!
\copyright  Copyright (c) 2017 - 2019 TYMPHANY Technologies International, Ltd.
            All Rights Reserved.
            TYMPHANY Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       tym_i2c_control.h
\brief      Support for the i2c control interface
*/

#ifndef __TYM_I2C_CONTROL__
#define __TYM_I2C_CONTROL__

#include <bitserial_api.h>
/*! Enumeration of event types supported by i2c dev. */
typedef enum
{
    i2c_device_touch_id = 1UL << 0,
    i2c_device_prox_id  = 1UL << 1,
    i2c_device_anc_id   = 1UL << 2,
} i2c_device_id_type;

/*! The high level configuration for taking measurement */
struct __i2c_control_config
{
    /*! The I2C clock address */
    uint8 i2c_addr;
    /*! The I2C initial device */    
    uint8 i2c_init_dev;
    /*! disable i2c communication for ANC calibration*/
    uint8 disable_i2c;
    /*! The I2C serial handle */  
    bitserial_handle handle;    
    /*! The PIOs used to communicate with the i2c */
    struct
    {
        /*! I2C serial data PIO */
        uint8 i2c_sda;
        /*! I2C serial clock PIO */
        uint8 i2c_scl;
    } pios;
};

bool tym_i2cdevice_init(uint8 i2c_addr,uint16 clock,uint8 dev);
void tym_i2cdevice_deinit(uint8 dev);
bool tym_i2c_write(uint8 i2c_addr, uint8 *write_data, uint16 write_size);
bool tym_i2c_read(uint8 i2c_addr,uint8 read_addr, uint8 *read_data, uint16 read_size);
void tym_disable_i2c_control(bool control);
#endif /* __TYM_I2C_CONTROL__ */
