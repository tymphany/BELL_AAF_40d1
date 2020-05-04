/*************************************************************************
 * Copyright (c) 2017 - 2018 Qualcomm Technologies International, Ltd.
 * All Rights Reserved.
 * Qualcomm Technologies International, Ltd. Confidential and Proprietary.
*************************************************************************/
/**
 * \file clk_mgr_private.h
 * \ingroup clk_mgr
 *
 * Private definitions for the clk_mgr component.
 */
#ifndef CLK_MGR_PRIVATE_H
#define CLK_MGR_PRIVATE_H

/****************************************************************************
Include Files
*/
#include "hal/hal_clock.h"
#include "audio_log/audio_log.h"
#include "pmalloc/pl_malloc.h"
#include "patch/patch.h"
#include "pl_assert.h"
#include "clk_mgr/clk_mgr_for_adaptors.h"
#include "clk_mgr.h"
#include "platform/pl_intrinsics.h"

#ifdef INSTALL_EXTERNAL_MEM
#include "extmem/extmem_cntrl.h"
#endif 

/* Default kcodec divider */
#define DEFAULT_CODEC_DIV   4

/*  clk_mgr clock status and mode
 *  cpu_clk_freq_mhz = root clock comes from curator
 *  eng_clk_freq_mhz = engine clock comes from curator
 *  kcodec_power_mode = KCODEC configured power mode
 *  kcodec_div = kcodec divider. It is used for error prediction if apps tries
 *  kcodec_clients = Resource manager treats kcodec and digimic as different
 *                  resource. We need to keep track here for now
 *  to change the clock without tearing the graph
 *  */
typedef struct clk_status
{
    OP_MODE mode;
    CLK_FREQ_MHZ cpu_clk_freq_mhz;
    CLK_FREQ_MHZ eng_clk_freq_mhz;
    CLK_FREQ_MHZ dds_clk_freq_mhz;
    CLK_MGR_POWER_SAVE_MODES kcodec_power_mode;
    uint16 kcodec_div;
} CLK_STATUS;

/*clk_mgr module status and mode*/
typedef struct
{
    CLK_FREQ_MHZ op_mode_clk[NUMBER_OF_MODES]; /* array idx = mode val= freq */
    CLK_STATUS current_mode;      /* Running clock configurations for the chip*/
    CLK_MGR_POWER_SAVE_MODES kcodec_power_mode_req; /* An outstanding PM request */
    void *priv_data;                   /* Clients private data */
    CLK_MGR_CBACK clk_mgr_cback;       /* Clients callback */
    uint16 kcodec_clients;  /* kcodec_clients = Resource manager Does not
                               support shared resources. Clk_mgr will keep
                               track of this for now */
    uint16 dds_clients; /* Stores how many resources are using DDS clock*/
} CLK_MGR_STATUS;

/**
 * \brief Utility function for typecasting CLK_FREQ_MHZ to HAL_FREQ_MHZ
 */
static inline HAL_FREQ_MHZ clk_mgr_to_hal_clk(CLK_FREQ_MHZ frequency)
{
    return (HAL_FREQ_MHZ) frequency;
}

/**
 * Definition of callback type for retrieving cpu and engine clock configuration
 */
typedef void (*CLK_MGR_SUBMIT_CBACK)(CLK_FREQ_MHZ cpu_clock,
                                     CLK_FREQ_MHZ engine_clock);

/**
 * Definition of callback type for retrieving cpu and engine clock configuration
 */
typedef void (*CLK_MGR_DDS_SUBMIT_CBACK)(uint16 clock_state,
                                     CLK_FREQ_MHZ dds_clock);

extern CLK_MGR_POWER_SAVE_MODES clk_mgr_get_power_mode(void);

extern bool clk_mgr_set_modes_freq(CLK_FREQ_MHZ *mode_freq);

extern void clk_mgr_update_power_mode_req(CLK_MGR_POWER_SAVE_MODES power_mode);

extern bool clk_mgr_submit_clocks_change(CLK_FREQ_MHZ cpu_clock,
                                         CLK_FREQ_MHZ engine_clock,
                                         CLK_MGR_SUBMIT_CBACK callback);
extern bool clk_mgr_chip_set_mode(OP_MODE mode, CLK_MGR_STATUS *clk_mgr_status);
extern CLK_MGR_STATUS* clk_mgr_status_get_instance(void);
extern CLK_FREQ_MHZ clk_mgr_calculate_engine_clock(CLK_FREQ_MHZ kcodec_clock_cfg, CLK_MGR_STATUS *clk_mgr_status);
extern bool clk_mgr_set_chip_aud(CLK_FREQ_MHZ eng_clk_freq_mhz, CLK_MGR_STATUS *clk_mgr_status, 
                            void *priv, CLK_MGR_CBACK callback);

extern CLK_FREQ_MHZ clk_mgr_power_mode_to_freq(void);

extern CLK_FREQ_MHZ clk_mgr_cpu_mode_req_to_freq(CLK_MGR_CPU_CLK requested_mode, OP_MODE current_mode);

#ifdef CHIP_HAS_SEPARATE_DDS_DOMAIN
extern bool clk_mgr_submit_dds_clock_change(CLK_FREQ_MHZ dds_clock, CLK_MGR_DDS_SUBMIT_CBACK callback);

#endif

#ifdef INSTALL_EXTERNAL_MEM
static inline void clk_mgr_set_extmem_change_clk(CLK_FREQ_MHZ cpu_clk)
{

    EXTMEM_CLK clk= (cpu_clk < FREQ_80MHZ)?
                     EXTMEM_CLK_32MHZ:EXTMEM_CLK_80MHZ;

    if(clk != extmem_get_clk(EXTMEM_SPI_RAM))
    {
        if(!extmem_clock_cntrl_with_retry(EXTMEM_SPI_RAM, clk, NULL,1000))
        {
            L1_DBG_MSG1("Clkmgr: Failed to change extmem clk %d", clk);
        }
    }
}
#else
#define clk_mgr_set_extmem_change_clk(x)
#endif  /* INSTALL_EXTERNAL_MEM */

#define CLK_MGR_INVALID_TEMPERATURE ((int16) 0x7fff)

#ifdef INSTALL_TEMPERATURE_MONITORING
/**
 * \brief Setup the temperature monitoring functionality.
 *
 * \note The function should be called as early as possible in the
 *       boot process.
 */
extern void clk_mgr_temperature_init(void);

/**
 * \brief Start reporting periodically the chip temperature.
 *
 * \note This is meant to be used as soon as a hardware block that
 *       needs to be recalibrated in case of change of temperature
 *       is being used.
 */
extern void clk_mgr_temperature_enable_monitoring(void);

/**
 * \brief Stop reporting periodically the chip temperature.
 *
 * \note This is meant to be used as soon as all hardware blocks that
 *       need to be recalibrated in case of change of temperature
 *       are not being used anymore.
 */
extern void clk_mgr_temperature_disable_monitoring(void);

/**
 * \brief Function called when a temperature measurement
 *        has been made.
 *
 * \param value Measured temperature in degrees Celsius.
 *
 * \note The special value CLK_MGR_INVALID_TEMPERATURE is
 *       used when querying the hardware is impossible
 *       (e.g. on an emulator).
 */
extern void clk_mgr_temperature_report(int16 value);
#else
#define clk_mgr_temperature_init()
#define clk_mgr_temperature_enable_monitoring()
#define clk_mgr_temperature_disable_monitoring()
#define clk_mgr_temperature_report(x)
#endif

#endif /* CLK_MGR_PRIVATE_H */
