/*******************************************************************************
Copyright (c) 2018 Qualcomm Technologies International, Ltd.
 
*******************************************************************************/

#ifndef __GATT_FAST_PAIR_SERVER_UUIDS_H__
#define __GATT_FAST_PAIR_SERVER_UUIDS_H__

/* UUIDs for Fast Pair Service and Characteristics*/

#define UUID_FAST_PAIR_SERVICE                      0xFE2C

#ifdef ENABLE_TYM_PLATFORM
/*Qualcomm Patch: improving fastpairing pairing time*/
#define UUID_KEYBASED_PAIRING                       0xFE2C1234836648148EB001DE32100BEA
#define UUID_PASSKEY                                0xFE2C1235836648148EB001DE32100BEA
#define UUID_ACCOUNT_KEY                            0xFE2C1236836648148EB001DE32100BEA
#else
#define UUID_KEYBASED_PAIRING                       0x1234
#define UUID_PASSKEY                                0x1235
#define UUID_ACCOUNT_KEY                            0x1236
#endif

#endif /* __GATT_FAST_PAIR_SERVER_UUIDS_H__ */

