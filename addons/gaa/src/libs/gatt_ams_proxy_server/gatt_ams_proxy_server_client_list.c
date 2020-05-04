/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

#include "gatt_ams_proxy_server_client_list.h"
#include "gatt_ams_proxy_server_db.h"
#include <gatt_proxy_stream.h>

#define MAX_ENTRIES 2

typedef struct
{
    const GAMS *client;
    gatt_proxy_stream_t remote_command_stream;
    gatt_proxy_stream_t entity_update_stream;
} client_entry_t;

static client_entry_t list[MAX_ENTRIES];


static client_entry_t * getListEntry(const GAMS *client)
{
    unsigned i;

    for(i = 0; i < MAX_ENTRIES; i++)
    {
        if (list[i].client == client)
            return &list[i];
    }

    return NULL;
}

static bool isClientInList(const GAMS *client)
{
    if (getListEntry(client))
        return TRUE;
    else
        return FALSE;
}

static gatt_proxy_stream_t createProxyStream(uint16 cid, uint16 client_handle, uint16 server_handle)
{
    if (client_handle != GATT_AMS_INVALID_HANDLE)
        return GattProxyStreamNew(cid, client_handle, server_handle);
    else
        return NULL;
}

static gatt_proxy_stream_t createRemoteCommandProxyStream(GAMS_PROXY_SS *ams_proxy, const GAMS *client)
{
    uint16 server_handle = ams_proxy->start_handle + HANDLE_AMS_PROXY_REMOTE_COMMAND_CHAR - 1;
    uint16 cid = GattAmsGetConnectionId(client);
    uint16 client_handle = GattAmsGetRemoteCommandHandle(client);

    return createProxyStream(cid, client_handle, server_handle);
}

static gatt_proxy_stream_t createEntityUpdateProxyStream(GAMS_PROXY_SS *ams_proxy, const GAMS *client)
{
    uint16 server_handle = ams_proxy->start_handle + HANDLE_AMS_PROXY_ENTITY_UPDATE_CHAR - 1;
    uint16 cid = GattAmsGetConnectionId(client);
    uint16 client_handle = GattAmsGetEntityUpdateHandle(client);

    return createProxyStream(cid, client_handle, server_handle);
}

static void initListEntry(GAMS_PROXY_SS *ams_proxy, const GAMS *client, client_entry_t *entry)
{
    entry->client = client;
    entry->remote_command_stream = createRemoteCommandProxyStream(ams_proxy, client);
    entry->entity_update_stream  = createEntityUpdateProxyStream(ams_proxy, client);
}

static void destroyListEntry(client_entry_t *entry)
{
    if (entry->remote_command_stream)
        GattProxyStreamDestroy(entry->remote_command_stream);
    if(entry->entity_update_stream)
        GattProxyStreamDestroy(entry->entity_update_stream);

    entry->client = NULL;
    entry->remote_command_stream = NULL;
    entry->entity_update_stream = NULL;
}

static void addClientInList(GAMS_PROXY_SS *ams_proxy, const GAMS *client)
{
    client_entry_t *entry = getListEntry(NULL);

    if (entry == NULL)
    {
        PANIC(("AMS PROXY: Client list is full!!!\n"));
    }
    else
        initListEntry(ams_proxy, client, entry);
}

void gattAmsProxyAddClient(GAMS_PROXY_SS *ams_proxy, const GAMS *client)
{
    if (isClientInList(client))
    {
        PANIC(("AMS PROXY: Client already in list!!!\n"));
    }
    else
        addClientInList(ams_proxy, client);
}

bool gattAmsProxyRemoveClient(const GAMS *client)
{
    client_entry_t *entry = getListEntry(client);

    if (entry)
    {
        destroyListEntry(entry);
        return TRUE;
    }
    else
        return FALSE;
}

const GAMS * gattAmsProxyGetClient(uint16 cid)
{
    unsigned i;

    for(i = 0; i < MAX_ENTRIES; i++)
    {
        if ((list[i].client != NULL) && (GattAmsGetConnectionId(list[i].client) == cid))
            return list[i].client;
    }

    return NULL;
}
