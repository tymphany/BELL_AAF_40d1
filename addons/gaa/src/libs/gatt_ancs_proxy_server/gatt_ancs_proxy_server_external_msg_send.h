/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

#ifndef GATT_ANCS_PROXY_SERVER_EXTERNAL_MSG_SEND_H_
#define GATT_ANCS_PROXY_SERVER_EXTERNAL_MSG_SEND_H_

#include "gatt_ancs_proxy_server_private.h"

void gattAncsProxySendReadReadyConfigInd(GANCS_PROXY_SS *ancs_proxy, uint16 cid);
void gattAncsProxySendWriteReadyConfigInd(GANCS_PROXY_SS *ancs_proxy, uint16 cid, uint16 ready_c_cfg);
void gattAncsProxySendReadyCharacteristicUpdateInd(GANCS_PROXY_SS *ancs_proxy, uint16 cid, uint16 ready_char);

#endif
