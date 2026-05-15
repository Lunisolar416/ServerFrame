
#include "hook.h"
#include "fd_manager.h"
#include "fiber.h"
#include "iomanager.h"
#include "log.h"
#include "scheduler.h"
#include <cerrno>
#include <ctime>
#include <dlfcn.h>
#include <functional>
#include <sys/types.h>
static mysylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
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
// timer_info 用来标记“这个 IO 等待是正常完成还是被超时/取消中断
struct timer_info
{
    int cancelled = 0;
};

/*enum class IoState
{
    TRY_IO, // 直接尝试 IO 操作
    WAIT,   // 等待事件发生或超时
    RETRY,  // 事件发生后，重新尝试 IO 操作
    DONE    // IO 操作完成
};
template <typename OriginFun, typename... Args>
static ssize_t do_io_state(int fd, OriginFun fun, const char* hook_fun_name, uint32_t event,
                           int timeout_so, ssize_t buflen, Args&&... args)
{
    if (!mysylar::t_hook_enable)
    {
        // 不是hook版本，直接调用原始函数
        return fun(fd, std::forward<Args>(args)...);
    }
    mysylar::FdCtx::ptr ctx = mysylar::FdMgr::GetInstance()->get(fd);
    if (!ctx)
    {
        return fun(fd, std::forward<Args>(args)...);
    }
    if (ctx->isClose())
    {
        errno = EBADF; // EBADF(Bad File Descriptor)文件描述符错误,意思是这个 fd 已经关闭/无效
        return -1;
    }
    // 如果不是socket或者用户设置了非阻塞，就直接调用原始函数
    if (!ctx->isSocket() || ctx->getUserNonblock())
    {
        return fun(fd, std::forward<Args>(args)...);
    }

    // to timeout
    uint64_t to = ctx->getTimeout(timeout_so);
    std::shared_ptr<timer_info> tinfo(new timer_info);

    IoState state = IoState::TRY_IO;
    ssize_t n = -1;
    mysylar::Timer::ptr timer;
    while (state != IoState::DONE)
    {
        switch (state)
        {
        case IoState::TRY_IO:
        {
            n = fun(fd, std::forward<Args>(args)...);
            while (n == -1 && errno == EINTR)
                n = fun(fd, std::forward<Args>(args)...);
            while (n == -1 && errno == EAGAIN)
            {
                state = IoState::WAIT;
                break;
            }
        }
        case IoState::WAIT:
        {
            mysylar::IOManager* iom = mysylar::IOManager::GetThis();

            std::weak_ptr<timer_info> winfo(tinfo);
            if (to != (uint64_t) -1)
            {
                // 添加一个条件定时器，超时后会执行回调函数，回调函数会设置 timer_info 的 cancelled
                // 字段，并取消事件监听，唤醒协程
                timer = iom->addConditionTimer(
                    to,
                    [winfo, fd, iom, event]()
                    {
                        auto t = winfo.lock();
                        // 避免重复处理或访问已经失效的 timer 状态
                        if (!t || t->cancelled)
                            return;
                        t->cancelled = ETIMEDOUT; // ETIMEDOUT (Connection timed out)连接超时
                        // 超时了，取消事件监听，唤醒协程
                        iom->cancelEvent(fd, (mysylar::IOManager::Event)(event));
                    },
                    winfo);
            }
            int rt = iom->addEvent(fd, (mysylar::IOManager::Event)(event));
            if (rt)
            {
                SYLAR_LOG_ERROR(g_logger)
                    << hook_fun_name << " addEvent(" << fd << ", " << event << ")";
                if (timer)
                {
                    timer->cancel();
                }
                return -1;
            }
            else
            {
                // 挂起协程，等待事件发生或超时
                mysylar::Fiber::YieldToHold();
                state = IoState::RETRY;
                break;
            }
        }
        case IoState::RETRY:
        {
            // 协程被唤醒后，先取消定时器，避免误
            if (timer)
            {
                // 这时候就是timer有addConditionTimer ，避免误触发,取消定时器
                timer->cancel();
            }
            if (tinfo->cancelled)
            {
                // 这时候就是超时或者被取消了，返回错误
                errno = tinfo->cancelled;

                return -1;
            }
            // 事件发生了，也就是这个协程被唤醒，重新尝试 IO 操作
            state = IoState::TRY_IO;
            break;
        }
        defult:
        {
            state = IoState::DONE;
            break;
        }
        }
    }
}*/
template <typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name, uint32_t event,
                     int timeout_so, ssize_t buflen, Args&&... args)
{
    if (!mysylar::t_hook_enable)
    {
        // 不是hook版本，直接调用原始函数
        return fun(fd, std::forward<Args>(args)...);
    }
    mysylar::FdCtx::ptr ctx = mysylar::FdMgr::GetInstance()->get(fd);
    if (!ctx)
    {
        return fun(fd, std::forward<Args>(args)...);
    }
    if (ctx->isClose())
    {
        errno = EBADF; // EBADF(Bad File Descriptor)文件描述符错误,意思是这个 fd 已经关闭/无效
        return -1;
    }
    // 如果不是socket或者用户设置了非阻塞，就直接调用原始函数
    if (!ctx->isSocket() || ctx->getUserNonblock())
    {
        return fun(fd, std::forward<Args>(args)...);
    }

    // to timeout
    uint64_t to = ctx->getTimeout(timeout_so);
    std::shared_ptr<timer_info> tinfo(new timer_info);
retry:
    // 先直接尝试 IO，是因为“很多时候数据其实已经准备好了”，没必要立刻进入 epoll + 挂起协程,优化
    size_t n = fun(fd, std::forward<Args>(args)...);

    while (n == -1 && errno == EINTR)
    {
        // 被打断了，继续尝试
        n = fun(fd, std::forward<Args>(args)...);
    }
    // 如果是 EAGAIN(Resource temporarily unavailable (资源暂时不可用，请稍后重试。))，说明是非阻塞
    // I/O 操作，需要挂起协程等待事件发生
    if (n == -1 && errno == EAGAIN)
    {
        //
        mysylar::IOManager* iom = mysylar::IOManager::GetThis();
        mysylar::Timer::ptr timer;
        std::weak_ptr<timer_info> winfo(tinfo);
        // 判断 timeout 是否有效（是否设置了超时时间）
        if (to != (uint64_t) -1)
        {
            // 添加一个条件定时器，超时后会执行回调函数，回调函数会设置 timer_info 的 cancelled
            // 字段，并取消事件监听，唤醒协程
            timer = iom->addConditionTimer(
                to,
                [winfo, fd, iom, event]()
                {
                    auto t = winfo.lock();
                    // 避免重复处理或访问已经失效的 timer 状态
                    if (!t || t->cancelled)
                        return;
                    t->cancelled = ETIMEDOUT; // ETIMEDOUT (Connection timed out)连接超时
                    // 超时了，取消事件监听，唤醒协程
                    iom->cancelEvent(fd, (mysylar::IOManager::Event)(event));
                },
                winfo);
        }

        int rt = iom->addEvent(fd, (mysylar::IOManager::Event)(event));
        if (rt)
        {
            SYLAR_LOG_ERROR(g_logger)
                << hook_fun_name << " addEvent(" << fd << ", " << event << ")";
            if (timer)
            {
                timer->cancel();
            }
            return -1;
        }
        else
        {
            // 挂起协程，等待事件发生或超时
            mysylar::Fiber::YieldToHold();
            // 协程被唤醒后，先取消定时器，避免误
            if (timer)
            {
                // 这时候就是timer有addConditionTimer ，避免误触发,取消定时器
                timer->cancel();
            }
            if (tinfo->cancelled)
            {
                // 这时候就是超时或者被取消了，返回错误
                errno = tinfo->cancelled;
                return -1;
            }
            // 事件发生了，也就是这个协程被唤醒，重新尝试 IO 操作
            goto retry;
        }
    }
    return n;
}

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

        // std::bind((void(mysylar::Scheduler::*)(mysylar::Fiber::ptr&, int)) &
        // mysylar::IOManager::schedule, iom, fiber, -1) 绑定成员函数，生成一个可调用对象
        // 成员函数指针调用必须绑定 this 对象
        iom->addTimer(seconds * 1000,
                      std::bind((void(mysylar::Scheduler::*)(mysylar::Fiber::ptr&, int)) &
                                    mysylar::IOManager::schedule,
                                iom, fiber, -1));
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
        iom->addTimer(usec / 1000,
                      std::bind((void(mysylar::Scheduler::*)(mysylar::Fiber::ptr&, int)) &
                                    mysylar::IOManager::schedule,
                                iom, fiber, -1));
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

    // socket函数的hook版本
    int socket(int domain, int type, int protocol) {}
    int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {}
    int accept(int s, struct sockaddr* addr, socklen_t* addrlen) {}
}
