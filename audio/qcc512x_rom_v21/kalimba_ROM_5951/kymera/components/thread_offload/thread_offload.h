/****************************************************************************
 * Copyright (c) 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \defgroup thread_offload Audio Thread Offload
 * \ingroup thread_offload
 *
 */
#ifndef THREAD_OFFLOAD_H
#define THREAD_OFFLOAD_H

#include "types.h"

typedef void thread_offload_callback(void *context);

/** 
 * RPC request structure
 */
typedef struct 
{
    unsigned id;                        /**< RPC client ID (typically internal operator ID) */
    unsigned addr;                      /**< Target function call address */
    unsigned r0;                        /**< Target function parameter 0 */
    unsigned r1;                        /**< Target function parameter 1 */
    unsigned r2;                        /**< Target function parameter 2 */
    unsigned r3;                        /**< Target function parameter 3 */
    thread_offload_callback *callback;  /**< RPC completion callback. Called in interrupt context! */
    void *context;                      /**< Client-defined context for callback function */
} thread_offload_rpc_data;


/**
 * \brief  Test whether the configuration for thread offload is valid.
 *
 * \return TRUE if the relevant MIB key is set, and P1 is otherwise disabled
 * 
 */
extern bool audio_thread_offload_is_configured(void);

/**
 * \brief  Enable thread offload feature. 
 *
 * \return TRUE if successful, FALSE otherwise
 * 
 * This checks the configuration and allocates required memory
 * but doesn't actually start P1.
 */
extern bool audio_thread_offload_enable(void);

/**
 * \brief  Initialise the P1 offload thread.
 *
 * Starts P1 running in thread offload mode. Call audio_thread_offload_enable() first.
 * Note there is no return value
 * 
 * Subsequent calls to audio_thread_offload_is_active() will return TRUE 
 * if P1 is running
 *
 */
extern void audio_thread_offload_init(void);

/**
 * \brief  Check for an outstanding RPC request with matching ID.
 *
 * \return TRUE if a matching request is currently queued or running.
 * 
 * RPC clients can use this function to check for outstanding requests 
 * before queueing new ones.
 */
extern bool audio_thread_rpc_is_queued(unsigned id);

/**
 * \brief  Test whether the thread offload feature is active.
 *
 * \param Client ID to check against.
 *
 * \return TRUE if P1 is running in offload mode.
 * 
 */
extern bool audio_thread_offload_is_active(void);

/**
 * \brief  Test whether the thread offload feature is enabled.
 *
 * \return TRUE if audio_thread_offload_enable has completed sucessfully
 * 
 */
extern bool audio_thread_offload_is_enabled(void);

/**
 * \brief  Queue a new offload RPC request
 *
 * \param RPC data structure containing call address and parameters
 *
 * \return TRUE if successfully queued, FALSE otherwise.
 * 
 */
extern bool thread_offload_queue_rpc(thread_offload_rpc_data *rpc_data);

#endif 
