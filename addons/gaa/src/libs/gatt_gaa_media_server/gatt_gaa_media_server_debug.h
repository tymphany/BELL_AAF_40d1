/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_gaa_media_server_debug.h
@brief   Enable debugging variant of library
*/

#ifndef GATT_GAA_MEDIA_SERVER_DEBUG_H_
#define GATT_GAA_MEDIA_SERVER_DEBUG_H_

#ifdef GATT_GAA_MEDIA_SERVER_DEBUG_LIB


#ifndef DEBUG_PRINT_ENABLED
#define DEBUG_PRINT_ENABLED
#endif

#include <panic.h>
#include <print.h>


#define GATT_GAA_MEDIA_SERVER_DEBUG_INFO(x) {PRINT(x);}
#define GATT_GAA_MEDIA_SERVER_DEBUG_PANIC(x) {GATT_GAA_MEDIA_SERVER_DEBUG_INFO(x); Panic();}
#define GATT_GAA_MEDIA_SERVER_PANIC(x) {GATT_GAA_MEDIA_SERVER_DEBUG_INFO(x); Panic();}

#else /* GATT_APPLE_PROXY_DEBUG_LIB */

#define GATT_GAA_MEDIA_SERVER_DEBUG_INFO(x)
#define GATT_GAA_MEDIA_SERVER_DEBUG_PANIC(x)
#define GATT_GAA_MEDIA_SERVER_PANIC(x) {Panic();}

#endif /* GATT_GAA_MEDIA_SERVER_DEBUG_LIB */

#endif /* GATT_GAA_MEDIA_SERVER_DEBUG_H_ */
