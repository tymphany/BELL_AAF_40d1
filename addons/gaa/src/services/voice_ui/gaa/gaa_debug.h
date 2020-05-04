/*******************************************************************************
Copyright (c) 2018-2019 Qualcomm Technologies International, Ltd.

FILE NAME
    gaa_debug.h

DESCRIPTION
    Debug macros for GAA

NOTES

*/

#ifndef GAA_DEBUG_H_
#define GAA_DEBUG_H_


#include <panic.h>
#include <stdio.h>
#include <logging.h>

#include "gsound_target.h"

#define DEBUG_GAA(...)                DEBUG_LOG(__VA_ARGS__)
#define GAA_INFO(...)                 DEBUG_LOG(__VA_ARGS__)
#define GAA_PRINT(...)                printf(__VA_ARGS__)
#define GAA_DEBUG(x)                  DEBUG_LOG("GS:"##x##"")

/* GSOUND_TODO - DEVELOPER_BUILD needs to be DEVELOPER_BUILDx for release */
#define DEVELOPER_BUILDx
#define ENABLE_GAA_FUNC_TRACEx

#ifdef DEVELOPER_BUILD
#define GAA_DEBUG_ASSERT(condition)   if(!(condition)){GAA_DEBUG("ASSERT!!");Panic();}
#define GAA_VERIFY(expression)        GAA_DEBUG_ASSERT(expression)
#else
#define GAA_DEBUG_ASSERT(condition)   if(!(condition)){GAA_DEBUG("ASSERT!!");}
#define GAA_VERIFY(expression)        expression
#endif

#ifdef ENABLE_GAA_FUNC_TRACE
#define GAA_FUNC_TRACE(...)           GAA_PRINT(__VA_ARGS__)
#else
#define GAA_FUNC_TRACE(...)
#endif


#endif  /* GAA_DEBUG_H_ */
