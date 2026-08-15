#ifndef __SYLAR_HTTP_SESSION_H__
#define __SYLAR_HTTP_SESSION_H__

#include "http.h"

#include "../socket_stream.h"
#include <memory>
namespace mysylar
{
namespace http
{
class HttpSession : public SocketStream
{
  public:
    typedef std::shared_ptr<HttpSession> ptr;
    HttpSession(Socket::ptr sock, bool owner = true);
    HttpRequest::ptr recvRequest();
    int sendResponse(HttpResponse::ptr res);

  private:
};
} // namespace http
} // namespace mysylar

#endif