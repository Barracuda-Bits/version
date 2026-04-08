#ifndef __LOGGING_H__
#define __LOGGING_H__

// EXTERNAL INCLUDES
#include <stdarg.h>
#include <stdio.h>
// INTERNAL INCLUDES

static inline void LOG(
    const char* fmt,
    ...
)
{
    extern bool verbose;

    if (!verbose)
		return;

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

#endif // __LOGGING_H__
