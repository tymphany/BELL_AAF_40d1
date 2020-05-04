/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_ancs_proxy_server_private.h
@brief   Definitions and functions internal to the library
*/

#ifndef GATT_ANCS_PROXY_SERVER_PRIVATE_H_
#define GATT_ANCS_PROXY_SERVER_PRIVATE_H_

#include <print.h>
#include <panic.h>
#include <stdlib.h>

#ifdef GATT_ANCS_PROXY_SERVER_DEBUG_LIB
#define DEBUG_PANIC(x) {PRINT(x); Panic();}
#else
#define DEBUG_PANIC(x) {PRINT(x);}
#endif

#define PANIC(x) {{PRINT(x); Panic();}}

#define MAKE_GATT_ANCS_PROXY_SERVER_MESSAGE(TYPE) TYPE##_T *message = (TYPE##_T*)PanicNull(calloc(1,sizeof(TYPE##_T)))
#define MAKE_GATT_ANCS_PROXY_SERVER_MESSAGE_WITH_LEN(TYPE,LEN) TYPE##_T *message = (TYPE##_T*)PanicNull(calloc(1,sizeof(TYPE##_T) + (LEN) - 1))

typedef enum
{
    ready_size = 2
} value_sizes_t;

typedef enum
{
    GATT_ANCS_PROXY_SERVER_SEND_NOTIFICATION,
    GATT_ANCS_PROXY_SERVER_SEND_NOTIFICATION_LATER,
    INTERNAL_MSG_WRITE_REMOTE_SERVER_CHAR
} internal_ancs_proxy_server_msg_t;

typedef struct
{
    uint16 cid;
    uint16 handle;
    uint16 length;
    uint8  value[1];
} GATT_ANCS_PROXY_SERVER_SEND_NOTIFICATION_T;

/*
    The ANCS proxy server internal structure for the server role.
*/
typedef struct
{
    TaskData lib_task;
    Task     app_task;
    uint16   start_handle;
} GANCS_PROXY_SS;

#endif
