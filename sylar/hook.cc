
#include "hook.h"
#include "config.h"
#include "fd_manager.h"
#include "fiber.h"
#include "iomanager.h"
#include "log.h"
#include "scheduler.h"
#include "timer.h"
#include <asm-generic/socket.h>
#include <cerrno>
#include <ctime>
#include <dlfcn.h>
#include <functional>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>
static mysylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
namespace mysylar
{

static thread_local bool t_hook_enable = false;
static mysylar::ConfigVar<int>::ptr g_tcp_connect_timeout =
    mysylar::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");
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
static uint64_t s_connect_timeout = -1;
struct _HookIniter
{
    _HookIniter()
    {
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getValue();
        // 监听配置项变化，动态更新连接超时时间
        g_tcp_connect_timeout->addListener(
            [](const int& old_value, const int& new_value)
            {
                SYLAR_LOG_INFO(g_logger)
                    << "tcp connect timeout changed from " << old_value << " to " << new_value;
                s_connect_timeout = new_value;
            });
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
                     int timeout_so, Args&&... args)
{
    if (!mysylar::t_hook_enable)
    {
        // 不是hook版本，直接调用原始函数
        return fun(fd, std::forward<Args>(args)...);
    }
    SYLAR_LOG_INFO(g_logger) << "do_io: " << hook_fun_name << " fd=" << fd << " event=" << event;
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
    ssize_t n = fun(fd, std::forward<Args>(args)...);

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
    int socket(int domain, int type, int protocol)
    {
        if (!mysylar::t_hook_enable)
        {
            return socket_f(domain, type, protocol);
        }
        int fd = socket_f(domain, type, protocol);
        if (fd == -1)
        {
            return fd;
        }
        mysylar::FdMgr::GetInstance()->get(fd, true);
        return fd;
    }

    // connect函数的hook版本
    int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen,
                             uint64_t timeout_ms)
    {
        if (!mysylar::t_hook_enable)
        {
            return connect_f(fd, addr, addrlen);
        }
        mysylar::FdCtx::ptr ctx = mysylar::FdMgr::GetInstance()->get(fd);
        if (!ctx || ctx->isClose())
        {
            errno = EBADF;
            return -1;
        }
        if (!ctx->isSocket())
        {
            return connect_f(fd, addr, addrlen);
        }
        if (ctx->getUserNonblock())
        {
            return connect_f(fd, addr, addrlen);
        }

        int n = connect(fd, addr, addrlen);
        if (n == 0)
        {
            return 0;
        }
        else if (n != -1 || errno != EINPROGRESS)
        {
            return n;
        }
        mysylar::IOManager* iom = mysylar::IOManager::GetThis();
        mysylar::Timer::ptr timer;
        std::shared_ptr<timer_info> tinfo(new timer_info);
        std::weak_ptr<timer_info> winfo(tinfo);
        if (timeout_ms != (uint64_t) -1)
        {
            timer = iom->addConditionTimer(
                timeout_ms,
                [winfo, fd, iom]()
                {
                    auto t = winfo.lock();
                    if (!t || t->cancelled)
                    {
                        return;
                    }
                    t->cancelled = ETIMEDOUT;
                    iom->cancelEvent(fd, mysylar::IOManager::WRITE);
                },
                winfo);
        }

        int rt = iom->addEvent(fd, mysylar::IOManager::WRITE);
        if (rt == 0)
        {
            mysylar::Fiber::YieldToHold();
            if (timer)
            {
                timer->cancel();
            }
            if (tinfo->cancelled)
            {
                errno = tinfo->cancelled;
                return -1;
            }
        }
        else
        {
            if (timer)
            {
                timer->cancel();
            }
            SYLAR_LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";
        }
        int error = 0;
        socklen_t len = sizeof(int);
        if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len))
        {
            return -1;
        }
        if (!error)
        {
            return 0;
        }
        else
        {
            errno = error;
            return -1;
        }
    }
    int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen)
    {
        return connect_with_timeout(sockfd, addr, addrlen, mysylar::s_connect_timeout);
    }
    int accept(int s, struct sockaddr* addr, socklen_t* addrlen)
    {
        int fd = do_io(s, accept_f, "accept", mysylar::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
        if (fd >= 0)
        {
            mysylar::FdMgr::GetInstance()->get(fd, true);
        }
        return fd;
    }

    // read
    ssize_t read(int fd, void* buf, size_t count)
    {
        return do_io(fd, read_f, "read", mysylar::IOManager::READ, SO_RCVTIMEO, buf, count);
    }
    ssize_t readv(int fd, const struct iovec* iov, int iovcnt)
    {
        return do_io(fd, readv_f, "readv", mysylar::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
    }

    ssize_t recv(int sockfd, void* buf, size_t len, int flags)
    {
        return do_io(sockfd, recv_f, "recv", mysylar::IOManager::READ, SO_RCVTIMEO, buf, len,
                     flags);
    }

    ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr,
                     socklen_t* addrlen)
    {
        return do_io(sockfd, recvfrom_f, "recvfrom", mysylar::IOManager::READ, SO_RCVTIMEO, buf,
                     len, flags, src_addr, addrlen);
    }

    ssize_t recvmsg(int sockefd, struct msghdr* msg, int flags)
    {
        return do_io(sockefd, recvmsg_f, "recvmsg", mysylar::IOManager::READ, SO_RCVTIMEO, msg,
                     flags);
    }

    // write
    ssize_t write(int fd, const void* buf, size_t count)
    {
        return do_io(fd, write_f, "write", mysylar::IOManager::WRITE, SO_SNDTIMEO, buf, count);
    }

    ssize_t writev(int fd, const struct iovec* iov, int iovcnt)
    {
        return do_io(fd, writev_f, "writev", mysylar::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
    }

    ssize_t send(int sockfd, const void* buf, size_t len, int flags)
    {
        return do_io(sockfd, send_f, "send", mysylar::IOManager::WRITE, SO_SNDTIMEO, buf, len,
                     flags);
    }

    ssize_t sendto(int sockfd, const void* buf, size_t len, int flags,
                   const struct sockaddr* dest_addr, socklen_t addrlen)
    {
        return do_io(sockfd, sendto_f, "sendto", mysylar::IOManager::WRITE, SO_SNDTIMEO, buf, len,
                     flags, dest_addr, addrlen);
    }

    ssize_t sendmsg(int sockfd, const struct msghdr* msg, int flags)
    {
        return do_io(sockfd, sendmsg_f, "sendmsg", mysylar::IOManager::WRITE, SO_SNDTIMEO, msg,
                     flags);
    }

    // close
    int close(int fd)
    {
        if (!mysylar::t_hook_enable)
        {
            return close_f(fd);
        }
        mysylar::FdCtx::ptr ctx = mysylar::FdMgr::GetInstance()->get(fd);
        if (ctx)
        {
            auto iom = mysylar::IOManager::GetThis();
            // 取消这个 fd 上的所有事件监听，避免协程被唤醒
            if (iom)
            {
                iom->cancelAll(fd);
            }
            mysylar::FdMgr::GetInstance()->del(fd);
        }
        return close_f(fd);
    }

    // fcntl
    int fcntl(int fd, int cmd, ...)
    {
        va_list va;
        va_start(va, cmd);
        switch (cmd)
        {
        case F_SETFL:
        {
            int arg = va_arg(va, int);
            va_end(va);
            mysylar::FdCtx::ptr ctx = mysylar::FdMgr::GetInstance()->get(fd);
            if (!ctx || ctx->isClose() || !ctx->isSocket())
            {
                return fcntl_f(fd, cmd, arg);
            }
            ctx->setUserNonblock(arg & O_NONBLOCK);
            if (ctx->getSysNonblock())
            {
                arg |= O_NONBLOCK;
            }
            else
            {
                arg &= ~O_NONBLOCK;
            }
            return fcntl_f(fd, cmd, arg);
        }
        break;
        case F_GETFL:
        {
            va_end(va);
            int arg = fcntl_f(fd, cmd);
            mysylar::FdCtx::ptr ctx = mysylar::FdMgr::GetInstance()->get(fd);
            if (!ctx || ctx->isClose() || !ctx->isSocket())
            {
                return arg;
            }
            if (ctx->getUserNonblock())
            {
                return arg | O_NONBLOCK;
            }
            else
            {
                return arg & ~O_NONBLOCK;
            }
        }
        break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
#ifdef F_SETPIPE_SZ
        case F_SETPIPE_SZ:
#endif
        {
            int arg = va_arg(va, int);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
        break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
#ifdef F_GETPIPE_SZ
        case F_GETPIPE_SZ:
#endif
        {
            va_end(va);
            return fcntl_f(fd, cmd);
        }
        break;
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
        {
            struct flock* arg = va_arg(va, struct flock*);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
        break;
        case F_GETOWN_EX:
        case F_SETOWN_EX:
        {
            struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
        break;
        default:
            va_end(va);
            return fcntl_f(fd, cmd);
        }
    }
    // ioctl
    int ioctl(int d, unsigned long int request, ...)
    {
        va_list va;
        va_start(va, request);
        void* arg = va_arg(va, void*);
        va_end(va);

        if (FIONBIO == request)
        {
            bool user_nonblock = !!*(int*) arg;
            mysylar::FdCtx::ptr ctx = mysylar::FdMgr::GetInstance()->get(d);
            if (!ctx || ctx->isClose() || !ctx->isSocket())
            {
                return ioctl_f(d, request, arg);
            }
            ctx->setUserNonblock(user_nonblock);
        }
        return ioctl_f(d, request, arg);
    }
    // getsockopt
    int getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen)
    {
        return getsockopt_f(sockfd, level, optname, optval, optlen);
    }

    // setsockopt
    int setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen)
    {
        if (!mysylar::t_hook_enable)
        {
            return setsockopt_f(sockfd, level, optname, optval, optlen);
        }
        if (level == SOL_SOCKET)
        {
            if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO)
            {
                mysylar::FdCtx::ptr ctx = mysylar::FdMgr::GetInstance()->get(sockfd);
                if (ctx)
                {
                    const timeval* v = (const timeval*) optval;
                    ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
                }
            }
        }
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
}
