#ifndef __SYLAR_UTIL_H__
#define __SYLAR_UTIL_H__

#include <cstdint>
#include <cxxabi.h>
#include <execinfo.h>
#include <pthread.h>
#include <sched.h>
#include <string>
#include <sys/syscall.h>
#include <vector>
namespace mysylar
{
pid_t GetThreadId();
uint32_t GetFiberId();
void Backtrace(std::vector<std::string>& bt, int size, int skip = 1);
std::string BacktraceToString(int size, int skip = 2, const std::string& prefix = "");
} // namespace mysylar
template <class T>
const char* TypeToName()
{
    static const char* s_name = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
    return s_name;
}
#endif // __SYLAR_UTIL_H__