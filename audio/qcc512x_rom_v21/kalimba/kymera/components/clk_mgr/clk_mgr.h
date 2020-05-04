/*************************************************************************
 * Copyright (c) 2017-2018 Qualcomm Technologies International, Ltd.
 * All Rights Reserved.
 * Qualcomm Technologies International, Ltd. Confidential and Proprietary.
*************************************************************************/
/**
 * \defgroup clk_mgr Clock Manager
 *
 * \file clk_mgr.h
 * \ingroup clk_mgr
 *
 * Public definitions for clk_mgr
 */
#ifndef CLK_MGR_H
#define CLK_MGR_H

/****************************************************************************
Include Files
*/
#include "clk_mgr/clk_mgr_common.h"

/**
 * \brief Configures Audio CPU clock according to the requested mode.
 *
 * This function will call the callback before return when the requested mode
 * is the same as the current mode
 *
 * \param mode Requested mode
 * \param priv Private caller data
 * \param callback Callback to be called after the reception of the CCP
 *  response
 *
 * \return True if success.
 */
extern bool clk_mgr_set_mode(OP_MODE mode, void *priv, CLK_MGR_CBACK callback);

/**
 * \brief Configures Audio engine and KCODEC clock according to the requested
 * configuration.
 *
 * Audio power mode will be requested by accmd_clock_power_save_mode.
 * clk_mgr_aud_on will try to provide the requested by the apps power
 * configuration. This may not be possible if CPU clk configuration uses
 * different divider. In such cases, Audio will provide the next lowest
 * possible power configuration.
 *
 * \param priv Private caller data
 * \param callback Callback to be called after the reception of the CCP
 *  response
 *
 * \return True if  success.
 */
extern bool clk_mgr_aud_on(void *priv, CLK_MGR_CBACK callback);

/**
 * \brief Turns engine/Kcodec clock off
 *
 * \param priv Private caller data
 * \param callback Callback to be called after the reception of the CCP
 *  response
 *
 * \return True if  success.
 */
extern bool clk_mgr_aud_off(void *priv, CLK_MGR_CBACK callback);

/**
 * \brief Returns Audio CPU clock frequency.
 *
 * \return  Audio CPU clock frequency.
 */
extern CLK_FREQ_MHZ clk_mgr_get_current_cpu_freq(void);

/**
 * \brief Returns Audio CPU frequency for the requested mode. If the requested
 *  mode is not the current mode, the returned frequency will not be the same as
 *  the running Audio cpu clock.
 *
 * \param mode
 *
 * \return  Audio CPU clock frequency.
 */
extern CLK_FREQ_MHZ clk_mgr_get_mode_freq(OP_MODE mode);

/**
 * \brief Returns active operation mode for the clock manager.
 *
 * \return  clm_mgr operation mode.
 */
extern OP_MODE clk_mgr_get_current_mode(void);

/**
 * \brief Returns the current configuration for the audio/kcodec clock.
 *
 * \return  Running frequency of kcodec clock
 */
extern CLK_FREQ_MHZ clk_mgr_get_aud_clk(void);

/**
 * \brief Returns the current configuration for the engine clock.
 *
 * \return  Running frequency of Engine clock
 */
extern CLK_FREQ_MHZ clk_mgr_get_eng_clk(void);

/**
 * \brief Changes the MUX divider to support SPDIF 192 KHz 24 Bit Mode
 *
 * \return  True if successful
 */
extern bool clk_mgr_spdif_192KHz_support_en(void);

/**
 * \brief Changes the MUX divider to default value
 *
 * \return  True if successful
 */
extern bool clk_mgr_spdif_192KHz_support_disable(void);

/**
 * \brief Initialises default values for clock manager data
 */
extern void clk_mgr_init(void);

/**
 * \brief Initialises default values for CPU and kcodec clock
 */
extern void clk_mgr_chip_init(void);

#endif /* CLK_MGR_H */
