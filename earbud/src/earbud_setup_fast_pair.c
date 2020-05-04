/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Scrambled APSK.

*/

#include "earbud_setup_fast_pair.h"

#include "fast_pair.h"

#define SCRAMBLED_ASPK { 0xF82F ,0xA006, 0x4847, 0x2CC3, 0x4609, 0xDD7B, 0x6E67, 0x256C, 0x8FE0, 0x1BBC, 0x9220, 0xBE27, 0x5AD7,\
0x03F7, 0x1A98, 0x23F3}

#ifdef INCLUDE_FAST_PAIR
static const uint16 scrambled_apsk[] = SCRAMBLED_ASPK;
#endif

void Earbud_SetupFastPair(void)
{
#ifdef INCLUDE_FAST_PAIR
    FastPair_SetPrivateKey(scrambled_apsk, sizeof(scrambled_apsk));
#endif
}
