/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       handover_profile.h
\defgroup   handover_profile Handover Profile
\ingroup    profiles
\brief      Handover Profile
*/

#ifndef HANDOVER_PROFILE_H_
#define HANDOVER_PROFILE_H_

#include <handover_if.h>
#include <domain_message.h>

/*! Messages that can be sent by handover profile to client tasks. */
typedef enum
{
    /*! Module initialisation complete */
    HANDOVER_PROFILE_INIT_CFM = HANDOVER_PROFILE_MESSAGE_BASE,

    /*! Handover Profile link to peer established. */
    HANDOVER_PROFILE_CONNECTION_IND,

    /*! Confirmation of a connection request. */
    HANDOVER_PROFILE_CONNECT_CFM,

    /*! Confirmation of a disconnect request. */
    HANDOVER_PROFILE_DISCONNECT_CFM,

    /*! Handover Profile link to peer removed. */
    HANDOVER_PROFILE_DISCONNECTION_IND,

    /*! Handover complete indication */
    HANDOVER_PROFILE_HANDOVER_COMPLETE_IND
}handover_profile_messages_t;

/*! Handover Profile status. */
typedef enum
{
    HANDOVER_PROFILE_STATUS_SUCCESS = 0,
    HANDOVER_PROFILE_STATUS_PEER_CONNECT_FAILED,
    HANDOVER_PROFILE_STATUS_PEER_CONNECT_CANCELLED,
    HANDOVER_PROFILE_STATUS_PEER_DISCONNECTED,
    HANDOVER_PROFILE_STATUS_PEER_LINKLOSS,
    HANDOVER_PROFILE_STATUS_HANDOVER_VETOED,
    HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT,
    HANDOVER_PROFILE_STATUS_HANDOVER_FAILURE
}handover_profile_status_t;

/*! brief Confirmation of the result of a connection request. */
typedef struct
{
    /*! Status of the connection request. */
    handover_profile_status_t status;
} HANDOVER_PROFILE_CONNECT_CFM_T;

/*! brief Confirmation of the result of a disconnect request. */
typedef HANDOVER_PROFILE_CONNECT_CFM_T HANDOVER_PROFILE_DISCONNECT_CFM_T;


#define HandoverProfile_Init(init_task) (FALSE)

#define HandoverProfile_ClientRegister(client_task) /* Nothing to do */

#define HandoverProfile_ClientUnregister(client_task) /* Nothing to do */

#define HandoverProfile_RegisterHandoverClients(clients, clients_size) (FALSE)

#define HandoverProfile_Connect(task, peer_addr) /* Nothing to do */

#define HandoverProfile_Disconnect(task) /* Nothing to do */

#define HandoverProfile_Handover(remote_addr) (HANDOVER_PROFILE_STATUS_HANDOVER_FAILURE)

#define HandoverProfile_HandleMdmSetBredrSlaveAddressInd(ind) /* Nothing to do */


#endif /*HANDOVER_PROFILE_H_*/

