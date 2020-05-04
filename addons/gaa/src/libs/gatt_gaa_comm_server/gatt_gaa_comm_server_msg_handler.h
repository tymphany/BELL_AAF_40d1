/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_gaa_comm_server_msg_handler.h
@brief   Library message handler
*/

#ifndef GATT_GAA_COMM_SERVER_MSG_HANDLER_H_
#define GATT_GAA_COMM_SERVER_MSG_HANDLER_H_

#include <message.h>

/***************************************************************************
NAME
    GattGaaCommServerMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the server role.
*/
void GattGaaCommServerMsgHandler(Task task, MessageId id, Message payload);

#endif
