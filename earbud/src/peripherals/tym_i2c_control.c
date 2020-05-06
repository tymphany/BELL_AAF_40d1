/*!
\copyright  Copyright (c) 2017 - 2019 TYMPHANY Technologies International, Ltd.
            All Rights Reserved.
            TYMPHANY Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       tym_i2c_control.c
\brief      Support for the i2c control interface
*/
/* ------------------------ Include ------------------------ */
#include <panic.h>
#include <pio.h>
#include <logging.h>
#include "tym_i2c_control.h"
#include "tym_pin_config.h"
/* ------------------------ Defines ------------------------ */

/*! \brief Returns the PIOs bank number.
    \param pio The pio.
*/
#define PIO2BANK(pio) ((uint16)((pio) / 32))
/*! \brief Returns the PIO bit position mask within a bank.
    \param pio The pio.
*/
#define PIO2MASK(pio) (1UL << ((pio) % 32))


/* ------------------------ Types -------------------------- */


/* --------------- Local Function Prototypes --------------- */
void i2c_gpio_init(void);
void i2c_ErrorRecovery(void);
/* --------------- Static Global Variables ----------------- */
struct __i2c_control_config gi2c_ctrl_config = {
    .i2c_addr = 0x44,
    .i2c_init_dev = 0,
    .disable_i2c = 0,
    .handle = BITSERIAL_HANDLE_ERROR,
    .pios = {
        .i2c_scl = I2C_SCL_PIN,
        .i2c_sda = I2C_SDA_PIN,
    },
};

/* ------------------ Local Function ----------------------- */
void i2c_gpio_init(void)
{
    int i;
    uint16 bank;
    uint32 mask;
    struct
    {
        uint16 pio;
        pin_function_id func;
    } i2c_pios[] = {{gi2c_ctrl_config.pios.i2c_scl, BITSERIAL_0_CLOCK_OUT},
                    {gi2c_ctrl_config.pios.i2c_scl, BITSERIAL_0_CLOCK_IN},
                    {gi2c_ctrl_config.pios.i2c_sda, BITSERIAL_0_DATA_OUT},
                    {gi2c_ctrl_config.pios.i2c_sda, BITSERIAL_0_DATA_IN}}; 
   
    for (i = 0; i < ARRAY_DIM(i2c_pios); i++)
    {
        uint16 pio = i2c_pios[i].pio;
        bank = PIO2BANK(pio);
        mask = PIO2MASK(pio);

        /* Setup I2C PIOs with strong pull-up */
        PanicNotZero(PioSetMapPins32Bank(bank, mask, 0));
        PanicFalse(PioSetFunction(pio, i2c_pios[i].func));
        PanicNotZero(PioSetDir32Bank(bank, mask, 0));
        PanicNotZero(PioSet32Bank(bank, mask, mask));
        PanicNotZero(PioSetStrongBias32Bank(bank, mask, mask));
    } 
                          
}

/* ----------------- Global Function ----------------------- */
/*! \brief initial I2c config and gpio config */
bool tym_i2cdevice_init(uint8 i2c_addr,uint16 clock,uint8 dev)
{
    bitserial_config config;
    bitserial_result result;
      
    if(gi2c_ctrl_config.i2c_init_dev == 0)
    {
        gi2c_ctrl_config.i2c_init_dev |= dev;    
        gi2c_ctrl_config.i2c_addr = i2c_addr;
        /* setup i2c gpio pin config */
        i2c_gpio_init();
        /* setup i2c mode */
        memset(&config, 0, sizeof(bitserial_config));
        config.mode = BITSERIAL_MODE_I2C_MASTER;
        config.clock_frequency_khz = clock;
        config.u.i2c_cfg.i2c_address = i2c_addr;
        config.u.i2c_cfg.flags = BITSERIAL_I2C_ACT_ON_NAK_STOP;//BITSERIAL_I2C_ACT_ON_NAK_CONTINUE;
        config.timeout_ms = 250;
        
        gi2c_ctrl_config.handle = BitserialOpen(BITSERIAL_BLOCK_0, &config);
        if (gi2c_ctrl_config.handle == BITSERIAL_HANDLE_ERROR)
        {
            gi2c_ctrl_config.i2c_init_dev = 0;
            return FALSE;
        }
        return TRUE;
    }
    else
    {    
        gi2c_ctrl_config.i2c_init_dev |= dev;
        result = BitserialChangeParam(gi2c_ctrl_config.handle, BITSERIAL_PARAMS_I2C_DEVICE_ADDRESS, i2c_addr, BITSERIAL_FLAG_BLOCK);
        if(result != BITSERIAL_RESULT_SUCCESS)
        {
            return FALSE;
        }  
        gi2c_ctrl_config.i2c_addr = i2c_addr;
        return TRUE;
    }
}

/*! \brief de-initial I2c config from device */
void tym_i2cdevice_deinit(uint8 dev)
{
	gi2c_ctrl_config.i2c_init_dev &= ~dev; 
    if(gi2c_ctrl_config.i2c_init_dev == 0)
    {    
        if(gi2c_ctrl_config.handle != BITSERIAL_HANDLE_ERROR)
        {    
            BitserialClose(gi2c_ctrl_config.handle);
    		gi2c_ctrl_config.handle = BITSERIAL_HANDLE_ERROR;
    	}
    }    
}

/*! \brief I2c read i2c_addr:device address,read_addr:register address,read_data:data pointer,read_size:read data size */
bool tym_i2c_read(uint8 i2c_addr,uint8 read_addr, uint8 *read_data, uint16 read_size)
{
    bitserial_result result;
    if(gi2c_ctrl_config.disable_i2c)
    {
        DEBUG_LOG("disable I2C mode");
        return TRUE;
    }
    if( gi2c_ctrl_config.i2c_addr != i2c_addr)
    {
        result = BitserialChangeParam(gi2c_ctrl_config.handle, BITSERIAL_PARAMS_I2C_DEVICE_ADDRESS, i2c_addr, BITSERIAL_FLAG_BLOCK);
        if(result != BITSERIAL_RESULT_SUCCESS)
        {
            return FALSE;
        }
        gi2c_ctrl_config.i2c_addr = i2c_addr;
    }    
    result = BitserialTransfer(gi2c_ctrl_config.handle, BITSERIAL_NO_MSG, &read_addr, 1, read_data, read_size);
    if (result != BITSERIAL_RESULT_SUCCESS)
    {
        DEBUG_LOG("error i2c r %d",result);
        //close reopen
        if(result == BITSERIAL_RESULT_TIMEOUT)
            i2c_ErrorRecovery();
        return FALSE; /* error */
    }
    return TRUE;
}

/*! \brief I2c write i2c_addr:device address,write_data:write data pointer,write_size:write data size */
bool tym_i2c_write(uint8 i2c_addr, uint8 *write_data, uint16 write_size)
{
    bitserial_result result;
    if(gi2c_ctrl_config.disable_i2c)
    {
        DEBUG_LOG("disable I2C mode");
        return TRUE;
    }
    
    if(gi2c_ctrl_config.i2c_addr != i2c_addr)
    {
        result = BitserialChangeParam(gi2c_ctrl_config.handle, BITSERIAL_PARAMS_I2C_DEVICE_ADDRESS, i2c_addr, 0);
        if(result != BITSERIAL_RESULT_SUCCESS)
        {
            return FALSE;
        }
       gi2c_ctrl_config.i2c_addr = i2c_addr;
    }    
    
    result = BitserialWrite(gi2c_ctrl_config.handle, BITSERIAL_NO_MSG, write_data, write_size,BITSERIAL_FLAG_BLOCK);
    if (result != BITSERIAL_RESULT_SUCCESS)
    {       
        DEBUG_LOG("error i2c w");
        //close reopen
        if(result == BITSERIAL_RESULT_TIMEOUT)
            i2c_ErrorRecovery();
        
        return FALSE;
    }  
    return TRUE;
}

/*! \brief i2c error recovery reconfig */
void i2c_ErrorRecovery(void)
{
    bitserial_config config;
    BitserialClose(gi2c_ctrl_config.handle);
    
    config.mode = BITSERIAL_MODE_I2C_MASTER;
    config.clock_frequency_khz = 100;
    config.timeout_ms = 250;
    config.u.i2c_cfg.i2c_address = gi2c_ctrl_config.i2c_addr;
    config.u.i2c_cfg.flags = BITSERIAL_I2C_ACT_ON_NAK_STOP;//BITSERIAL_I2C_ACT_ON_NAK_CONTINUE;
    gi2c_ctrl_config.handle = BitserialOpen(BITSERIAL_BLOCK_0, &config);
}

/*! \brief disable/enable i2c control */
void tym_disable_i2c_control(bool control)
{
    gi2c_ctrl_config.disable_i2c = control;
}


