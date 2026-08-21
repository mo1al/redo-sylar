// tcpserver.h —— TCP 服务器：管理监听套接字，接受并调度客户端连接。
#pragma once
#include"iomanager.h"
#include"socket.h"
#include"nocopyable.h"
#include<functional>
namespace mo1al
{
// TCP 服务器：监听端口并调度客户端连接处理
class TcpServer:public std::enable_shared_from_this<TcpServer>,public Nocopyable
{
public:
    typedef std::shared_ptr<TcpServer>ptr;

    TcpServer(IOManager*work=IOManager::GetThis(),
            IOManager*acceptwork=IOManager::GetThis());
    virtual ~TcpServer();
    // 绑定地址并监听
    virtual bool bind(Address::ptr addr);
    virtual bool bind(const std::vector<Address::ptr>&addrs,std::vector<Address::ptr>&fails);

    virtual bool start();
    virtual void stop();
    bool isStop()const{return m_isStop;}
    std::string getName()const{return m_name;}
    uint64_t getRecvTimeOut()const{return m_recvTimeout;}
    void setName(const std::string&name){m_name = name;}
    void setRecvTimeOut(uint64_t v){m_recvTimeout = v;}

    std::vector<Socket::ptr>getSocks()const{return m_socks;}
protected:
    virtual void handleClient(Socket::ptr client);
    virtual void startAccept(Socket::ptr sock);
private:
    std::vector<Socket::ptr>m_socks;
    IOManager* m_work;
    IOManager* m_acceptwork;
    std::string m_name;
    uint64_t m_recvTimeout;
    bool m_isStop;
};
}