#include"tcpserver.h"
#include"log.h"
#include"iomanager.h"
mo1al::Logger::ptr g_logger = SYLAR_LOG_ROOT();
void test_tcpserver()
{
    mo1al::Address::ptr address = mo1al::Address::LookupAny("0.0.0.0:8080");
    std::vector<mo1al::Address::ptr>addrs;
    std::vector<mo1al::Address::ptr>fails;
    addrs.push_back(address);
    mo1al::TcpServer::ptr tcp(new mo1al::TcpServer());
    while(!tcp->bind(addrs,fails)){
        sleep(2);
    }
    tcp->start(); // 进去 
    sleep(100); // 主携程在睡 hook
    tcp->stop();
}
int main()
{
    mo1al::IOManager iom(2);
    iom.schedule(&test_tcpserver);
    return 0;
}