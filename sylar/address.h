#ifndef __SYLAR_ADDRESS_H__
#define __SYLAR_ADDRESS_H__

#include <map>
#include <memory>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <vector>
namespace mysylar
{
class IPAddress;
class Address
{
  public:
    typedef std::shared_ptr<Address> ptr;

    static Address::ptr Create(const sockaddr* addr, socklen_t addrlen);

    static bool Lookup(std::vector<Address::ptr>& result, const std::string& host,
                       int family = AF_INET, int type = 0, int protocol = 0);
    static Address::ptr LookupAny(const std::string& host, int family = AF_INET, int type = 0,
                                  int protocol = 0);
    static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string& host,
                                                         int family = AF_INET, int type = 0,
                                                         int protocol = 0);
    static bool GetInterfaceAddresses(
        std::multimap<std::string, std::pair<Address::ptr, uint32_t>>& result,
        int family = AF_INET);
    static bool GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t>>& result,
                                      const std::string& iface, int family = AF_INET);
    virtual ~Address() {}

    int getFamily() const;

    virtual sockaddr* getAddr() = 0;
    virtual const sockaddr* getAddr() const = 0;
    // virtual sockaddr* getAddr() = 0;
    virtual socklen_t getAddrLen() const = 0;
    virtual std::ostream& insert(std::ostream& os) const = 0;

    bool operator<(const Address& rhs) const;
    bool operator==(const Address& rhs) const;
    bool operator!=(const Address& rhs) const;

    std::string toString();
};

class IPAddress : public Address
{
  public:
    typedef std::shared_ptr<IPAddress> ptr;
    static IPAddress::ptr Create(const char* address, uint16_t port = 0);
    // 广播地址
    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;
    // 网络地址
    virtual IPAddress::ptr networdAddress(uint32_t prefix_len) = 0;
    // 子网掩码
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

    //
    virtual uint32_t getPort() const = 0;
    virtual void setPort(uint16_t v) = 0;
};

class IPv4Address : public IPAddress
{
  public:
    typedef std::shared_ptr<IPv4Address> ptr;
    static IPv4Address::ptr Create(const char* address, uint16_t port = 0);
    IPv4Address(const sockaddr_in& address);
    IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);
    // 广播地址
    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    // 网络地址
    IPAddress::ptr networdAddress(uint32_t prefix_len) override;
    // 子网掩码
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;

    uint32_t getPort() const override;
    void setPort(uint16_t v) override;

    sockaddr* getAddr() override;
    const sockaddr* getAddr() const override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

  private:
    sockaddr_in m_addr;
};
class IPv6Address : public IPAddress
{
  public:
    typedef std::shared_ptr<IPv6Address> ptr;
    static IPv6Address::ptr Create(const char* address, uint16_t port = 0);
    IPv6Address();
    IPv6Address(const sockaddr_in6& address);
    IPv6Address(const uint8_t address[16], uint16_t port = 0);
    // 广播地址
    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    // 网络地址
    IPAddress::ptr networdAddress(uint32_t prefix_len) override;
    // 子网掩码
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;

    uint32_t getPort() const override;
    void setPort(uint16_t v) override;
    sockaddr* getAddr() override;
    const sockaddr* getAddr() const override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

  private:
    sockaddr_in6 m_addr;
};
class UnixAddress : public Address
{
  public:
    typedef std::shared_ptr<UnixAddress> ptr;
    UnixAddress();
    UnixAddress(const std::string& path);
    sockaddr* getAddr() override;
    const sockaddr* getAddr() const override;
    virtual socklen_t getAddrLen() const override;
    virtual std::ostream& insert(std::ostream& os) const;
    void setAddrLen(uint32_t v);

  private:
    struct sockaddr_un m_addr;
    socklen_t m_length;
};
class UnknownAddress : public Address
{
  public:
    typedef std::shared_ptr<UnknownAddress> ptr;
    UnknownAddress(int family);
    UnknownAddress(const sockaddr& addr);
    sockaddr* getAddr() override;
    const sockaddr* getAddr() const override;
    virtual socklen_t getAddrLen() const override;
    virtual std::ostream& insert(std::ostream& os) const;

  private:
    sockaddr m_addr;
};
/**
 * @brief 流式输出Address
 */
std::ostream& operator<<(std::ostream& os, const Address& addr);
} // namespace mysylar
#endif // __SYLAR_ADDRESS_H__