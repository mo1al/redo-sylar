#include"hook.h"
#include"log.h"
#include"iomanager.h"
#include <sys/types.h>        
#include <sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<string.h>
static mo1al::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_sleep()
{
    // 阻塞的问题 实现的是异步
    mo1al::IOManager iom(1);
    // 并发任务
    iom.schedule([](){
        sleep(2);
        SYLAR_LOG_INFO(g_logger)<<"sleep 2"; // 应该是切换到该携程上下文来做这个
    });
    iom.schedule([](){
        sleep(3);
        SYLAR_LOG_INFO(g_logger)<<"sleep 3";
    });
    SYLAR_LOG_INFO(g_logger)<<"test sleep";
}
void test_sock()
{
    
    int sock = socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET,"183.2.172.177",&addr.sin_addr.s_addr);

    SYLAR_LOG_INFO(g_logger)<<" begin connect";
    int rt=1;
    rt = connect(sock,(const sockaddr*)&addr,sizeof(addr));
    SYLAR_LOG_INFO(g_logger)<<" connect rt1 = "<<rt<<" errno1 = "<<errno;
    if(rt)return ;

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    rt = send(sock,data,sizeof(data),0);
    SYLAR_LOG_INFO(g_logger) << " send rt=" << rt << " errno=" << errno;
    if(rt<=0)return ;

    std::string buff;
    buff.resize(4096);

    rt = recv(sock,&buff[0],buff.size(),0);
    SYLAR_LOG_INFO(g_logger) << " recv rt=" << rt << " errno=" << errno;
    if(rt<=0)return ;

    buff.resize(rt);
    SYLAR_LOG_INFO(g_logger) << " finish";    
}
int main()
{
    test_sleep();
    // test_hook();
    // mo1al::IOManager iom(1);
    // iom.schedule(test_sock);
    return 0;
}