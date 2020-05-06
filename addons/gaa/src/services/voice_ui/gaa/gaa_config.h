/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Configuration related definitions for GAA support.
*/

#ifndef GAA_CONFIG_H_
#define GAA_CONFIG_H_

#ifdef ENABLE_TYM_PLATFORM
#define gaa_TargetModelId()   ((0x37 << 16) | 0x27DB)
/*! Get GAA Model Identifier for our target platform
*/
#else /*Orig */
/*! Get GAA Model Identifier for our target platform 
*/
#define gaa_TargetModelId() ((0xF0 << 16) | 0x0100)
#endif

/*! Get GAA Enabled Flag 
*/
#define gaa_Enabled() (TRUE)

/*! Get GAA Action Mapping
*/
#ifdef ENABLE_TYM_PLATFORM
#define gaa_ActionMapping() (1)  /* "Dedicated assistant physical button (three button)" */
#else
#define gaa_ActionMapping() (0)  /* "Dedicated assistant physical button (one button)" */ 
#endif
/*! Get GAA TWS uild config 
*/
#define gaa_IsTwsBuild() (TRUE)

#endif  /* GAA_CONFIG_H_ */
