/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       pairing_config.h
\brief      Configuration related definitions for the pairing task.
*/

#ifndef PAIRING_CONFIG_H_
#define PAIRING_CONFIG_H_


#ifdef CF133_BATT
/*! Minimum RSSI to pair with a device */
#define appConfigPeerPairingMinRssi() (-60)
#else
#ifdef ENABLE_TYM_PLATFORM /*for reduced LE TX Power,change from -50 to -65 */
/*! Minimum RSSI to pair with a device */
#define appConfigPeerPairingMinRssi() (-65)
#else
/*! Minimum RSSI to pair with a device */
#define appConfigPeerPairingMinRssi() (-50)
#endif
#endif

/*! Minimum difference in RSSI between devices discovered
    with the highest RSSI in order to pair */
#define appConfigPeerPairingMinRssiDelta() (10)

/*! Timeout in seconds for user initiated peer pairing */
#define appConfigPeerPairingTimeout()       (120)

#ifdef ENABLE_TYM_PLATFORM
/*! Timeout in seconds for user initiated handset pairing */
#define appConfigHandsetPairingTimeout()    (600)
#else
/*! Timeout in seconds for user initiated handset pairing */
#define appConfigHandsetPairingTimeout()    (120)
#endif
/*! Timeout in seconds to disable page/inquiry scan after entering idle state */
#define appConfigPairingScanDisableDelay()  (5)

/*! Timeout in seconds for automatic peer pairing */
#define appConfigAutoPeerPairingTimeout()       (0)

/*! Timeout in seconds for authentication */
#define appConfigAuthenticationTimeout()       (90)

#ifdef ENABLE_TYM_PLATFORM
/*! Timeout in seconds for automatic handset pairing */
#define appConfigAutoHandsetPairingTimeout()    (600)
#else
/*! Timeout in seconds for automatic handset pairing */
#define appConfigAutoHandsetPairingTimeout()    (300)
#endif
/*! Key ID peer Earbud link-key derivation */
#define appConfigTwsKeyId()       (0x74777332UL)

#endif /* PAIRING_CONFIG_H_ */
