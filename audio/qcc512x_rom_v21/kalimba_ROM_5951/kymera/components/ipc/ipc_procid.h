/****************************************************************************
 * Copyright (c) 2016 - 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file ipc_procid.h
 * \ingroup ipc
 *
 * Inter Processor Communication Public Header.
 * This provides the basic idea of a processor to the rest of the system. 
 */
#ifndef IPC_PROCID_H
#define IPC_PROCID_H

/**
 * Enumeration for processor identity number
 */
typedef enum
{
    IPC_PROCESSOR_0 = 0, /* Processor 0 */
    IPC_PROCESSOR_1 = 1, /* Processor 1 */
    IPC_PROCESSOR_MAX
} IPC_PROCESSOR_ID_NUM;

/**
 * \brief  Returns the processor ID
 */
extern IPC_PROCESSOR_ID_NUM ipc_get_processor_id(void);

/**
 * \brief  Returns TRUE if the processor number passed
 * as argument is invalid or not started.
 *
 * \param[in] pid - Number of the processor to test for.
 */
extern bool ipc_invalid_processor_id(IPC_PROCESSOR_ID_NUM pid);

#define IPC_PRIMARY_CONTEXT()   (ipc_get_processor_id() == IPC_PROCESSOR_0)
#define IPC_SECONDARY_CONTEXT() (ipc_get_processor_id() != IPC_PROCESSOR_0)

/* returns true if the processor id matches with the current processor context */
static inline bool IPC_ON_SAME_CORE(IPC_PROCESSOR_ID_NUM id)
{
    return ipc_get_processor_id() == id;
}

#endif /* IPC_PROCID_H */
