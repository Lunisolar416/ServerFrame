#ifndef __SYLAR__HOOK_H__
#define __SYLAR__HOOK_H__

// Hook（钩子）本质上是：拦截并替换原有函数行为的技术。通过Hook，我们可以在不修改原有函数代码的情况下，改变函数的行为，或者在函数执行前后添加一些额外的逻辑。
// 在Linux系统中，Hook技术常用于调试、性能分析、安全监控等

// Socket相关函数的都是同步的，Hook技术可以将这些函数替换为异步的版本，从而实现非阻塞的网络编程。这对于高性能服务器开发非常有用，可以提高系统的吞吐量和响应速度。
#include <fcntl.h>
#include <stdint.h>
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

    // fcntl 对已经打开的 fd（文件/Socket）进行各种控制操作 fd：文件描述符（socket / file
    // /pipe）cmd：控制命令 arg：可选参数（看 cmd）
    // 设置非阻塞 fcntl(fd, F_SETFL, O_NONBLOCK);
    // 获取当前的文件状态标志 fcntl(fd, F_GETFL);
    // 设置当前的文件状态标志 fcntl(fd, F_SETFL, flags);
    // 复制文件描述符 fcntl(fd, F_DUPFD, arg);
    typedef int (*fcntl_fun)(int fd, int cmd, ...);
    extern fcntl_fun fcntl_f;

    // ioctl 对“设备/文件描述符”的特殊控制通道
    typedef int (*ioctl_fun)(int d, unsigned long int request, ...);
    extern ioctl_fun ioctl_f;

    // getsockopt/setsockopt 读取/设置 socket 参数（选项）
    // level：参数所在的协议层（SOL_SOCKET / IPPROTO_TCP / IPPROTO_IP）optname：参数名称
    // optval：参数值 optlen：参数值长度
    // SOL_SOCKET   → socket 通用层  IPPROTO_TCP  → TCP 层 IPPROTO_IP   → IP 层
    typedef int (*getsockopt_fun)(int sockfd, int level, int optname, void* optval,
                                  socklen_t* optlen);
    extern getsockopt_fun getsockopt_f;

    // setsockopt_fun 设置 socket 参数（选项）
    /*
    端口复用 SO_REUSEADDR，允许多个套接字绑定到同一个地址和端口上，常用于服务器重启时快速恢复服务。
            int opt = 1;
            setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));*/
    /*
    开启 keepalive，保活TCP链接，定期发送心跳包检测连接是否仍然有效，防止死链接占用资源。
            int opt = 1;
            setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
    */
    typedef int (*setsockopt_fun)(int sockfd, int level, int optname, const void* optval,
                                  socklen_t optlen);
    extern setsockopt_fun setsockopt_f;

    extern int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen,
                                    uint64_t timeout_ms);
}

#endif //__SYLAR__HOOK_H__