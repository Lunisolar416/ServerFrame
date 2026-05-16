#ifndef __SYLAR_ADDRESS_H__
#define __SYLAR_ADDRESS_H__

#include <cstring>
#include <memory>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

class Address
{
  public:
    typedef std::shared_ptr<Address> ptr;
    virtual ~Address() {}

    int getFamily() const;

    virtual const sockaddr* getAddr() const = 0;
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
    IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);
    // 广播地址
    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    // 网络地址
    IPAddress::ptr networdAddress(uint32_t prefix_len) override;
    // 子网掩码
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;

    uint32_t getPort() const override;
    void setPort(uint16_t v) override;

    const sockaddr* getAddr() const override;
    virtual socklen_t getAddrLen() const override;
    virtual std::ostream& insert(std::ostream& os) const;

  private:
    sockaddr_in m_addr;
};
class IPv6Address : public IPAddress
{
  public:
    typedef std::shared_ptr<IPv6Address> ptr;

    IPv6Address(uint32_t address = INADDR_ANY, uint16_t port = 0);
    // 广播地址
    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    // 网络地址
    IPAddress::ptr networdAddress(uint32_t prefix_len) override;
    // 子网掩码
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;

    uint32_t getPort() const override;
    void setPort(uint16_t v) override;

    const sockaddr* getAddr() const override;
    virtual socklen_t getAddrLen() const override;
    virtual std::ostream& insert(std::ostream& os) const;

  private:
    sockaddr_in6 m_addr;
};
class UnixAddress : public Address
{
  public:
    typedef std::shared_ptr<UnixAddress> ptr;
    UnixAddress(const std::string& path);
    const sockaddr* getAddr() const override;
    virtual socklen_t getAddrLen() const override;
    virtual std::ostream& insert(std::ostream& os) const;

  private:
    struct sockaddr_un m_addr;
    socklen_t m_length;
};
class UnknownAddress : public Address
{
  public:
    typedef std::shared_ptr<UnknownAddress> ptr;
    UnknownAddress(int family);
    const sockaddr* getAddr() const override;
    virtual socklen_t getAddrLen() const override;
    virtual std::ostream& insert(std::ostream& os) const;

  private:
    sockaddr m_addr;
};
#endif // __SYLAR_ADDRESS_H__