#include"socket.h"
#include"log.h"
#include"address.h"
#include"iomanager.h"
static mo1al::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
void test_socket()
{
    // client
    mo1al::IPAddress::ptr address = mo1al::Address::LookupAnyIPAddress("www.baidu.com");
    if(address) {
        SYLAR_LOG_INFO(g_logger) << "get address: " << address->toString();
    } else {
        SYLAR_LOG_ERROR(g_logger) << "get address fail";
        return;
    }
    mo1al::Socket::ptr sock = mo1al::Socket::CreateTcp(address);
    address->setPort(80);
    SYLAR_LOG_INFO(g_logger) << "addr=" << address->toString();
    if(!sock->connect(address)) {
        SYLAR_LOG_ERROR(g_logger) << "connect " << address->toString() << " fail";
        return;
    } else {
        SYLAR_LOG_INFO(g_logger) << "connect " << address->toString() << " connected";
    }

    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff, sizeof(buff));
    if(rt <= 0) {
        SYLAR_LOG_INFO(g_logger) << "send fail rt=" << rt;
        return;
    }

    std::string buffs;
    buffs.resize(4096);
    rt = sock->recv(&buffs[0], buffs.size());

    if(rt <= 0) {
        SYLAR_LOG_INFO(g_logger) << "recv fail rt=" << rt;
        return;
    }

    // buffs.resize(rt);
    // SYLAR_LOG_INFO(g_logger) << buffs;
}
int main()
{
    mo1al::IOManager iom(1);
    iom.schedule(&test_socket);
    return 0;
}