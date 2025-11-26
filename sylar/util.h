#ifndef __SYLAR_UTIL_H__
#define __SYLAR_UTIL_H__

#include <cstdint>
#include <sched.h>
namespace mysylar
{
pid_t getThreadId();
uint32_t getFiberId();

} // namespace mysylar

#endif // __SYLAR_UTIL_H__