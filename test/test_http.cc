#include "../sylar/http/http.h"
#include "../sylar/http/http_parser.h"
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

void test_httpRequestParser()
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
    mysylar::http::HttpRequestParser parser;
    parser.getData()->setHeader("host", "www.baidu.com");
    parser.getData()->setMethod(mysylar::http::HttpMethod::GET);
    std::string request = parser.getData()->toString();
    int rt = sock->send(request.c_str(), request.size());
    if (rt <= 0)
    {
        SYLAR_LOG_ERROR(g_logger) << "send failed rt=" << rt;
        return;
    }
    std::string buffers;
    buffers.resize(4069);
    char buf[4096];
    mysylar::http::HttpResponseParser resp_parser;

    std::string buffer;
    while (true)
    {
        int rt = sock->recv(buf, sizeof(buf));
        if (rt <= 0)
            break;

        buffer.append(buf, rt);

        size_t nparse = resp_parser.execute(&buffer[0], buffer.size(), false);

        buffer.erase(0, nparse);

        if (resp_parser.isFinished())
            break;
    }
    if (resp_parser.isFinished())
    {
        SYLAR_LOG_INFO(g_logger) << "response: " << std::endl
                                 << resp_parser.getData()->toString()
                                 << resp_parser.getData()->getBody();
    }
    else if (resp_parser.hasError())
    {
        SYLAR_LOG_ERROR(g_logger) << "parse response error";
    }
    else
    {
        SYLAR_LOG_INFO(g_logger) << "response no finished";
    }
}

void test_httpResponseParser()
{
    std::string data = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello";
    mysylar::http::HttpResponseParser parser;

    
}

int main()
{
    // test_req();
    // test_response();
    // test_httpRequestParser();
    test_httpResponseParser();
    return 0;
}