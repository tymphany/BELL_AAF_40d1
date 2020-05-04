/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_ancs_proxy_server_uuids.h
@brief   UUIDs used by ANCS Service
*/

#ifndef __GATT_ANCS_PROXY_SERVER_UUIDS_H__
#define __GATT_ANCS_PROXY_SERVER_UUIDS_H__


/*The Headset ANCS GATT service (UUID 67A846AD-DE3E-451B-A6D8-7B2899CA2370 ) must
  expose the following characteristics (all of which must require authorisation):
  Notification Source: UUID 9FBF120D-6301-42D9-8C58-25E699A21DBD (notifiable)
  Control Point: UUID 69D1D8F3-45E1-49A8-9821-9BBDFDAAD9D9 (writeable with response)
  Data Source: UUID 22EAC6E9-24D6-4BB5-BE44-B36ACE7C7BFB (notifiable)
  Ready: UUID 753EED35-A584-45BB-BAED-67FC7B2DC142 (read/notify) */

#define UUID_ANCS_PROXY_SERVICE                           0x67A846ADDE3E451BA6D87B2899CA2370
#define UUID_ANCS_PROXY_NOTIFICATION_SOURCE_CHAR          0x9FBF120D630142D98C5825E699A21DBD
#define UUID_ANCS_PROXY_CONTROL_POINT_CHAR                0x69D1D8F345E149A898219BBDFDAAD9D9
#define UUID_ANCS_PROXY_DATA_SOURCE_CHAR                  0x22EAC6E924D64BB5BE44B36ACE7C7BFB
#define UUID_ANCS_PROXY_READY_CHAR                        0x753EED35A58445BBBAED67FC7B2DC142

#endif /* __GATT_ANCS_PROXY_SERVER_UUIDS_H__ */
