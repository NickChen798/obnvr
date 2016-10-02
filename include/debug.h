
#ifndef __OB_DEBUG_H__
#define __OB_DEBUG_H__

#include "ob_platform.h"

#ifndef LOG_TAG
#define LOG_TAG     " "
#endif

#ifndef LOG_ERROR
#define LOG_ERROR   1
#endif

#ifndef LOG_DEBUG
#define LOG_DEBUG   0
#endif

#ifndef LOG_INFO
#define LOG_INFO    0
#endif

#ifndef LOG_TRACE
#define LOG_TRACE   0
#endif

#define ob_output(format,...)   do {        \
    struct timeval di_tt;                   \
    gettimeofday(&di_tt, 0);                \
    dbg("[%u.%06u][%s]" format, (u32)di_tt.tv_sec, (u32)di_tt.tv_usec, LOG_TAG, ##__VA_ARGS__);  \
} while(0)
#define ob_output2(format,...)   do {        \
    struct timeval di_tt;                   \
    gettimeofday(&di_tt, 0);                \
    dbg("[%u.%06u][%s]%s(%d):" format, (u32)di_tt.tv_sec, (u32)di_tt.tv_usec, LOG_TAG, __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
} while(0)
#define ob_dummy(format,...) do; while(0)

#if LOG_ERROR > 0
#define ob_error    ob_output2
#else
#define ob_error    ob_dummy
#endif

#if LOG_INFO > 0
#define ob_info     ob_output
#else
#define ob_info     ob_dummy
#endif

#if LOG_DEBUG > 0
#define ob_debug    ob_output
#else
#define ob_debug    ob_dummy
#endif

#if LOG_TRACE > 0
#define ob_trace    ob_output
#else
#define ob_trace    ob_dummy
#endif

#define ob_assert(state, format, ...) do {  \
            if(!(state)) {                  \
                ob_error(format, ...);      \
                exit(-1);                   \
            }                               \
        } while(0)

#endif // __OB_DEBUG_H__

