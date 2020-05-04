/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Public GAA API
*/

#ifndef GAA_H_
#define GAA_H_

#include <message.h>

#if (defined(__QCC302X_APPS__) || defined(__QCC512X_APPS__)) && defined(INCLUDE_GAA) && !defined(INCLUDE_KYMERA_AEC)
    #error GAA needs the INCLUDE_KYMERA_AEC compilation switch for this platform
#endif

#if defined(INCLUDE_GAA) && defined(INCLUDE_AMA) && !defined(INCLUDE_VA_COEXIST)
    #error GAA and AMA are mutually exclusive unless VA coexistence feature is enabled
#endif

/*! \brief Initialise the GAA component
    \param[in] init_task - unused
    \return bool TRUE
 */
bool Gaa_Init(Task init_task);

#endif /* GAA_H_ */
