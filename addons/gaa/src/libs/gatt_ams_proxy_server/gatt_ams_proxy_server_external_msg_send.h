/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

#ifndef GATT_AMS_PROXY_SERVER_EXTERNAL_MSG_SEND_H_
#define GATT_AMS_PROXY_SERVER_EXTERNAL_MSG_SEND_H_

#include "gatt_ams_proxy_server_private.h"

void gattAmsProxySendReadReadyConfigInd(GAMS_PROXY_SS *ams_proxy, uint16 cid);
void gattAmsProxySendWriteReadyConfigInd(GAMS_PROXY_SS *ams_proxy, uint16 cid, uint16 ready_c_cfg);
void gattAmsProxySendReadyCharacteristicUpdateInd(GAMS_PROXY_SS *ams_proxy, uint16 cid, uint16 ready_char);

#endif
