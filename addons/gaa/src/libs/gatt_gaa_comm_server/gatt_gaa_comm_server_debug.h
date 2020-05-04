/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_gaa_comm_server_debug.h
@brief   Enable debugging variant of library
*/

#ifndef GATT_GAA_COMM_SERVER_DEBUG_H_
#define GATT_GAA_COMM_SERVER_DEBUG_H_

#define GATT_GAA_COMM_DEBUG_LIB
#ifdef GATT_GAA_COMM_DEBUG_LIB

#include <panic.h>
#include <logging.h>

/* TODO: find out if and why we want the line number; coding standard says put function name inn debug string */
#if 0
#define GATT_GAA_COMM_SERVER_DEBUG_INFO(...) {DEBUG_LOG("%d:", __LINE__); DEBUG_LOG(__VA_ARGS__);}
#else
#define GATT_GAA_COMM_SERVER_DEBUG_INFO(...) DEBUG_LOG(__VA_ARGS__)
#endif

#define GATT_GAA_COMM_SERVER_DEBUG_PANIC(...) {GATT_GAA_COMM_SERVER_DEBUG_INFO(__VA_ARGS__); Panic();}
#define GATT_GAA_COMM_SERVER_PANIC(...) GATT_GAA_COMM_SERVER_DEBUG_PANIC(__VA_ARGS__)

#else /* GATT_GAA_COMM_SERVER_DEBUG_LIB */

#define GATT_GAA_COMM_SERVER_DEBUG_INFO(...)
#define GATT_GAA_COMM_SERVER_DEBUG_PANIC(...)
#define GATT_GAA_COMM_SERVER_PANIC(...) {Panic();}

#endif /* GATT_GAA_COMM_SERVER_DEBUG_LIB */

#endif /* GATT_GAA_COMM_SERVER_DEBUG_H_ */
