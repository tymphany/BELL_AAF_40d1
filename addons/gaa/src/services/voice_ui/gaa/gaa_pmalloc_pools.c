/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Creation of pmalloc pools used in GAA configs only
*/

#include "gaa_pmalloc_pools.h"
#include <pmalloc.h>

/*
 *  Buffers needed for upgrade as the upgrade\CONFIG_HYDRACORE\secrsa_padding.c module's
 *  ce_pkcs1_pss_padding_verify function fails to malloc for a GAA build without them,
 *  causing verification of a DFU file (received by any transport from any host application) to fail.
 */
#define GAA_OTA_VERIFICATION_BUFFER_SIZE       288
#define GAA_OTA_NUMBER_OF_VERIFICATION_BUFFERS   0

_Pragma ("unitsuppress Unused")
_Pragma("datasection apppool") static const pmalloc_pool_config app_pools[] =
{
    {GAA_OTA_VERIFICATION_BUFFER_SIZE, GAA_OTA_NUMBER_OF_VERIFICATION_BUFFERS},
    {GAA_OTA_DATA_BUFFER_SIZE, 1}
};


void Gaa_InitPmallocPools(void)
{
    /*
     * The linker will discard any files in an archive library that do not contain
     * an externally-referenced symbol. This function is used to guarantee that the
     * architecture-specific app_pools structure is included in the final executable
     * when needed.
     */
}
