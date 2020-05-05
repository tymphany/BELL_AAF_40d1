/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       touch_config.h
\brief      Configuration related definitions for proximity sensor support.
*/

#ifndef __TYM_TOUCH_CONFIG_H__
#define __TYM_TOUCH_CONFIG_H__


#include "tym_touch.h"


/*! The touch sensor configuration */
extern touchConfig touch_config;

/*! Returns the proximity sensor configuration */
#define appConfigTouch() (&touch_config)

#endif /* __TOUCH_CONFIG_H__ */
