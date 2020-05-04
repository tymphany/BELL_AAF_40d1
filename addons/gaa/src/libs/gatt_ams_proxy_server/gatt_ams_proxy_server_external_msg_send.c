/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

#include "gatt_ams_proxy_server_external_msg_send.h"
#include "gatt_ams_proxy_server.h"

void gattAmsProxySendReadReadyConfigInd(GAMS_PROXY_SS *ams_proxy, uint16 cid)
{
    MAKE_GATT_AMS_PROXY_SERVER_MESSAGE(GATT_AMS_PROXY_SERVER_READ_READY_C_CFG_IND);
    message->cid = cid;
    MessageSend(ams_proxy->app_task, GATT_AMS_PROXY_SERVER_READ_READY_C_CFG_IND, message);
}

void gattAmsProxySendWriteReadyConfigInd(GAMS_PROXY_SS *ams_proxy, uint16 cid, uint16 ready_c_cfg)
{
    MAKE_GATT_AMS_PROXY_SERVER_MESSAGE(GATT_AMS_PROXY_SERVER_WRITE_READY_C_CFG_IND);
    message->cid = cid;
    message->ready_c_cfg = ready_c_cfg;
    MessageSend(ams_proxy->app_task, GATT_AMS_PROXY_SERVER_WRITE_READY_C_CFG_IND, message);
}

void gattAmsProxySendReadyCharacteristicUpdateInd(GAMS_PROXY_SS *ams_proxy, uint16 cid, uint16 ready_char)
{
    MAKE_GATT_AMS_PROXY_SERVER_MESSAGE(GATT_AMS_PROXY_SERVER_READY_CHAR_UPDATE_IND);
    message->cid = cid;
    message->ready_char = ready_char;
    MessageSend(ams_proxy->app_task, GATT_AMS_PROXY_SERVER_READY_CHAR_UPDATE_IND, message);
}
