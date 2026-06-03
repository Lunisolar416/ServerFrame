#ifndef __SYLAR_TCP_SERVER_H__
#define __SYLAR_TCP_SERVER_H__

#include "address.h"
#include "config.h"
#include "iomanager.h"
#include "noncopyable.h"
#include "socket.h"
#include <functional>
#include <memory>
#include <sys/types.h>
#include <vector>
namespace mysylar
{
class TcpServer : public std::enable_shared_from_this<TcpServer>, Noncopyable
{
  public:
    typedef std::shared_ptr<TcpServer> ptr;
    TcpServer(IOManager* worker = IOManager::GetThis(),
              IOManager* accpet_worker = IOManager::GetThis());

    virtual ~TcpServer();

    virtual bool bind(Address::ptr addr);
    virtual bool bind(const std::vector<Address::ptr>& addrs, std::vector<Address::ptr>& fails);
    virtual bool start();
    virtual void stop();

    uint64_t getRecvTimeout() const
    {
        return m_recvTimeout;
    }
    std::string getName() const
    {
        return m_name;
    }

    void setRecvTimeout(uint64_t v)
    {
        m_recvTimeout = v;
    }
    void setName(const std::string& v)
    {
        m_name = v;
    }

    bool isStop() const
    {
        return m_isStop;
    }
    std::vector<Socket::ptr> getSocks() const
    {
        return m_socks;
    }

  protected:
    virtual void handleClient(Socket::ptr client);
    virtual void startAccept(Socket::ptr sock);

  private:
    std::vector<Socket::ptr> m_socks;
    IOManager* m_worker;
    IOManager* m_acceptWorker;
    uint64_t m_recvTimeout;
    std::string m_name;
    std::string m_type = "tcp";
    bool m_isStop;
};
} // namespace mysylar

#endif // __SYLAR_TCP_SERVER_H__