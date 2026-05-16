#include "../sylar/hook.h"
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

void test_sleep()
{
    mysylar::IOManager iom(1);
    iom.schedule(
        []()
        {
            sleep(2);
            SYLAR_LOG_INFO(g_logger) << "sleep 2";
        });
    iom.schedule(
        []()
        {
            sleep(3);
            SYLAR_LOG_INFO(g_logger) << "sleep 3";
        });
    SYLAR_LOG_INFO(g_logger) << "test_sleep";
}

void test_sock()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "39.156.70.239", &addr.sin_addr.s_addr);

    SYLAR_LOG_INFO(g_logger) << "begin connect";
    int rt = connect(sock, (const sockaddr*) &addr, sizeof(addr));
    SYLAR_LOG_INFO(g_logger) << "connect rt=" << rt << " errno=" << errno;

    if (rt)
    {
        return;
    }

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    rt = send(sock, data, sizeof(data), 0);
    SYLAR_LOG_INFO(g_logger) << "send rt=" << rt << " errno=" << errno;

    if (rt <= 0)
    {
        return;
    }

    std::string buffer;
    char temp[4096];

    while (true)
    {
        int rt = recv(sock, temp, sizeof(temp), 0);

        if (rt > 0)
        {
            buffer.append(temp, rt);
        }
        else if (rt == 0)
        {
            break;
        }
        else
        {
            break;
        }
    }

    SYLAR_LOG_INFO(g_logger) << buffer;
}

int main()
{
    // test_sleep();
    mysylar::IOManager iom;
    iom.schedule(&test_sock);
    return 0;
}