/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Scrambled APSK.

*/

#include "earbud_setup_fast_pair.h"

#include "fast_pair.h"
#ifdef ENABLE_TYM_PLATFORM /*use new dfu key product scramble aspk*/
#define SCRAMBLED_ASPK { 0x6E3D, 0x56A0, 0x5D0F, 0x119C, 0x8829, 0x0877, 0xFD99, 0x2BDB, 0x39DF, 0x5418, 0x5B52, 0x5809, 0x3766,\
0x2918, 0xD613,0x4218}
#else
#define SCRAMBLED_ASPK {0x55d4, 0xd417, 0x32da, 0x81bb, 0xbde7, 0xe2ee, 0x8a44, 0x6fd3, 0xa181, 0xda60, 0xb9b8, 0x7b16, 0x445b,\
0x7c3c, 0xb224, 0x0c35}
#endif
#ifdef INCLUDE_FAST_PAIR
static const uint16 scrambled_apsk[] = SCRAMBLED_ASPK;
#endif

void Earbud_SetupFastPair(void)
{
#ifdef INCLUDE_FAST_PAIR
    FastPair_SetPrivateKey(scrambled_apsk, sizeof(scrambled_apsk));
#endif
}
