/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_gaa_comm_server_access.h
@brief   Server read/write access
*/

#ifndef GATT_GAA_COMM_SERVER_ACCESS_H_
#define GATT_GAA_COMM_SERVER_ACCESS_H_

#include "gatt_gaa_comm_server.h"


/***************************************************************************
NAME
    handleGaaCommServerAccess

DESCRIPTION
    Handles the GATT_MANAGER_SERVER_ACCESS_IND message that was sent to the GAA COMM library.
*/
void handleGaaCommServerAccess(const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind);

#endif
