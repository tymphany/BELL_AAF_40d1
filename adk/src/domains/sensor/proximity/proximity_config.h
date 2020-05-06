/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       proximity_config.h
\brief      Configuration related definitions for proximity sensor support.
*/

#ifndef PROXIMITY_CONFIG_H_
#define PROXIMITY_CONFIG_H_


#include "proximity.h"

#ifdef ENABLE_TYM_PLATFORM
/*! The proximity sensor configuration */
extern proximityConfig proximity_config;
#else
/*! The proximity sensor configuration */
extern const proximityConfig proximity_config;
#endif
/*! Returns the proximity sensor configuration */
#define appConfigProximity() (&proximity_config)

#endif /* PROXIMITY_CONFIG_H_ */
