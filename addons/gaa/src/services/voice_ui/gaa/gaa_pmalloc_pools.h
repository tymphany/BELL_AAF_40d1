/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Creation of pmalloc pools used in GAA configs only.
*/

#ifndef GAA_PMALLOC_POOLS_H_
#define GAA_PMALLOC_POOLS_H_

/*
 * The size of the buffer that is allocated by GSoundTargetOtaBegin to handle
 * GSoundTargetOtaData. Largest mallocDebugNoPanic buffer size available.
 */
#define GAA_OTA_DATA_BUFFER_SIZE   692

/*! \brief Extend pmalloc pools as needed for the GAA feature.
*/
void Gaa_InitPmallocPools(void);

#endif  /* GAA_PMALLOC_POOLS_H_ */
