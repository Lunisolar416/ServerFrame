#ifndef __SYLAR_MACRO_H__
#define __SYLAR_MACRO_H__

#include <assert.h>
#include <string.h>

#include "util.h"

/// 断言宏封装
#define SYLAR_ASSERT(x)                                                                            \
    if (!(x))                                                                                      \
    {                                                                                              \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x << "\nbacktrace:\n"                  \
                                          << mysylar::BacktraceToString(100, 2, "    ");           \
        assert(x);                                                                                 \
    }

/// 断言宏封装
#define SYLAR_ASSERT2(x, w)                                                                        \
    if (!(x))                                                                                      \
    {                                                                                              \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x << "\n"                              \
                                          << w << "\nbacktrace:\n"                                 \
                                          << mysylar::BacktraceToString(100, 2, "    ");           \
        assert(x);                                                                                 \
    }
#endif // __SYLAR_MACRO_H__