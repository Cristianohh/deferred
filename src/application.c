/*! @file application.c
 *  @author Kyle Weicht
 *  @date 9/13/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */
#include "application.h"

#include <stdio.h>
#include <stdarg.h>
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
#endif

/*
 * Internal 
 */

/*
 * External
 */
void debug_output(const char* format, ...)
{
    va_list ap;
    char buffer[1024];
    va_start(ap, format);
    vsprintf(buffer, format, ap);
    va_end(ap);
#ifdef _WIN32
    OutputDebugString(buffer);
#elif defined(__APPLE__)
    printf("%s", buffer);
#endif
}
