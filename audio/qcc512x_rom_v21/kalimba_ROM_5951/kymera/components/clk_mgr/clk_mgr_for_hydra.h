/*************************************************************************
 * Copyright (c) 2017-2018 Qualcomm Technologies International, Ltd.
 * All Rights Reserved.
 * Qualcomm Technologies International, Ltd. Confidential and Proprietary.
*************************************************************************/
/**
 * \defgroup clk_mgr Clock Manager
 *
 * \file clk_mgr_for_hydra.h
 * \ingroup clk_mgr
 *
 * Definition used exclusively by the ssccp_router.
 */
#ifndef CLK_MGR_FOR_HYDRA_H
#define CLK_MGR_FOR_HYDRA_H

/****************************************************************************
Include Files
*/
#include "clk_mgr/clk_mgr_common.h"
#include "types.h"

/**
 * \brief Handle a clock change response from the Curator.
 *
 * This function will only be called from ssccp_router
 *
 * \param pdu The undecoded PDU
 * \param pdu_len_words Its length
 *
 * \return True if the PDU was successfully unpacked.
 */
extern bool clk_mgr_variant_rsp_handler(const uint16 *pdu,
                                        uint16 pdu_len_words);
#endif /* CLK_MGR_FOR_HYDRA_H */