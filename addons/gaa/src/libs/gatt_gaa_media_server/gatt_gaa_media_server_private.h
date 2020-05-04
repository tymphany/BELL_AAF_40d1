/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_gaa_media_server_private.h
@brief   Definitions and functions internal to the library
*/

#ifndef GATT_GAA_MEDIA_SERVER_PRIVATE_H_
#define GATT_GAA_MEDIA_SERVER_PRIVATE_H_

#include <panic.h>
#include <stdlib.h>

#include "gatt_gaa_media_server.h"

/* Macros for creating messages */
#define MAKE_GATT_GAA_MEDIA_SERVER_MESSAGE(TYPE) TYPE##_T *message = (TYPE##_T*) PanicNull(calloc(1, sizeof(TYPE##_T)))
#define MAKE_GATT_GAA_MEDIA_SERVER_MESSAGE_WITH_LEN(TYPE, LEN) TYPE##_T *message = (TYPE##_T*) PanicNull(calloc(1, sizeof(TYPE##_T) + (LEN) - 1))

#define GATT_GAA_MEDIA_MAX_CLIENTS (2)

typedef struct
{
    /*! Client characteristic configuration. */
    uint16 config[gaa_media_char_last];
    /*! Connection identifier of client. */
    uint16 cid;
} gatt_gaa_media_client_t;


/*! @brief The GAA_MEDIA server internal structure for the server role. */
typedef struct __GATT_GAA_MEDIA_SS
{
    TaskData lib_task;
    uint8* active_app;
    uint16 size_active_app;
    gatt_gaa_media_client_t client[GATT_GAA_MEDIA_MAX_CLIENTS];

} GATT_GAA_MEDIA_SS;


extern GATT_GAA_MEDIA_SS *gaa_media_local_server;

gatt_gaa_media_client_t *gattGaaMediaServerFindClient(uint16 cid);

#endif /* GATT_GAA_MEDIA_SERVER_PRIVATE_H_*/
