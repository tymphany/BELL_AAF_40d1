/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_gaa_comm_server_msg_handler.c
@brief   Library message handler
*/

#include "gatt_gaa_comm_server_msg_handler.h"

#include <logging.h>
#include <connection.h>
#include "gatt_gaa_comm_server_access.h"
#include "gatt_gaa_comm_server_private.h"
#include "gatt_gaa_comm_server_db.h"

void GattGaaCommServerMsgHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
    case GATT_MANAGER_SERVER_ACCESS_IND:
        handleGaaCommServerAccess((GATT_MANAGER_SERVER_ACCESS_IND_T *) message);
        break;

    default:
        DEBUG_LOG("GattGaaCommServerMsgHandler: unhandled 0x%04X");
        break;
    }
}
