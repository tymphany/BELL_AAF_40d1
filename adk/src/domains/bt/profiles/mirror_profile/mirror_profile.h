/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Public interface to mirror ACL & eSCO connections.

This module manages the mirror connections used in TWM devices.

It is responsible for:

* Creating and managing the mirror ACL / eSCO / A2DP connection., e.g.
    dealing with link-loss of the mirror connection(s).
* Only actively creating mirror links when a peer earbud and a handset
    are connected.
* Acting in either a Primary or Secondary role. e.g. only the Primary can
    actively connect or disconnect a mirror connection.
* Interfacing with the voice/audio sources which will create the necessary SCO or
    A2DP audio chains to render a mirrored eSCO or A2DP link.
* Notifying registered modules about the state of the mirror links.
* Taking part in the handover process by:
  * Being able to veto it.
  * Being able to switch roles from Primary -> Secondary and vice-versa.
  * Making the mirror eSCO or A2DP Sink available to the handover process so
    it can be moved into a 'real' HFP or A2DP profile instance and vice-versa.

In a mirror ACL / eSCO / A2DP connection there is always a Primary device
and a Secondary device. This module puts all the responsibility for connecting
/ disconnecting and managing the lifetime of the connection on the Primary
device.

As an example, if there is a link-loss scenario both Primary & Secondary will
receive a MDM_LINK_DISCONNECT_IND with a link-loss reason. But then the
behaviour is different:

Primary:
* Actively try to re-connect the mirror connection(s)

Secondary:
* Do nothing until it receives notification of a successful mirror connection.

The mirror_profile module listens to notifications from other bluetooth profile
modules in order to know when to create the mirror connection(s).

Both mirror eSCO (HFP) and A2DP connections first require a mirror ACL
connection to be created.

The mirror eSCO connection is created when the handset HFP SCO channel is
created. Usually this will be when a call is incoming or outgoing.

The mirror A2DP is created when the A2DP media starts to stream.

Dependencies
To create a mirror ACL connection there must be:
* An ACL connection to the Handset.
* An ACL connection to the peer earbud.

To create a mirror eSCO connection there must be:
* A mirror ACL connection.
* A HFP SCO connection to the Handset.

To create a mirror A2DP connection there must be:
* A mirror ACL connection
* An A2DP media connection to the Handset (the module only mirrors the A2DP media
  channel L2CAP channel).

The handset must be master of the ACL connection to the primary earbud and the
primary earbud must be master of the ACL connection to the secondary earbud.
The link policy module and topology set the correct ACL connection roles.

The Bluetooth firmware automatically disconnects mirror links when the
dependencies (above) are not met, e.g. on link loss. Therefore the mirror profile
does not initiate mirror link disconnection.

*/

#ifndef MIRROR_PROFILE_H_
#define MIRROR_PROFILE_H_

#include <message.h>

#include "connection_manager.h"
#include "domain_message.h"
#include "voice_sources.h"

/*! \brief Events sent by mirror_profile to other modules. */
typedef enum
{
    /*! Module initialisation complete */
    MIRROR_PROFILE_INIT_CFM = MIRROR_PROFILE_MESSAGE_BASE,

    /*! Mirror ACL connect indication */
    MIRROR_PROFILE_CONNECT_IND,

    /*! Confirmation of a connection request. */
    MIRROR_PROFILE_CONNECT_CFM,

    /*! Confirmation of a connection request. */
    MIRROR_PROFILE_DISCONNECT_CFM,

    /*! Mirror ACL disconnect indication */
    MIRROR_PROFILE_DISCONNECT_IND,

    /*! Mirror eSCO connect indication */
    MIRROR_PROFILE_ESCO_CONNECT_IND,

    /*! Mirror eSCO disconnect indication */
    MIRROR_PROFILE_ESCO_DISCONNECT_IND,

    /*! Mirror A2DP stream is active indication */
    MIRROR_PROFILE_A2DP_STREAM_ACTIVE_IND,

    /*! Mirror A2DP stream is inactive indication */
    MIRROR_PROFILE_A2DP_STREAM_INACTIVE_IND,

} mirror_profile_msg_t;

/*! \brief Status codes used by mirror_profile */
typedef enum
{
    /*! Mirror profile got connected to peer */
    mirror_profile_status_peer_connected = 0,

    /*! Unable to connect Mirror profile with Peer */
    mirror_profile_status_peer_connect_failed,

    /*! Mirror profile peer connect is cancelled */
    mirror_profile_status_peer_connect_cancelled,

    /*! Mirror profile disconnected */
    mirror_profile_status_peer_disconnected
} mirror_profile_status_t;

/*! \brief The way in which the A2DP media should be started. */
typedef enum
{
    /*! The audio should start without requiring synchronisation with the secondary
        earbud. This mode is reported on the primary earbud. This mode is used
        when the primary earbud is not connected to the secondary earbud when
        audio is started.
    */
    MIRROR_PROFILE_A2DP_START_PRIMARY_UNSYNCHRONISED,

    /*! The audio should start synchronised at primary and secondary. This mode
        is reported on the primary earbud. It is used when the primary and
        secondary earbuds are connected when audio is started.
    */
    MIRROR_PROFILE_A2DP_START_PRIMARY_SYNCHRONISED,

    /*! The audio should start synchronised at primary and secondary. This mode
        is reported on the secondary earbud. It is used when the primary and
        secondary earbuds are connected when audio is started.
    */
    MIRROR_PROFILE_A2DP_START_SECONDARY_SYNCHRONISED,

    /*! The audio should start at secondary synchronised with primary. This mode
        is reported on the secondary earbud. It is used when the secondary connects
        to a primary with an already active audio stream.
    */
    MIRROR_PROFILE_A2DP_START_SECONDARY_JOINS_SYNCHRONISED,

    MIRROR_PROFILE_A2DP_START_Q2Q_MODE,

} mirror_profile_a2dp_start_mode_t;

/*! \brief Confirmation of the result of a connection request. */
typedef struct
{
    /*! Status of the connection request. */
    mirror_profile_status_t status;
} MIRROR_PROFILE_CONNECT_CFM_T;

/*! \brief Confirmation of the result of a disconnection request. */
typedef struct
{
    /*! Status of the disconnection request. */
    mirror_profile_status_t status;
} MIRROR_PROFILE_DISCONNECT_CFM_T;


/*! \brief Mirror ACL connect indication. */
typedef CON_MANAGER_TP_CONNECT_IND_T MIRROR_PROFILE_CONNECT_IND_T;

/*! \brief Mirror ACL disconnect indication. */
typedef CON_MANAGER_TP_DISCONNECT_IND_T MIRROR_PROFILE_DISCONNECT_IND_T;

/*! \brief Mirror ACL connect indication. */
typedef CON_MANAGER_TP_CONNECT_IND_T MIRROR_PROFILE_ESCO_CONNECT_IND_T;

/*! \brief Mirror ACL disconnect indication. */
typedef CON_MANAGER_TP_DISCONNECT_IND_T MIRROR_PROFILE_ESCO_DISCONNECT_IND_T;


#define MirrorProfile_IsConnected() (FALSE)

#define MirrorProfile_IsEscoActive() (FALSE)

#define MirrorProfile_IsA2dpActive() (FALSE)

#define MirrorProfile_ClientRegister(task) UNUSED(task)

#define MirrorProfile_ClientUnregister(task) UNUSED(task)

#define MirrorProfile_Connect(task, peer_addr) /* Nothing to do */

#define MirrorProfile_Disconnect(task) /* Nothing to do */

#define MirrorProfile_SetRole(primary) UNUSED(primary)

#define MirrorProfile_GetScoSink() ((Sink)NULL)

#define MirrorProfile_GetMirrorAclHandle() ((uint16)0xFFFF)

#define MirrorProfile_HandleConnectionLibraryMessages(id, message, already_handled) (already_handled)

#define MirrorProfile_GetA2dpStartMode() MIRROR_PROFILE_A2DP_START_PRIMARY_UNSYNCHRONISED

#define MirrorProfile_GetA2dpAudioSyncTransportSink() ((Sink)NULL)

#define MirrorProfile_GetA2dpAudioSyncTransportSource() ((Source)NULL)

#define MirrorProfile_EnableMirrorEsco() /* Nothing to do */

#define MirrorProfile_DisableMirrorEsco() /* Nothing to do */

#define MirrorProfile_GetExpectedPeerLinkTransmissionTime() ((uint32) 0x0)

#define MirrorProfile_EnableMirrorA2dp(void) /* Nothing to do */

#define MirrorProfile_DisableMirrorA2dp(void)  /* Nothing to do */


#endif /* MIRROR_PROFILE_H_ */
