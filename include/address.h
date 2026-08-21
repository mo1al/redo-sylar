// address.h —— 网络地址封装：支持 IPv4/IPv6/Unix 地址的抽象与构造。
#pragma once
#include<memory>
#include"en_dian.h"
#include<sys/socket.h>
#include<sys/un.h>
#include<sys/types.h>
#include<unistd.h>
#include <arpa/inet.h>
#include<string>
#include<vector>

namespace mo1al
{
class IPAddress;
// 网络地址抽象基类：封装 sockaddr 并提供地址解析与比较
class Address
{
public:
    typedef std::shared_ptr<Address>ptr;
    virtual ~Address(){}

    static Address::ptr Create(const sockaddr*addr,socklen_t addrlen);

    // 域名解析：将主机名解析为地址列表
    static bool Lookup(std::vector<Address::ptr>& result, const std::string& host,
        int family = AF_INET, int type = 0, int protocol = 0);

    static Address::ptr LookupAny(const std::string& host,
        int family = AF_INET, int type = 0, int protocol = 0);

    static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string& host,
            int family = AF_INET, int type = 0, int protocol = 0);

    // 获取地址族（IPv4/IPv6/Unix/Unknown）
    int getFamily()const;

    // 获取底层 sockaddr 指针
    virtual const sockaddr* getAddr()const = 0;
    virtual sockaddr* getAddr() = 0;

    virtual socklen_t getAddrLen() const = 0;

    // 将地址信息输出到流
    virtual std::ostream& insert(std::ostream& os)const = 0;
    std::string toString();

    bool operator<(const Address&rhs) const;
    bool operator==(const Address&rhs) const;
    bool operator!=(const Address&rhs) const;
};
// IP 地址抽象基类
class IPAddress: public Address
{
public:
    typedef std::shared_ptr<IPAddress>ptr;
    // 通过域名/服务器名/IP 创建地址
    static IPAddress::ptr Create(const char*address,uint16_t port=0);
    // 广播地址、网络地址、子网掩码
    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;
    virtual IPAddress::ptr networdAddress(uint32_t prefix_len) = 0;
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

    virtual uint16_t getPort()const = 0;
    virtual void setPort(uint16_t v) = 0;
};
// IPv4 地址
class IPv4Address:public IPAddress
{
public:
    typedef std::shared_ptr<IPv4Address>ptr;
    IPv4Address(const sockaddr_in&address);
    IPv4Address(uint32_t address = INADDR_ANY,uint16_t port=0);

    // 通过点分十进制 IP 创建 IPv4 地址
    static IPv4Address::ptr Create(const char*address,uint16_t port=0);

    virtual const sockaddr* getAddr()const override;
    virtual sockaddr* getAddr() override;
    virtual socklen_t getAddrLen() const override;
    virtual std::ostream& insert(std::ostream& os)const override;

    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    virtual IPAddress::ptr networdAddress(uint32_t prefix_len) override;
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len) override;
    virtual uint16_t getPort()const override;
    virtual void setPort(uint16_t v) override;
private:
    sockaddr_in m_addr; // 底层 IPv4 地址结构
};

// IPv6 地址
class IPv6Address : public IPAddress {
public:
    typedef std::shared_ptr<IPv6Address> ptr;
    static IPv6Address::ptr Create(const char* address, uint16_t port = 0);

    IPv6Address();

    IPv6Address(const sockaddr_in6& address);

    IPv6Address(const uint8_t address[16], uint16_t port = 0);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    IPAddress::ptr networdAddress(uint32_t prefix_len) override;
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;
    uint16_t getPort() const override;
    void setPort(uint16_t v) override;
private:
    sockaddr_in6 m_addr;
};

// Unix 域套接字地址
class UnixAddress : public Address {
public:
    typedef std::shared_ptr<UnixAddress> ptr;

    UnixAddress();

    UnixAddress(const std::string& path);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    void setAddrLen(uint32_t v);
    std::string getPath() const;
    std::ostream& insert(std::ostream& os) const override;
private:
    sockaddr_un m_addr;
    socklen_t m_length;
};


// 未知地址族
class UnknownAddress : public Address {
public:
    typedef std::shared_ptr<UnknownAddress> ptr;
    UnknownAddress(int family);
    UnknownAddress(const sockaddr& addr);
    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;
private:
    sockaddr m_addr;
};

// 流式输出地址（含端口）
std::ostream& operator<<(std::ostream&os,const Address&addr);
}