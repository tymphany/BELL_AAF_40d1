/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       earbud_config.h
\brief      Application configuration file
*/

#ifndef EARBUD_CONFIG_H_
#define EARBUD_CONFIG_H_

#include <earbud_init.h>

/*! @{ */
/*! Left and Right device selection.

Only devices that have fixed Left and Right roles are supported. This can be 
selected using the Bluetooth Device Address, see #USE_BDADDR_FOR_LEFT_RIGHT,
or by using an external input.

If an alternative mechanism is used for Left / Right, then the  macros used 
here should be replaced (possibly by functions) that return the correct
role selection. */

#ifdef USE_BDADDR_FOR_LEFT_RIGHT
/* Left and right earbud roles are selected from Bluetooth address. */
/*! TRUE if this is the left earbud (Bluetooth address LAP is odd). */
#define appConfigIsLeft()           (InitGetTaskData()->appInitIsLeft)
/*! TRUE if this is the right earbud (Bluetooth address LAP is even). */
#define appConfigIsRight()          (appConfigIsLeft() ^ 1)
#else
#ifdef ENABLE_TYM_PLATFORM

/*! Left and right earbud roles are selected from the state of this PIO */
#define appConfigHandednessPio()    (66)
/*! TRUE if this is the left earbud (the #appConfigHandednessPio state is 0) */
#define appConfigIsLeft()           ((PioGet32Bank(appConfigHandednessPio() / 32) & (1UL << (appConfigHandednessPio()%32))) ? 0 :1)
/*! TRUE if this is the right earbud (the #appConfigHandednessPio state is 0) */
#define appConfigIsRight()          (appConfigIsLeft() ^ 1)
#else/*ENABLE_TYM_PLATFORM*/
/*! Left and right earbud roles are selected from the state of this PIO */
#define appConfigHandednessPio()    (2)
/*! TRUE if this is the left earbud (the #appConfigHandednessPio state is 1) */
#define appConfigIsLeft()           ((PioGet32Bank(appConfigHandednessPio() / 32) & (1UL << appConfigHandednessPio())) ? 1 : 0)
/*! TRUE if this is the right earbud (the #appConfigHandednessPio state is 0) */
#define appConfigIsRight()          (appConfigIsLeft() ^ 1)
#endif /*ENABLE_TYM_PLATFORM*/
#endif

/*! @} */

/*! Default state proxy events to register */
#define appConfigStateProxyRegisteredEventsDefault()            \
                   (state_proxy_event_type_phystate |           \
                    state_proxy_event_type_a2dp_conn |          \
                    state_proxy_event_type_a2dp_discon |        \
                    state_proxy_event_type_avrcp_conn |         \
                    state_proxy_event_type_avrcp_discon |       \
                    state_proxy_event_type_hfp_conn |           \
                    state_proxy_event_type_hfp_discon |         \
                    state_proxy_event_type_a2dp_supported |     \
                    state_proxy_event_type_avrcp_supported |    \
                    state_proxy_event_type_hfp_supported |      \
                    state_proxy_event_type_peer_linkloss |      \
                    state_proxy_event_type_handset_linkloss)

/*! User will need to change this as per the hardware used. 0xEC (for CF376+ QCC5126) */
#ifdef ENABLE_TYM_PLATFORM
#define BOARD_TX_POWER_PATH_LOSS        (0xEC) /*EC(-20), 4 - 20 = -16 */
#else
#define BOARD_TX_POWER_PATH_LOSS        (0xEC) /* default -20 */
#endif
#endif /* EARBUD_CONFIG_H_ */
