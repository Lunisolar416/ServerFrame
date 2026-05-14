
#include "hook.h"
#include "fiber.h"
#include "iomanager.h"
#include <ctime>
#include <dlfcn.h>
#include <sys/types.h>

namespace mysylar
{

static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX)                                                                               \
    XX(sleep)                                                                                      \
    XX(usleep)                                                                                     \
    XX(nanosleep)                                                                                  \
    XX(socket)                                                                                     \
    XX(connect)                                                                                    \
    XX(accept)                                                                                     \
    XX(read)                                                                                       \
    XX(readv)                                                                                      \
    XX(recv)                                                                                       \
    XX(recvfrom)                                                                                   \
    XX(recvmsg)                                                                                    \
    XX(write)                                                                                      \
    XX(writev)                                                                                     \
    XX(send)                                                                                       \
    XX(sendto)                                                                                     \
    XX(sendmsg)                                                                                    \
    XX(close)                                                                                      \
    XX(fcntl)                                                                                      \
    XX(ioctl)                                                                                      \
    XX(getsockopt)                                                                                 \
    XX(setsockopt)

void hook_init()
{
    static bool is_inited = false;
    if (is_inited)
        return;

// 使用dlsym函数获取原始函数的地址，并将其赋值给对应的函数指针变量
#define XX(name) name##_f = (name##_fun) dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX);
#undef XX
}

struct _HookIniter
{
    _HookIniter()
    {
        hook_init();
    }
};

// 全局静态变量，在进入main函数之前就会被初始化，调用hook_init函数，完成函数指针的初始化
static _HookIniter s_hook_initer;

bool is_hook_enable()
{
    return t_hook_enable;
}

void set_hook_enable(bool flag)
{
    t_hook_enable = flag;
}
} // namespace mysylar

extern "C"
{
#define XX(name) name##_fun name##_f = nullptr;
    HOOK_FUN(XX);
#undef XX

    // sleep函数的hook版本
    unsigned int sleep(unsigned int seconds)
    {
        if (!mysylar::t_hook_enable)
        {
            // 如果hook没有启用，直接调用原始的sleep函数
            return sleep_f(seconds);
        }
        mysylar::Fiber::ptr fiber = mysylar::Fiber::GetThis();
        mysylar::IOManager* iom = mysylar::IOManager::GetThis();
        iom->addTimer(seconds * 1000, [iom, fiber]() { iom->schedule(fiber, -1); });
        mysylar::Fiber::YieldToHold();
        return 0;
    }
    int usleep(useconds_t usec)
    {
        if (!mysylar::t_hook_enable)
        {
            // 如果hook没有启用，直接调用原始的sleep函数
            return usleep_f(usec);
        }
        mysylar::Fiber::ptr fiber = mysylar::Fiber::GetThis();
        mysylar::IOManager* iom = mysylar::IOManager::GetThis();
        iom->addTimer(usec, [iom, fiber]() { iom->schedule(fiber, -1); });
        mysylar::Fiber::YieldToHold();
        return 0;
    }
    int nanosleep(const struct timespec* req, struct timespec* rem)
    {
        if (!mysylar::t_hook_enable)
        {
            // 如果hook没有启用，直接调用原始的sleep函数
            return nanosleep(req, rem);
        }
        int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
        mysylar::Fiber::ptr fiber = mysylar::Fiber::GetThis();
        mysylar::IOManager* iom = mysylar::IOManager::GetThis();
        iom->addTimer(timeout_ms, [iom, fiber]() { iom->schedule(fiber, -1); });
        mysylar::Fiber::YieldToHold();
        return 0;
    }
}
