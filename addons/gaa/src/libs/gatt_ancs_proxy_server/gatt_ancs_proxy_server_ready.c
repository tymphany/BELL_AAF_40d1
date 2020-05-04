/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

#include "gatt_ancs_proxy_server_ready.h"
#include "gatt_ancs_proxy_server_client_list.h"
#include "gatt_ancs_proxy_server_external_msg_send.h"

uint16 gattAncsProxyGetReadyCharacteristic(uint16 cid)
{
    const GANCS *client = gattAncsProxyGetClient(cid);

    if (client == NULL)
        return 0;
    else
        return 1;
}

void gattAncsProxyClientIsReady(GANCS_PROXY_SS *ancs_proxy, const GANCS *client)
{
    uint16 cid = GattAncsGetConnectionId(client);

    gattAncsProxyAddClient(ancs_proxy, client);
    gattAncsProxySendReadyCharacteristicUpdateInd(ancs_proxy, cid, gattAncsProxyGetReadyCharacteristic(cid));
}

void gattAncsProxyClientIsDestroyed(GANCS_PROXY_SS *ancs_proxy, const GANCS *client)
{
    if (gattAncsProxyRemoveClient(client))
    {
        uint16 cid = GattAncsGetConnectionId(client);
        gattAncsProxySendReadyCharacteristicUpdateInd(ancs_proxy, cid, gattAncsProxyGetReadyCharacteristic(cid));
    }
}
