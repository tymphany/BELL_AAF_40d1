/*******************************************************************************
Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd.

*******************************************************************************/

#ifndef __GATT_AMS_PROXY_SERVER_UUIDS_H__
#define __GATT_AMS_PROXY_SERVER_UUIDS_H__

/* The AMS is a primary Bluetooth 4.0 service whose service UUID is 89D3502B-0F36-433A-8EF4-C502AD55F8DC.
   Remote Command: UUID 9B3C81D8-57B1-4A8A-B8DF-0E56F7CA51C2 (writeable, notifiable)
   Entity Update: UUID 2F7CABCE-808D-411F-9A0C-BB92BA96C102 (writeable with response, notifiable)
   Entity Attribute: UUID C6B2F38C-23AB-46D8-A6AB-A3A870BBD5D7 (readable, writeable) */

#define UUID_AMS_PROXY_SERVICE                           0x55F80AEFD89F41A49E360FFC88DC81CE
#define UUID_AMS_PROXY_REMOTE_COMMAND_CHAR               0x9B3C81D857B14A8AB8DF0E56F7CA51C2
#define UUID_AMS_PROXY_ENTITY_UPDATE_CHAR                0x2F7CABCE808D411F9A0CBB92BA96C102
#define UUID_AMS_PROXY_ENTITY_ATTRIBUTE_CHAR             0xC6B2F38C23AB46D8A6ABA3A870BBD5D7
#define UUID_AMS_PROXY_READY_CHAR                        0x3ADF41AFF7A14E16863E53A188D5BF8D

#endif /* __GATT_AMS_PROXY_SERVER_UUIDS_H__ */
