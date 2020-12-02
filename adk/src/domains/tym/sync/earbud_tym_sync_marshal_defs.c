/*!
\copyright  Copyright (c) 2005 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Definition of marshalled messages used by Tym Sync.
*/

#include "earbud_tym_sync_marshal_defs.h"

#include <marshal_common.h>

#include <connection.h>

#include <marshal.h>
/*
typedef struct tym_sync_app_configuration
{
    uint8 auto_power_off_cmd;
    uint8 auto_power_off_timer;
    uint8 enable_auto_wear;
    uint8 resversed;
    uint8 custom_ui[8];
}tym_sync_app_configuration_t;
 */
const marshal_member_descriptor_t tym_sync_app_configuration_member_descriptors[] = 
{
    MAKE_MARSHAL_MEMBER(tym_sync_app_configuration_t, uint8, auto_power_off_cmd),
    MAKE_MARSHAL_MEMBER(tym_sync_app_configuration_t, uint8, auto_power_off_timer),
    MAKE_MARSHAL_MEMBER(tym_sync_app_configuration_t, uint8, enable_auto_wear),
    MAKE_MARSHAL_MEMBER(tym_sync_app_configuration_t, uint8, enable_auto_play),
    MAKE_MARSHAL_MEMBER(tym_sync_app_configuration_t, uint8, ambient_ext_anc), 
    MAKE_MARSHAL_MEMBER(tym_sync_app_configuration_t, uint8, smartassistant),
    MAKE_MARSHAL_MEMBER_ARRAY(tym_sync_app_configuration_t, uint8, custom_ui, 8),
    MAKE_MARSHAL_MEMBER(tym_sync_app_configuration_t, uint8, wear_detect), 
    MAKE_MARSHAL_MEMBER(tym_sync_app_configuration_t, uint8, reserved),
};


const marshal_type_descriptor_t marshal_type_descriptor_tym_sync_data_t =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(sizeof(tym_sync_data_t));
const marshal_type_descriptor_t marshal_type_descriptor_tym_sync_app_configuration_t =
    MAKE_MARSHAL_TYPE_DEFINITION(tym_sync_app_configuration_t, tym_sync_app_configuration_member_descriptors);

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

/*! X-Macro generate key sync marshal type descriptor set that can be passed to a (un)marshaller
 *  to initialise it.
 *  */
#define EXPAND_AS_TYPE_DEFINITION(type) (const marshal_type_descriptor_t *)&marshal_type_descriptor_##type,
const marshal_type_descriptor_t * const tym_sync_marshal_type_descriptors[NUMBER_OF_MARSHAL_OBJECT_TYPES] = {
    MARSHAL_COMMON_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
};


