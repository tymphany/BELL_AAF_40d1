/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_gaa_media_server_uuids.h
@brief   UUIDs used by GAA_MEDIA Service
*/

#ifndef __GATT_GAA_MEDIA_SERVER_UUIDS_H__
#define __GATT_GAA_MEDIA_SERVER_UUIDS_H__

/*!
   GAA_MEDIA_SERVER (UUID: 91C10D9C-AAEF-42BD-B6D6-8A648C19213D) is a service unique to GAA headset. It contains 4 characteristics:
   1. Active App (UUID: 99D1064E-4517-46AA-8FB4-6BE64DD1A1F1) is a read/write/notify characteristic.
   2. Media Command (UUID: FBE87F6C-3F1A-44B6-B577-0BAC731F6E85) is a write/notify characteristic.
   3. Media Status (UUID: 420791C0-BFF5-4BD1-B957-371614031136) is a write/notify characteristic
   4. Broadcast (UUID: E4EF5A46-30F9-4287-A3E7-643066ACB768) is a write/notify characteristic
 */
#define UUID_GAA_MEDIA_SERVICE             0x91C10D9CAAEF42BDB6D68A648C19213D
#define UUID_GAA_MEDIA_ACTIVE_APP_CHAR     0x99D1064E451746AA8FB46BE64DD1A1F1
#define UUID_GAA_MEDIA_COMMAND_CHAR        0xFBE87F6C3F1A44B6B5770BAC731F6E85
#define UUID_GAA_MEDIA_STATUS_CHAR         0x420791C0BFF54BD1B957371614031136
#define UUID_GAA_MEDIA_BROADCAST_CHAR      0xE4EF5A4630F94287A3E7643066ACB768

#endif /* __GATT_GAA_MEDIA_SERVER_UUIDS_H__ */
