#include "../sylar/mysylar.h"
#include "../sylar/tcp_server.h"
#include <bits/types/struct_iovec.h>
#include <cstring>
#include <iostream>
#include <vector>
using namespace mysylar;

static Logger::ptr g_logger = SYLAR_LOG_NAME("Echo Server");
class EchoServer : public TcpServer
{
  public:
    EchoServer(int type) : m_type(type) {}
    void handleClient(Socket::ptr client) override
    {
        SYLAR_LOG_INFO(g_logger) << "HandleClient" << *client;
        ByteArray::ptr ba(new ByteArray);
        while (true)
        {
            ba->clear();
            std::vector<iovec> iovs;
            ba->getWriteBuffers(iovs, 1024);

            int rt = client->recv(&iovs[0], iovs.size());
            if (rt == 0)
            {
                SYLAR_LOG_INFO(g_logger) << "client close :" << *client;
                break;
            }
            else if (rt < 0)
            {
                SYLAR_LOG_INFO(g_logger)
                    << "client error rt=" << rt << " errno = " << errno << strerror(errno);
                break;
            }

            ba->setPosition(ba->getPosition() + rt);
            ba->setPosition(0);
            if (m_type == 1)
            {                                // text
                std::cout << ba->toString(); // << std::endl;
            }
            else
            {
                std::cout << ba->toHexString(); // << std::endl;
            }
            std::cout.flush();

            // 获取有效数据对应的读缓冲区
            std::vector<iovec> buffers;
            ba->getReadBuffers(buffers, rt);

            // 回显给客户端
            int send_rt = client->send(&buffers[0], buffers.size());

            if (send_rt <= 0)
            {
                SYLAR_LOG_INFO(g_logger) << "send error rt=" << send_rt << " errno=" << errno
                                         << " errstr=" << strerror(errno);
                break;
            }
        }
    }

  private:
    int m_type = 0;
};
int type = 1;
void run()
{
    SYLAR_LOG_INFO(g_logger) << "server type=" << type;
    EchoServer::ptr es(new EchoServer(type));
    auto addr = Address::LookupAny("0.0.0.0:8020");
    while (!es->bind(addr))
    {
        sleep(2);
    }
    es->start();
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        SYLAR_LOG_INFO(g_logger) << "used as[" << argv[0] << " -t] or [" << argv[0] << " -b]";
        return 0;
    }

    if (!strcmp(argv[1], "-b"))
    {
        type = 2;
    }

    IOManager iom(2);
    iom.schedule(run);
    return 0;
}
