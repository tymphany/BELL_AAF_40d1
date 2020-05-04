/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

#include "gatt_ams_proxy_server_ready.h"
#include "gatt_ams_proxy_server_client_list.h"
#include "gatt_ams_proxy_server_external_msg_send.h"

uint16 gattAmsProxyGetReadyCharacteristic(uint16 cid)
{
    const GAMS *client = gattAmsProxyGetClient(cid);

    if (client == NULL)
        return 0;
    else
        return 1;
}

void gattAmsProxyClientIsReady(GAMS_PROXY_SS *ams_proxy, const GAMS *client)
{
    uint16 cid = GattAmsGetConnectionId(client);

    gattAmsProxyAddClient(ams_proxy, client);
    gattAmsProxySendReadyCharacteristicUpdateInd(ams_proxy, cid, gattAmsProxyGetReadyCharacteristic(cid));
}

void gattAmsProxyClientIsDestroyed(GAMS_PROXY_SS *ams_proxy, const GAMS *client)
{
    if (gattAmsProxyRemoveClient(client))
    {
        uint16 cid = GattAmsGetConnectionId(client);
        gattAmsProxySendReadyCharacteristicUpdateInd(ams_proxy, cid, gattAmsProxyGetReadyCharacteristic(cid));
    }
}
