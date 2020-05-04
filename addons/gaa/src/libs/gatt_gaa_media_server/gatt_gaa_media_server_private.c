/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_gaa_media_server_private.c
@brief   Functions internal to the library
*/

#include "gatt_gaa_media_server_private.h"

GATT_GAA_MEDIA_SS *gaa_media_local_server;

gatt_gaa_media_client_t *gattGaaMediaServerFindClient(uint16 cid)
{
    int i;

    for (i = 0; i < GATT_GAA_MEDIA_MAX_CLIENTS; ++i)
    {
        if (gaa_media_local_server->client[i].cid == cid)
        {
            return &gaa_media_local_server->client[i];
        }
    }

    return NULL;
}
