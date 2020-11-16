/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.

QCC512x_QCC302x.SRC.1.0 R49.1 with changes for ADK-297, ADK-638, B-305341, B-305370

FILE NAME
    upgrade_msg_internal.h
    
DESCRIPTION

*/
#ifndef UPGRADE_MSG_INTERNAL_H_
#define UPGRADE_MSG_INTERNAL_H_

#ifndef UPGRADE_INTERNAL_MSG_BASE
#define UPGRADE_INTERNAL_MSG_BASE 0x300
#endif

typedef enum
{
    UPGRADE_INTERNAL_IN_PROGRESS = UPGRADE_INTERNAL_MSG_BASE,
    UPGRADE_INTERNAL_IN_PROGRESS_JOURNAL,
    UPGRADE_INTERNAL_CONTINUE,

    /*! used to prompt the state machine to perform a warm reboot
        after we've asked the VM application for permission */
    UPGRADE_INTERNAL_REBOOT,

    /*! used to prompt the state machine to perform a partition erase
        tasks after we've asked the VM application for permission */
    UPGRADE_INTERNAL_ERASE,

    UPGRADE_INTERNAL_COMMIT,

    /*! internal message to set the state machine to battery low */
    UPGRADE_INTERNAL_BATTERY_LOW,
    /*ENABLE_TYM_PLATFORM added Qualcomm patch QTILVM_TYM_RHA_Changes_r40_1_v2 for OTA issue*/
    /*! send to itself after reboot to commit, it is used to handle no reconnection cases */
    /* B-305341 Handle DFU timeout and abort in the post reboot phase */
    UPGRADE_INTERNAL_RECONNECTION_TIMEOUT,
 
    /*! Internal message used to delay the reboot of devices to revert commit */
    UPGRADE_INTERNAL_DELAY_REVERT_REBOOT
    /* End B-305341 */
} UpgradeMsgInternal;

#endif /* UPGRADE_MSG_INTERNAL_H_ */
