/*!
\copyright  Copyright (c) 2005 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Definition of messages that can be sent between key sync components.
*/

#ifndef __EARBUD_TYM_SYNC_MARSHAL_DEFS_H__
#define __EARBUD_TYM_SYNC_MARSHAL_DEFS_H__

#include <marshal_common.h>

#include <connection.h>

#include <marshal.h>

#include "earbud_tym_sync_defs.h"

/* Create base list of marshal types the key sync will use. */
#define MARSHAL_TYPES_TABLE(ENTRY) \
    ENTRY(tym_sync_data_t) \
    ENTRY(tym_sync_app_configuration_t)

/* X-Macro generate enumeration of all marshal types */
#define EXPAND_AS_ENUMERATION(type) MARSHAL_TYPE(type),
enum MARSHAL_TYPES
{
    /* common types must be placed at the start of the enum */
    DUMMY = NUMBER_OF_COMMON_MARSHAL_OBJECT_TYPES-1,
    /* now expand the marshal types specific to this component. */
    MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    NUMBER_OF_MARSHAL_OBJECT_TYPES
};
#undef EXPAND_AS_ENUMERATION

/* Make the array of all message marshal descriptors available. */
extern const marshal_type_descriptor_t * const tym_sync_marshal_type_descriptors[];

#endif /* __EARBUD_TYM_SYNC_MARSHAL_DEFS_H__ */
