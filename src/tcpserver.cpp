#include"tcpserver.h"
#include"log.h"
#include"config.h"
namespace mo1al
{
static Logger::ptr g_logger = SYLAR_LOG_ROOT();
static ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout =
    Config::LookUp("tcp_server.read_timeout",(uint64_t)(2*60*1000),"tcp server read timeout");

TcpServer::TcpServer(IOManager*work,IOManager*acceptwork)
    : m_work(work),m_acceptwork(acceptwork),m_isStop(true),m_name("sylar/1.0.0"),
        m_recvTimeout(g_tcp_server_read_timeout->getValue())
{

}
TcpServer::~TcpServer()
{
    for(auto&i:m_socks){
        i->close();
    }
    m_socks.clear();
}
bool TcpServer::bind(Address::ptr addr){
    std::vector<Address::ptr>addrs;
    std::vector<Address::ptr>fails;
    addrs.push_back(addr);
    return bind(addrs,fails);
}
bool TcpServer::bind(const std::vector<Address::ptr>& addrs
                        ,std::vector<Address::ptr>& fails)
{
    for(auto&addr:addrs){
        Socket::ptr sock = Socket::CreateTcp(addr);
        if(!sock->bind(addr)){
            SYLAR_LOG_ERROR(g_logger) << "bind fail errno="
                << errno << " errstr=" << strerror(errno)
                << " addr=[" << addr->toString() << "]";
            fails.push_back(addr);
            continue;
        }
        if(!sock->listen()){
            SYLAR_LOG_ERROR(g_logger) << "listen fail errno="
                << errno << " errstr=" << strerror(errno)
                << " addr=[" << addr->toString() << "]";
            fails.push_back(addr);
            continue;
        }
        m_socks.push_back(sock);
    }
    if(!fails.empty()){
        m_socks.clear();
        return false;
    }
    for(auto&i:m_socks)
    {
        SYLAR_LOG_INFO(g_logger) << " server bind success: "<<*i;
    }
    return true;
}
void TcpServer::startAccept(Socket::ptr sock)
{
    while(!m_isStop)
    {
        Socket::ptr client = sock->accept(); // 接受客户端连接
        if(client){
            client->setRecvTimeout(m_recvTimeout);
            m_work->schedule(std::bind(&TcpServer::handleClient,shared_from_this(),client));
        }
        else{
            SYLAR_LOG_ERROR(g_logger) << "accept errno=" << errno
                << " errstr=" << strerror(errno);
        }
    }
}
bool TcpServer::start()
{
    if(!m_isStop){
        return true;
    }
    m_isStop = false;
    for(auto&i:m_socks){
        m_acceptwork->schedule(std::bind(&TcpServer::startAccept,shared_from_this(),i));
    }
    return true;
}
void TcpServer::stop()
{
    m_isStop = true;
    for(auto&i:m_socks){
        i->cancelAll();
        i->close();
    }
    m_socks.clear();
}
// 默认的客户端处理逻辑（子类可重写）
void TcpServer::handleClient(Socket::ptr client)
{
    SYLAR_LOG_INFO(g_logger)<<"handleClient"<<*client;
}
}