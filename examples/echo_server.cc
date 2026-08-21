#include"tcpserver.h"
#include"log.h"
#include"iomanager.h"
#include"bytearray.h"
#include"address.h"
#include"iomanager.h"
static mo1al::Logger::ptr g_logger = SYLAR_LOG_ROOT();
class EchoServer:public mo1al::TcpServer
{
public:
    typedef std::shared_ptr<EchoServer>ptr;
    EchoServer(int type){
        m_type =type;
    }
    void handleClient(mo1al::Socket::ptr client){
        SYLAR_LOG_INFO(g_logger) << "handleClient " << *client; 
        static int i=0;
        while(true){
            mo1al::ByteArray::ptr ba(new mo1al::ByteArray());
            std::vector<iovec>buffer;
            ba->getWriteBuffers(buffer,1024);
            //recv
            int rt = client->recv(&buffer[0],buffer.size());
            if(rt == 0) {
                SYLAR_LOG_INFO(g_logger) << "client close: " << *client;
                break;
            } else if(rt < 0) {
                SYLAR_LOG_INFO(g_logger) << "client error rt=" << rt
                    << " errno=" << errno << " errstr=" << strerror(errno);
                break;
            }
            ba->setPosition(ba->getPosition()+rt);
            ba->setPosition(0);
            if(m_type==1){
                // SYLAR_LOG_INFO(g_logger)<<ba->toString();
                std::cout<<ba->toString();
            }
            else{
                // SYLAR_LOG_INFO(g_logger)<<ba->toHexString();
                std::cout<<ba->toHexString();
            }
            std::cout.flush();
        }
    }
private:
    int m_type = 1;//text 2=bindary
};

void run()
{
    mo1al::Address::ptr address = mo1al::Address::LookupAny("0.0.0.0:8020");
    std::vector<mo1al::Address::ptr>addrs;
    std::vector<mo1al::Address::ptr>fails;
    addrs.push_back(address);
    EchoServer::ptr echo(new EchoServer(1));
    while(!echo->bind(addrs,fails))
    {
        sleep(2);
    }
    echo->start();
}

int main(char**argc,int argv)
{
    mo1al::IOManager iom(2);
    iom.schedule(&run);
    return 0;
}