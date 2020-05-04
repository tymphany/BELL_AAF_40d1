/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

#include "gatt_ancs_proxy_server_external_msg_send.h"
#include "gatt_ancs_proxy_server.h"

void gattAncsProxySendReadReadyConfigInd(GANCS_PROXY_SS *ancs_proxy, uint16 cid)
{
    MAKE_GATT_ANCS_PROXY_SERVER_MESSAGE(GATT_ANCS_PROXY_SERVER_READ_READY_C_CFG_IND);
    message->cid = cid;
    MessageSend(ancs_proxy->app_task, GATT_ANCS_PROXY_SERVER_READ_READY_C_CFG_IND, message);
}

void gattAncsProxySendWriteReadyConfigInd(GANCS_PROXY_SS *ancs_proxy, uint16 cid, uint16 ready_c_cfg)
{
    MAKE_GATT_ANCS_PROXY_SERVER_MESSAGE(GATT_ANCS_PROXY_SERVER_WRITE_READY_C_CFG_IND);
    message->cid = cid;
    message->ready_c_cfg = ready_c_cfg;
    MessageSend(ancs_proxy->app_task, GATT_ANCS_PROXY_SERVER_WRITE_READY_C_CFG_IND, message);
}

void gattAncsProxySendReadyCharacteristicUpdateInd(GANCS_PROXY_SS *ancs_proxy, uint16 cid, uint16 ready_char)
{
    MAKE_GATT_ANCS_PROXY_SERVER_MESSAGE(GATT_ANCS_PROXY_SERVER_READY_CHAR_UPDATE_IND);
    message->cid = cid;
    message->ready_char = ready_char;
    MessageSend(ancs_proxy->app_task, GATT_ANCS_PROXY_SERVER_READY_CHAR_UPDATE_IND, message);
}
