#ifndef __SYLAR__HOOK_H__
#define __SYLAR__HOOK_H__

// Hook（钩子）本质上是：拦截并替换原有函数行为的技术。通过Hook，我们可以在不修改原有函数代码的情况下，改变函数的行为，或者在函数执行前后添加一些额外的逻辑。
// 在Linux系统中，Hook技术常用于调试、性能分析、安全监控等

// Socket相关函数的都是同步的，Hook技术可以将这些函数替换为异步的版本，从而实现非阻塞的网络编程。这对于高性能服务器开发非常有用，可以提高系统的吞吐量和响应速度。
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
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

    typedef int (*nanosleep_fun)(const struct timespec* req, struct timespec* rem);
    extern nanosleep_fun nanosleep_f;

    // socket
    typedef int (*socket_fun)(int domain, int type, int protocol);
    extern socket_fun socket_f;

    typedef int (*connect_fun)(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
    extern connect_fun connect_f;

    typedef int (*accept_fun)(int s, struct sockaddr* addr, socklen_t* addrlen);
    extern accept_fun accept_f;

    // read
    typedef ssize_t (*read_fun)(int fd, void* buf, size_t count);
    extern read_fun read_f;

    typedef ssize_t (*readv_fun)(int fd, const struct iovec* iov, int iovcnt);
    extern readv_fun readv_f;

    typedef ssize_t (*recv_fun)(int sockfd, void* buf, size_t len, int flags);
    extern recv_fun recv_f;
    typedef ssize_t (*recvfrom_fun)(int sockfd, void* buf, size_t len, int flags,
                                    struct sockaddr* src_addr, socklen_t* addrlen);
    extern recvfrom_fun recvfrom_f;
    typedef ssize_t (*recvmsg_fun)(int sockefd, struct msghdr* msg, int flags);
    extern recvmsg_fun recvmsg_f;

    // write
    typedef size_t (*write_fun)(int fd, const void* buf, size_t count);
    extern write_fun write_f;
    typedef ssize_t (*writev_fun)(int fd, const struct iovec* iov, int iovcnt);
    extern writev_fun writev_f;
    typedef ssize_t (*send_fun)(int sockfd, const void* buf, size_t len, int flags);
    extern send_fun send_f;
    typedef ssize_t (*sendto_fun)(int sockfd, const void* buf, size_t len, int flags,
                                  const struct sockaddr* dest_addr, socklen_t addrlen);
    extern sendto_fun sendto_f;
    typedef ssize_t (*sendmsg_fun)(int sockfd, const struct msghdr* msg, int flags);
    extern sendmsg_fun sendmsg_f;

    // close
    typedef int (*close_fun)(int fd);
    extern close_fun close_f;
}

#endif //__SYLAR__HOOK_H__