
#include "../sylar/address.h"
#include "../sylar/log.h"
#include <arpa/inet.h>
#include <iostream>
#include <sys/socket.h>
using namespace mysylar;

Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test()
{
    std::vector<Address::ptr> addrs;
    bool v = Address::Lookup(addrs, "www.lunisolar.top");
    if (!v)
    {
        SYLAR_LOG_ERROR(g_logger) << "lookup failed";
        return;
    }
    for (size_t i = 0; i < addrs.size(); ++i)
    {
        SYLAR_LOG_INFO(g_logger) << i << "-" << addrs[i]->toString();
    }
}
void test_iface()
{
    std::multimap<std::string, std::pair<Address::ptr, uint32_t>> results;
    bool v = Address::GetInterfaceAddresses(results);
    if (!v)
    {
        SYLAR_LOG_ERROR(g_logger) << "getInterFace failed";
        return;
    }
    for (auto i : results)
    {
        SYLAR_LOG_INFO(g_logger) << i.first << "-" << i.second.first->toString() << "-"
                                 << i.second.second;
    }
}
void test_ipv4()
{
    auto ip = IPAddress::Create("192.168.10.129");
    if (ip)
    {
        SYLAR_LOG_INFO(g_logger) << ip->toString();
    }
}
int main()
{
    /*
    IPAddress::ptr ip = IPAddress::Create("www.baidu.com");

    uint32_t prefix_len = 24;

    auto net = ip->networdAddress(prefix_len);
    auto bcast = ip->broadcastAddress(prefix_len);
    auto mask = ip->subnetMask(prefix_len);

    std::cout << "IP      : " << *ip << std::endl;
    std::cout << "Network : " << *net << std::endl;
    std::cout << "Broadcast: " << *bcast << std::endl;
    std::cout << "Mask    : " << *mask << std::endl;*/
    test_ipv4();
    return 0;
}