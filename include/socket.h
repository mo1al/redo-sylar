// socket.h —— Socket 封装：统一封装套接字的创建、绑定、连接与收发。
#pragma once
#include<memory>
#include"nocopyable.h"
#include"address.h"
namespace mo1al
{
// Socket 封装：提供套接字创建、连接、收发等接口
class Socket :public std::enable_shared_from_this<Socket>,public Nocopyable
{
public:
    typedef std::shared_ptr<Socket>ptr;
    typedef std::weak_ptr<Socket>weak_ptr;

    enum Type{
        TCP = SOCK_STREAM,
        UDP = SOCK_DGRAM
    };

    static Socket::ptr CreateTcp(Address::ptr address);
    static Socket::ptr CreateUdp(const Address::ptr addres);

    Socket(int family,int type,int protocol=0);
    virtual ~Socket();

    int64_t getSendTimeout();
    void setSendTimeout(int64_t v);
    int64_t getRecvTimeout();
    void setRecvTimeout(int64_t v);

    bool getoption(int level,int option,void*result,socklen_t*len);
    template<class T>
    bool getoption(int level,int option,T&result)
    {
        socklen_t length = sizeof(T);
        return getoption(level,option,&result,&length);
    }

    bool setoption(int level,int option,const void*result,socklen_t len);
    template<class T>
    bool setoption(int level,int option,const T&value)
    {
        return setoption(level,option,&value,sizeof(T));
    }

    Socket::ptr accept();
    virtual bool bind(const Address::ptr address);
    virtual bool connect(const Address::ptr,uint64_t timeout_ms=-1);
    virtual bool reconnect(uint64_t timeout_ms=-1);
    virtual bool listen(int backlog=SOMAXCONN);
    virtual bool close();

    virtual int send(const void*buffer,size_t length,int flags=0);
    virtual int send(const iovec* buffers, size_t length, int flags = 0);
    virtual int sendTo(const void* buffer, size_t length, const Address::ptr to, int flags = 0);
    virtual int sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags = 0);
    virtual int recv(void* buffer, size_t length, int flags = 0);
    virtual int recv(iovec* buffers, size_t length, int flags = 0);
    virtual int recvFrom(void* buffer, size_t length, Address::ptr from, int flags = 0);
    virtual int recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags = 0);

    Address::ptr getLocalAddress();
    Address::ptr getRemoteAddress();

    bool isValid()const;
    int getError();
    int getFamily() const { return m_family;}
    int getType() const { return m_type;}
    int getProtocol() const { return m_protocol;}
    int getSock()const{return m_sock;}
    bool isConnected() const { return m_isConnected;}

    virtual std::ostream& dump(std::ostream&os)const;
    virtual std::string toString()const;

    bool cancelRead();
    bool cancelAccept();
    bool cancelWrite();
    bool cancelAll();
protected:
    // 初始化 socket 选项
    void initSocket();
    // 创建底层 socket 描述符
    void newSocket();
    // 由已有 fd 初始化 Socket
    virtual bool init(int sock);
private:
    int m_sock; // socket 描述符
    int m_family;
    int m_type;
    int m_protocol;
    bool m_isConnected;
    Address::ptr m_localAddress;
    Address::ptr m_remoteAddress;
};
std::ostream& operator<<(std::ostream& os, const Socket& sock);
}