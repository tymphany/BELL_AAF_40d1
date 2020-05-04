/*************************************************************************
 * Copyright (c) 2017-2018 Qualcomm Technologies International, Ltd.
 * All Rights Reserved.
 * Qualcomm Technologies International, Ltd. Confidential and Proprietary.
*************************************************************************/
/**
 * \defgroup clk_mgr Clock Manager
 *
 * \file clk_mgr_common.h
 * \ingroup clk_mgr
 *
 * Common definitions included indirectly by all users of the clock manager.
 */
#ifndef CLK_MGR_COMMON_H
#define CLK_MGR_COMMON_H

#include "status_prim.h"

/* CPU power modes */
typedef enum
{
    LOW_POWER,
    CUSTOM,
    ACTIVE,
    OP_MODE_N /*Do not use! keeps track of count*/
} OP_MODE;

/* Number of operation modes */
#define NUMBER_OF_MODES   OP_MODE_N

/* clock frequency
 * In case of Stre: Forced to uint16 to match curator CCP response
 * AuraPlus: It should match with HAL_FREQ_MHZ */
typedef enum
{
    CLK_OFF,
    FREQ_1MHZ = 1,
    FREQ_2MHZ = 2,
    FREQ_4MHZ = 4,
    FREQ_8MHZ = 8,
    FREQ_16MHZ = 16,
    FREQ_32MHZ = 32,
    FREQ_64MHZ = 64,
    FREQ_80MHZ = 80,
    FREQ_100MHZ = 100,
    FREQ_120MHZ = 120,
    FREQ_128MHZ = 128,
    FREQ_QUERY = 0x7ffe,      /* Used to request no change and get current value */
    FREQ_UNSPECIFIED = 0x7fff /* error code when input/response is invalid */
} CLK_FREQ_MHZ;

/* Note: This type must be kept in line with its counterpart
   on the adaptor side and on the AoV Client side. */
typedef enum
{
    CLK_MGR_CPU_CLK_NO_CHANGE = 0,
    CLK_MGR_CPU_CLK_EXT_LP_CLOCK = 1,
    CLK_MGR_CPU_CLK_VERY_LP_CLOCK = 2,
    CLK_MGR_CPU_CLK_LP_CLOCK = 3,
    CLK_MGR_CPU_CLK_VERY_SLOW_CLOCK = 4,
    CLK_MGR_CPU_CLK_SLOW_CLOCK = 5,
    CLK_MGR_CPU_CLK_BASE_CLOCK = 6,
    CLK_MGR_CPU_CLK_TURBO = 7,
    CLK_MGR_CPU_CLK_TURBO_PLUS = 8
} CLK_MGR_CPU_CLK;

/**
 * Definition of callback type. Return the CPU configuration status to
 * the client module
 *
 * \param priv Private caller data
 * \param status Audio status code
 *
 */
typedef void (*CLK_MGR_CBACK)(void *priv, STATUS_KYMERA status);

#endif /* CLK_MGR_COMMON_H */
