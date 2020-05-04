/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

#include "gatt_ancs_proxy_server_client_list.h"
#include "gatt_ancs_proxy_server_db.h"
#include <gatt_proxy_stream.h>

#define MAX_ENTRIES 2

typedef struct
{
    const GANCS *client;
    gatt_proxy_stream_t notification_source_stream;
    gatt_proxy_stream_t data_source_stream;
} client_entry_t;

static client_entry_t list[MAX_ENTRIES];


static client_entry_t * getListEntry(const GANCS *client)
{
    unsigned i;

    for(i = 0; i < MAX_ENTRIES; i++)
    {
        if (list[i].client == client)
            return &list[i];
    }

    return NULL;
}

static bool isClientInList(const GANCS *client)
{
    if (getListEntry(client))
        return TRUE;
    else
        return FALSE;
}

static gatt_proxy_stream_t createProxyStream(uint16 cid, uint16 client_handle, uint16 server_handle)
{
    if (client_handle != GATT_ANCS_INVALID_HANDLE)
        return GattProxyStreamNew(cid, client_handle, server_handle);
    else
        return NULL;
}

static gatt_proxy_stream_t createNotificationSourceProxyStream(GANCS_PROXY_SS *ancs_proxy, const GANCS *client)
{
    uint16 server_handle = ancs_proxy->start_handle + HANDLE_ANCS_PROXY_NOTIFICATION_SOURCE_CHAR - 1;
    uint16 cid = GattAncsGetConnectionId(client);
    uint16 client_handle = GattAncsGetNotificationSourceHandle(client);

    return createProxyStream(cid, client_handle, server_handle);
}

static gatt_proxy_stream_t createDataSourceProxyStream(GANCS_PROXY_SS *ancs_proxy, const GANCS *client)
{
    uint16 server_handle = ancs_proxy->start_handle + HANDLE_ANCS_PROXY_DATA_SOURCE_CHAR - 1;
    uint16 cid = GattAncsGetConnectionId(client);
    uint16 client_handle = GattAncsGetDataSourceHandle(client);

    return createProxyStream(cid, client_handle, server_handle);
}

static void initListEntry(GANCS_PROXY_SS *ancs_proxy, const GANCS *client, client_entry_t *entry)
{
    entry->client = client;
    entry->notification_source_stream = createNotificationSourceProxyStream(ancs_proxy, client);
    entry->data_source_stream  = createDataSourceProxyStream(ancs_proxy, client);
}

static void destroyListEntry(client_entry_t *entry)
{
    if (entry->notification_source_stream)
        GattProxyStreamDestroy(entry->notification_source_stream);
    if(entry->data_source_stream)
        GattProxyStreamDestroy(entry->data_source_stream);

    entry->client = NULL;
    entry->notification_source_stream = NULL;
    entry->data_source_stream = NULL;
}

static void addClientInList(GANCS_PROXY_SS *ancs_proxy, const GANCS *client)
{
    client_entry_t *entry = getListEntry(NULL);

    if (entry == NULL)
    {
        PANIC(("ANCS PROXY: Client list is full!!!\n"));
    }
    else
        initListEntry(ancs_proxy, client, entry);
}

void gattAncsProxyAddClient(GANCS_PROXY_SS *ancs_proxy, const GANCS *client)
{
    if (isClientInList(client))
    {
        PANIC(("ANCS PROXY: Client already in list!!!\n"));
    }
    else
        addClientInList(ancs_proxy, client);
}

bool gattAncsProxyRemoveClient(const GANCS *client)
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

const GANCS * gattAncsProxyGetClient(uint16 cid)
{
    unsigned i;

    for(i = 0; i < MAX_ENTRIES; i++)
    {
        if ((list[i].client != NULL) && (GattAncsGetConnectionId(list[i].client) == cid))
            return list[i].client;
    }

    return NULL;
}
