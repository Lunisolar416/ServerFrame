#include "../sylar/socket.h"

#include "../sylar/iomanager.h"
#include "../sylar/mysylar.h"
static mysylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_socket()
{
    mysylar::IPAddress::ptr addr = mysylar::Address::LookupAnyIPAddress("www.baidu.com");

    if (addr)
    {
        addr->setPort(80);
    }
    else
    {
        SYLAR_LOG_ERROR(g_logger) << "get address failed";
        return;
    }
    mysylar::Socket::ptr sock = mysylar::Socket::CreateTCP(addr);
    SYLAR_LOG_INFO(g_logger) << "get address: " << addr->toString();
    bool res = sock->connect(addr);
    if (res)
    {
        SYLAR_LOG_INFO(g_logger) << "connect " << addr->toString() << " connected";
    }
    else
    {
        SYLAR_LOG_ERROR(g_logger) << "connect " << addr->toString() << " failed";
    }
    const char buffs[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buffs, sizeof(buffs));
    if (rt <= 0)
    {
        SYLAR_LOG_ERROR(g_logger) << "send failed rt=" << rt;
        return;
    }
    std::string buffers;
    buffers.resize(4069);
    int wt = sock->recv(&buffers[0], buffers.size());
    if (wt <= 0)
    {
        SYLAR_LOG_ERROR(g_logger) << "recv failed wt=" << wt;
        return;
    }
    buffers.resize(wt);
    SYLAR_LOG_INFO(g_logger) << buffers;
}

int main()
{
    mysylar::IOManager iom;
    iom.schedule(&test_socket);
    return 0;
}