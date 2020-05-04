/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Link policy manager for TWM builds.

When mirroring, the topology is fixed and role switches are not desirable
for any device in the topology.

The primary earbud is master of the link to the secondary earbud, and the roles
only switch when the devices perform handover. These roles are enforced by a call
to #appLinkPolicyAlwaysMaster in tws topology.

The handset is master of the link to the primary earbud. Mirroring is currently
only supported when the primary earbud is ACL slave. Since being ACL master is
preferable (from link scheduling point of view) compared to being slave, there
is no reason for the primary earbud to request a role switch (once slave) and no
good reason for the handset to request a role switch to become slave. Therefore,
calls to allow/prohibit or change roles (that were valid in non-mirroring
topology) are ignored here.
*/

#include "link_policy.h"
#include "logging.h"
#include "bdaddr.h"
#include "bt_device.h"
#include "app/bluestack/dm_prim.h"
#include "panic.h"

