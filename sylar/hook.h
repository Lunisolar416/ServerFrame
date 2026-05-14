#ifndef __SYLAR__HOOK_H__
#define __SYLAR__HOOK_H__

// Hook（钩子）本质上是：拦截并替换原有函数行为的技术。通过Hook，我们可以在不修改原有函数代码的情况下，改变函数的行为，或者在函数执行前后添加一些额外的逻辑。
// 在Linux系统中，Hook技术常用于调试、性能分析、安全监控等

// Socket相关函数的都是同步的，Hook技术可以将这些函数替换为异步的版本，从而实现非阻塞的网络编程。这对于高性能服务器开发非常有用，可以提高系统的吞吐量和响应速度。
#include <unistd.h>
namespace mysylar
{

bool is_hook_enable();

void set_hook_enable(bool flag);
} // namespace mysylar

extern "C"
{
    // sleep
    typedef unsigned int (*sleep_fun)(unsigned int seconds);
    extern sleep_fun sleep_f;

    typedef int (*usleep_fun)(useconds_t usec);
    extern usleep_fun usleep_f;
}

#endif //__SYLAR__HOOK_H__