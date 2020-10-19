/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\ingroup    kymera
\brief      List of UCIDs.

*/

#ifndef KYMERA_UCID_H_
#define KYMERA_UCID_H_

#include "kymera_chain_roles.h"
#include <chain.h>

/*@{*/


/*! \brief AEC_REF operator UCIDs */
typedef enum kymera_operator_ucids
{
    UCID_AEC_NB    = 0,
    UCID_AEC_WB    = 1,
    UCID_AEC_WB_VA = 2,
    UCID_AEC_UWB   = 3,
    UCID_AEC_SWB   = 4,
    UCID_AEC_DEFAULT = 5,
    UCID_AEC_SA_LT_MODE_1 = 6,
    UCID_AEC_SA_LT_MODE_2 = 7,
    UCID_AEC_SA_LT_MODE_3 = 8,
    UCID_AEC_NB_LT_MODE_1 = 9,
    UCID_AEC_NB_LT_MODE_2 = 10,
    UCID_AEC_NB_LT_MODE_3 = 11,
    UCID_AEC_WB_LT_MODE_1 = 12,
    UCID_AEC_WB_LT_MODE_2 = 13,
    UCID_AEC_WB_LT_MODE_3 = 14,
    UCID_AEC_UWB_LT_MODE_1 = 15,
    UCID_AEC_UWB_LT_MODE_2 = 16,
    UCID_AEC_UWB_LT_MODE_3 = 17,
    UCID_AEC_SWB_LT_MODE_1 = 18,
    UCID_AEC_SWB_LT_MODE_2 = 19,
    UCID_AEC_SWB_LT_MODE_3 = 20,
    UCID_AEC_A2DP_44_1_KHZ_LT_MODE_1 = 21,
    UCID_AEC_A2DP_44_1_KHZ_LT_MODE_2 = 22,
    UCID_AEC_A2DP_44_1_KHZ_LT_MODE_3 = 23,
    UCID_AEC_A2DP_48_KHZ_LT_MODE_1 = 24,
    UCID_AEC_A2DP_48_KHZ_LT_MODE_2 = 25,
    UCID_AEC_A2DP_48_KHZ_LT_MODE_3 = 26,
    UCID_AEC_A2DP_OTHER_SAMPLE_RATE_LT_MODE_1 = 27,
    UCID_AEC_A2DP_OTHER_SAMPLE_RATE_LT_MODE_2 = 28,
    UCID_AEC_A2DP_OTHER_SAMPLE_RATE_LT_MODE_3 = 29,
    UCID_AEC_WB_VA_LT_MODE_1 = 30,
    UCID_AEC_WB_VA_LT_MODE_2 = 31,
    UCID_AEC_WB_VA_LT_MODE_3 = 32,
    UCID_CVC_SEND    = 0,
    UCID_CVC_SEND_VA = 1,
    UCID_CVC_RECEIVE = 0,
    UCID_VOLUME_CONTROL = 0,
    UCID_SOURCE_SYNC = 0,
#ifdef ENABLE_TYM_PLATFORM
    UCID_PASS_ADD_HEADROOM = 0,
    UCID_PASS_REMOVE_HEADROOM = 1,
    UCID_USER_EQ_BANK0 = 0, //0x2480
    UCID_USER_EQ_BANK1 = 1, //0x2482
    UCID_USER_EQ_BANK2 = 2, //0x2484
    UCID_USER_EQ_BANK3 = 3, //0x2486
    UCID_USER_EQ_BANK4 = 4, //0x2488
    UCID_USER_EQ_BANK5 = 5, //0x248a
    UCID_USER_EQ_BANK6 = 6, //0x248c

    UCID_SPEAKER_EQ1 = 7,   //0x248e
    UCID_SPEAKER_EQ2 = 8    //0x2490
#endif/*ENABLE_TYM_PLATFORM*/
} kymera_operator_ucid_t;

/*! \brief Set the UCID for a single operator */
bool Kymera_SetOperatorUcid(kymera_chain_handle_t chain, chain_operator_role_t role, kymera_operator_ucid_t ucid);

/*@}*/

#endif /* KYMERA_UCID_H_ */
