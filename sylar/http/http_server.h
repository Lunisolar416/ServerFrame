#ifndef __SYLAR_HTTP_SERVER_H__
#define __SYLAR_HTTP_SERVER_H__

#include "../tcp_server.h"
#include "http.h"
#include "http_session.h"
#include "servlet.h"
#include <memory>
namespace mysylar
{
namespace http
{

class HttpServer : public TcpServer
{
  public:
    typedef std::shared_ptr<HttpServer> ptr;
    HttpServer(bool keepalive = false, IOManager* worker = IOManager::GetThis(),
               IOManager* accept_worker = IOManager::GetThis());
    ServletDispatch::ptr getServletDispatch() const
    {
        return m_dispatch;
    }
    void setServletDispatch(ServletDispatch::ptr v)
    {
        m_dispatch = v;
    }

  protected:
    virtual void handleClient(Socket::ptr client) override;

  private:
    bool m_isKeepalive;
    ServletDispatch::ptr m_dispatch;
};

} // namespace http
} // namespace mysylar

#endif