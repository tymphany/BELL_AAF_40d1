/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       a2dp_profile_config.h
\brief      Configuration related definitions for A2DP state machine.
*/

#ifndef A2DP_PROFILE_CONFIG_H_
#define A2DP_PROFILE_CONFIG_H_


/*! Time to wait to connect A2DP media channel after a remotely initiated A2DP connection */
#define appConfigA2dpMediaConnectDelayAfterLocalA2dpConnectMs() D_SEC(3)

/*! This config relates to the behavior of the TWS standard master when AAC sink.
    If TRUE, it will forward the received stereo AAC data to the slave earbud.
    If FALSE, it will transcode one channel to SBC mono and forward. */
#define appConfigAacStereoForwarding() TRUE

#ifdef ENABLE_TYM_PLATFORM
/*! Default volume gain in dB */
/* (-13 + 45) * 127 / 42 =  96.76 .... for 12/16..96/127*/
#define appConfigDefaultVolumedB() (-13)
#else
/*! Default volume gain in dB */
#define appConfigDefaultVolumedB() (-10)
#endif
/*! Delay to use when we want to send a A2DP media start response but the
    A2DP audio chain is not ready yet. */
#define appConfigA2dpSyncSendMediaStartResponseDelayMs() (5)

#endif /* A2DP_PROFILE_CONFIG_H_ */
