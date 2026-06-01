#include "../sylar/http/http.h"
#include "../sylar/mysylar.h"
#include "../sylar/socket.h"

#include "../sylar/iomanager.h"

static mysylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();
void test_req()
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
    mysylar::http::HttpRequest::ptr req(new mysylar::http::HttpRequest);
    req->setHeader("host", "www.baidu.com");
    req->setMethod(mysylar::http::HttpMethod::GET);
    std::string request = req->toString();
    int rt = sock->send(request.c_str(), request.size());
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
void test_response()
{
    mysylar::http::HttpResponse::ptr rsp(new mysylar::http::HttpResponse);
    rsp->setHeader("X-X", "sylar");
    rsp->setBody("hello sylar");
    rsp->setStatus((mysylar::http::HttpStatus) 200);
    rsp->setClose(false);

    rsp->dump(std::cout) << std::endl;
}
int main()
{
    // test_req();
    test_response();
    return 0;
}