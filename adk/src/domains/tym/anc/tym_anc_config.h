/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       touch_config.h
\brief      Configuration related definitions for proximity sensor support.
*/

#ifndef __TYM_ANC_CONFIG_H__
#define __TYM_ANC_CONFIG_H__


#include "tym_anc.h"


/*! The touch sensor configuration */
extern ancConfig anc_config;

/*! Returns the proximity sensor configuration */
#define appConfigAnc() (&anc_config)

#endif /* __TOUCH_CONFIG_H__ */