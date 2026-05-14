#include "../sylar/iomanager.h"
#include "../sylar/log.h"
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
mysylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();
int sock = 0;

void test_fiber()
{
    SYLAR_LOG_INFO(g_logger) << "test_fiber sock=" << sock;

    // sleep(3);

    // close(sock);
    // sylar::IOManager::GetThis()->cancelAll(sock);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "115.239.210.27", &addr.sin_addr.s_addr);

    if (!connect(sock, (const sockaddr*) &addr, sizeof(addr)))
    {
    }
    else if (errno == EINPROGRESS)
    {
        SYLAR_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        mysylar::IOManager::GetThis()->addEvent(sock, mysylar::IOManager::READ, []()
                                                { SYLAR_LOG_INFO(g_logger) << "read callback"; });
        mysylar::IOManager::GetThis()->addEvent(sock, mysylar::IOManager::WRITE,
                                                []()
                                                {
                                                    SYLAR_LOG_INFO(g_logger) << "write callback";
                                                    // close(sock);
                                                    mysylar::IOManager::GetThis()->cancelEvent(
                                                        sock, mysylar::IOManager::READ);
                                                    close(sock);
                                                });
    }
    else
    {
        SYLAR_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }
}

void test1()
{
    std::cout << "EPOLLIN=" << EPOLLIN << " EPOLLOUT=" << EPOLLOUT << std::endl;
    mysylar::IOManager iom(2, false);
    iom.schedule(&test_fiber);
}
void test_server()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    fcntl(fd, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(fd, (sockaddr*) &addr, sizeof(addr));
    listen(fd, 100);

    mysylar::IOManager::GetThis()->addEvent(fd, mysylar::IOManager::READ,
                                            [fd]()
                                            {
                                                sockaddr_in client;
                                                socklen_t len = sizeof(client);

                                                int cfd = accept(fd, (sockaddr*) &client, &len);

                                                if (cfd > 0)
                                                {
                                                    fcntl(cfd, F_SETFL, O_NONBLOCK);

                                                    std::cout << "new client: " << cfd << std::endl;

                                                    // 注册 client 的读事件
                                                    mysylar::IOManager::GetThis()->addEvent(
                                                        cfd, mysylar::IOManager::READ,
                                                        [cfd]()
                                                        {
                                                            char buf[1024];
                                                            int n = recv(cfd, buf, sizeof(buf), 0);

                                                            if (n > 0)
                                                            {
                                                                buf[n] = '\0';
                                                                std::cout << "recv: " << buf
                                                                          << std::endl;
                                                                send(cfd, buf, n, 0);
                                                            }
                                                            else
                                                            {
                                                                close(cfd);
                                                            }
                                                        });
                                                }
                                            });
}

void testTimer()
{
    mysylar::IOManager iom(2);
    iom.addTimer(500, []() { SYLAR_LOG_INFO(g_logger) << "Hello Timer"; }, true);
}
int main()
{
    testTimer();
    return 0;
}