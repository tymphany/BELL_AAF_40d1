/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation for GAA trace.Refer to gsound_target_trace.h for API documentation
*/


#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "gsound_target.h"
#include "gaa_debug.h"

void GSoundTargetTraceInit(void)
{
    /* void return */
}

void GSoundTargetTrace(const char *filename, int lineno, const char *fmt, ...)
{
    /* Define some parameters as UNUSED in case GAA_FUNC_TRACE is a null macro. */
    UNUSED(filename);
    UNUSED(lineno);
    GAA_FUNC_TRACE("GS:TRACE: %s %d", filename, lineno);
    if (fmt != NULL)
    {
#ifdef ENABLE_GAA_FUNC_TRACE
        /* Initially use a dummy buffer to determine the size to allocate. */
        char dummy;
        int result;
        va_list va_parameters;

        va_start(va_parameters, fmt);
        result = vsnprintf(&dummy, 1, fmt, va_parameters);
        va_end(va_parameters);
        if (result <= 0)
        {
            /*
             * A negative result indicates an error. Do nothing.
             * A result of zero indicates an empty string. Do nothing.
             */
        }
        else
        {
            /*
             * The return value from vsnprintf is the number of characters
             * (excluding the terminating null byte) which would have been
             * written to the final string if enough space had been available.
             * Use this value to allocate a buffer of the required size,
             * adding one for the terminating null character.
             */
            char *buffer = (char *) malloc((size_t) ++result);
            if (buffer != NULL)
            {
                va_start(va_parameters, fmt);
                result = vsnprintf(buffer, (size_t) result, fmt, va_parameters);
                va_end(va_parameters);
                if (result > 0)
                {
                    GAA_FUNC_TRACE("%s", buffer);
                }
                free(buffer);
            }
        }
#endif /* ENABLE_GAA_FUNC_TRACE */
    }
    GAA_FUNC_TRACE("\n");
}

void GSoundTargetTraceAssert(const char *filename, int lineno, const char *condition)
{
    UNUSED(condition);
    GAA_PRINT("GS:ASSERT: %s %d %s", filename, lineno, condition);
    GAA_DEBUG_ASSERT(0);
}
