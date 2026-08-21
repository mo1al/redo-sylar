#include"sylar.h"
#include"iomanager.h"
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<fcntl.h>
// #include<iostream>
using namespace std;
static mo1al::Logger::ptr g_logger = SYLAR_LOG_ROOT();
int sock = 0;
void test_fiber()
{
    
}
void test()
{
    mo1al::IOManager iom;
    iom.schedule(&test_fiber);
    sock = socket(AF_INET,SOCK_STREAM,0);
    fcntl(sock,F_SETFL,O_NONBLOCK);
    sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET,"183.2.172.177",&addr.sin_addr.s_addr);
    if(!connect(sock, (const sockaddr*)&addr, sizeof(addr))) 
    {

    } 
    else if(errno == EINPROGRESS) 
    {
        SYLAR_LOG_INFO(g_logger) << "add event errno=" << errno << ";" << strerror(errno);
        // 服务器端发数据才会变成可读 
        mo1al::IOManager::GetThis()->addEvent(sock, mo1al::IOManager::READ, [](){
            SYLAR_LOG_INFO(g_logger) << "read callback";
        });
        // tcp连接完成时 先变可写  socket 服务器端 <> 客户端(write)
        mo1al::IOManager::GetThis()->addEvent(sock, mo1al::IOManager::WRITE, [](){
            SYLAR_LOG_INFO(g_logger) << "write callback";
            //close(sock);
            mo1al::IOManager::GetThis()->cancelEvent(sock, mo1al::IOManager::READ); // readcallback
            close(sock);
        });
    } 
    else 
    {
        SYLAR_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    };
}
// void test1()
// {
//     mo1al::IOManager iom(2);
//     // 一起输出 
//     mo1al::Timer::ptr timer = iom.addTimer(1000,[timer](){
//         static int i = 0;
//         SYLAR_LOG_INFO(g_logger)<<"hello timer i:"<<i;
//         // if(++i==3)timer->cancel(); 
//         if(++i==3)timer->reset(2000,true); 
//     },true);
// } // 1.&timer是悬垂引用 2.是timer空的lambad闭包

mo1al::Timer::ptr s_timer;
void test_timer() {
    mo1al::IOManager iom(2);
    s_timer = iom.addTimer(1000, [](){
        static int i = 0;
        SYLAR_LOG_INFO(g_logger) << "hello timer i=" << i;
        if(++i == 3) {
            s_timer->reset(500, true);
            //s_timer->cancel();
        }
    }, true);
}
int main()
{
    // test1();
    test_timer();
    return 0;
}