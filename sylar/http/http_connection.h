#ifndef __SYLAR_HTTP_CONNECTION_H__
#define __SYLAR_HTTP_CONNECTION_H__

#include "http.h"
#include <memory>

#include "../socket_stream.h"
namespace mysylar
{
namespace http
{

class HttpConnection : public SocketStream
{
  public:
    typedef std::shared_ptr<HttpConnection> ptr;
    HttpConnection(Socket::ptr sock, bool owner = true);
    ~HttpConnection();
    HttpResponse::ptr recvResponse();
    int sendRequest(HttpRequest::ptr req);

  private:
};

} // namespace http
} // namespace mysylar

#endif //__SYLAR_HTTP_CONNECTION_H__