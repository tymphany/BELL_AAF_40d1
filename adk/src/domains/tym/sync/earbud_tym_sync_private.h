/*!
\copyright  Copyright (c) 2005 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Key Sync component private header.
*/

#ifndef TYM_SYNC_PRIVATE_H
#define TYM_SYNC_PRIVATE_H

#include <message.h>

/*! \brief Key sync task data. */
typedef struct
{
    TaskData task;
    bool leftpairact;
    bool rightpairact;    
} tym_sync_task_data_t;

/*! \brief Component level visibility of the key sync task data. */
extern tym_sync_task_data_t tym_sync;

/*! \brief Accessor for key sync task data. */
#define tymSync_GetTaskData()   (&tym_sync)

/*! \brief Accessor for key sync task. */
#define tymSync_GetTask()       (&tym_sync.task)

#endif /* TYM_SYNC_PRIVATE_H */
