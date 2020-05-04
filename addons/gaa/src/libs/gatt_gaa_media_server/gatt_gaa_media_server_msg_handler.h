/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_gaa_media_server_msg_handler.h
@brief   Library message handler
*/

#ifndef GATT_GAA_MEDIA_SERVER_MSG_HANDLER_H_
#define GATT_GAA_MEDIA_SERVER_MSG_HANDLER_H_

#include <message.h>


/***************************************************************************
NAME
    GattGaaMediaServerMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the server role.
*/
void GattGaaMediaServerMsgHandler(Task task, MessageId id, Message payload);

#endif
