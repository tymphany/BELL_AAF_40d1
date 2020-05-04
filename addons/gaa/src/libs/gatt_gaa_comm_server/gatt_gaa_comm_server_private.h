/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_gaa_comm_server_private.h
@brief   Definitions and functions internal to the library
*/

#ifndef GATT_GAA_COMM_SERVER_PRIVATE_H_
#define GATT_GAA_COMM_SERVER_PRIVATE_H_

#include <csrtypes.h>
#include <message.h>
#include <panic.h>
#include <stdlib.h>
#include <gatt_manager.h>
#include "gatt_gaa_comm_server.h"
#include "gatt_gaa_comm_server_debug.h"


#define INVALID_HANDLE      (0xFFFF)
#define INVALID_SINK        (0xFFFF)
#define GATT_HEADER_SIZE    (3)
#define HANDLE_OFFSET       (2)

/* Macros for creating messages */
#define MAKE_GATT_GAA_COMM_SERVER_MESSAGE(TYPE) TYPE##_T *message = (TYPE##_T*)PanicNull(calloc(1,sizeof(TYPE##_T)))
#define MAKE_GATT_GAA_COMM_SERVER_MESSAGE_WITH_LEN(TYPE,LEN) TYPE##_T *message = (TYPE##_T*)PanicNull(calloc(1,sizeof(TYPE##_T) + (LEN) - 1))

/*! @brief The GAA COMM server internal structure for the server role.
 */
typedef struct __GGAA_COMM_SS
{
    Task            lib_task;
    Task            app_task;
    bdaddr          addr;
    uint16          cid;
    uint16          start_handle;
    unsigned        cc_control:2;
    unsigned        cc_audio:2;
} GGAA_COMM_SS;

GGAA_COMM_SS * GattGaaCommGetServer(void);
Task GattGaaCommGetAppTask(void);
Task GattGaaCommGetLibTask(void);
void GaaCommServerNotifyRegisterInterestSinkMsg(void);

#endif
