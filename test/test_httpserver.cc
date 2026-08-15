#include "../sylar/http/http_server.h"
#include "../sylar/http/http_session.h"
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

    mysylar::http::HttpServer::ptr server(new mysylar::http::HttpServer);
    auto sd = server->getServletDispatch();
    sd->addServlet("/mysylar/xx",
                   [](mysylar::http::HttpRequest::ptr request,
                      mysylar::http::HttpResponse::ptr response,
                      mysylar::http::HttpSession::ptr session)
                   {
                       response->setBody(request->toString());
                       return 0;
                   });
    sd->addGlobServlet("/mysylar/*",
                       [](mysylar::http::HttpRequest::ptr request,
                          mysylar::http::HttpResponse::ptr response,
                          mysylar::http::HttpSession::ptr session)
                       {
                           response->setBody(request->toString());
                           return 0;
                       });
    while (!server->bind(addrs, fails))
    {
        sleep(2);
    }
    server->start();
}

int main()
{
    mysylar::IOManager iom(2);
    iom.schedule(run);
    return 0;
}