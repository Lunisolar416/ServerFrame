#include "../sylar/mysylar.h"
#include "../sylar/tcp_server.h"

static mysylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run()
{
    auto addr = mysylar::Address::LookupAny("0.0.0.0:8080");
    // SYLAR_LOG_INFO(g_logger) << *addr << " - " << *unixaddr;
    std::vector<mysylar::Address::ptr> addrs;
    std::vector<mysylar::Address::ptr> fails;
    addrs.push_back(addr);

    mysylar::TcpServer::ptr tcp_server(new mysylar::TcpServer);

    while (!tcp_server->bind(addrs, fails))
    {
        sleep(2);
    }
    tcp_server->start();
}

int main()
{
    mysylar::IOManager iom(2);
    iom.schedule(run);
    return 0;
}