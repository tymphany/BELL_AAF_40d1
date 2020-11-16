
/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file
\brief      Procedure to abort DFU.
*/

#include "tws_topology_procedure_dfu_abort.h"
#include "tws_topology_procedures.h"
#include "upgrade.h"
#include "device_upgrade.h"
#include <logging.h>
#include "upgrade_msg_host.h"

void TwsTopology_ProcedureDfuAbortStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);
void TwsTopology_ProcedureDfuAbortCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

const procedure_fns_t proc_dfu_abort_fns = {
    TwsTopology_ProcedureDfuAbortStart,
    TwsTopology_ProcedureDfuAbortCancel,
};

void TwsTopology_ProcedureDfuAbortStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data)
{
    UNUSED(goal_data);
    UNUSED(result_task);

    DEBUG_LOG("TwsTopology_ProcedureDfuAbortStart");

    /* procedure started synchronously, confirm start now */
    proc_start_cfm_fn(tws_topology_procedure_dfu_abort, procedure_result_success);

    /*Abort DFU when DFU is in progress. */
    if (UpgradeIsInProgress())
    {
        DEBUG_LOG("TwsTopology_ProcedureDfuAbortStart Abort DFU.");

        if(!UpgradeIsAborting())
            UpgradeHandleAbortDuringUpgrade();

        /*
         * This is a device triggered abort. Since its a defined abort,
         * it can switch to autonomous mode so that the graceful cleanup
         * especially erase on abort takes place.
         */
        UpgradePermit(upgrade_perm_assume_yes);
    }

    /* procedure completed synchronously so indicate completed already */
    Procedures_DelayedCompleteCfmCallback(proc_complete_fn, tws_topology_procedure_dfu_abort, procedure_result_success);
}

void TwsTopology_ProcedureDfuAbortCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    DEBUG_LOG("TwsTopology_ProcedureDfuAbortCancel");

    /* nothing to cancel, just return success to keep goal engine happy */
    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn, tws_topology_procedure_dfu_abort, procedure_result_success);
}
