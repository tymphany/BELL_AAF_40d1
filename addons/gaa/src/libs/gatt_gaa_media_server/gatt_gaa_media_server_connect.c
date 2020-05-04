/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_gaa_media_server_connect.c
@brief   Handle GAA Media Server GATT connection
*/

#include "gatt_gaa_media_server.h"
#include "gatt_gaa_media_server_private.h"

bool GattGaaMediaServerConnect(uint16 cid)
{
    gatt_gaa_media_client_t *client = gattGaaMediaServerFindClient(0);
    
    if (client)
    {
        client->cid = cid;
        return TRUE;
    }
    
    return FALSE;
}

bool GattGaaMediaServerDisconnect(uint16 cid)
{
    gatt_gaa_media_client_t *client = gattGaaMediaServerFindClient(cid);
    
    if (client)
    {
        memset(client, 0, sizeof *client);
        return TRUE;
    }
    
    return FALSE;
}
