#include "../sylar/address.h"
#include "../sylar/http/http_connection.h"
#include "../sylar/http/http_server.h"
#include "../sylar/mysylar.h"
#include "../sylar/tcp_server.h"
#include <iostream>

mysylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run()
{
    auto addr = mysylar::Address::LookupAny("httpbin.org:80");

    if (!addr)
    {
        return;
    }

    auto sock = mysylar::Socket::CreateTCP(addr);

    if (!sock->connect(addr))
    {
        return;
    }

    mysylar::http::HttpConnection::ptr conn(new mysylar::http::HttpConnection(sock));

    auto req = std::make_shared<mysylar::http::HttpRequest>();

    req->setPath("/stream/5");
    req->setHeader("host", "httpbin.org");

    SYLAR_LOG_INFO(g_logger) << *req;

    conn->sendRequest(req);

    auto rsp = conn->recvResponse();

    if (!rsp)
    {
        SYLAR_LOG_ERROR(g_logger) << "recv failed";

        return;
    }

    SYLAR_LOG_INFO(g_logger) << *rsp;
}

int main()
{
    run();
    return 0;
}