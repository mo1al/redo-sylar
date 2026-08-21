#include "sylar.h"

static mo1al::Logger::ptr g_logger = SYLAR_LOG_ROOT();

// 注册各个路由
void registerServlet(mo1al::http::ServletDispatch::ptr dispatch)
{
    // 首页
    dispatch->addServlet("/", [](mo1al::http::HttpRequest::ptr req,
                                 mo1al::http::HttpResponse::ptr rsp,
                                 mo1al::http::HttpSession::ptr session) {
        rsp->setBody("Welcome to sylar HTTP server!\r\n");
        return 0;
    });

    // 回显请求内容
    dispatch->addServlet("/echo", [](mo1al::http::HttpRequest::ptr req,
                                     mo1al::http::HttpResponse::ptr rsp,
                                     mo1al::http::HttpSession::ptr session) {
        rsp->setBody(req->toString());
        return 0;
    });

    // 通配符路由：/sylar/ 下的所有路径
    dispatch->addGlobServlet("/sylar/*", [](mo1al::http::HttpRequest::ptr req,
                                            mo1al::http::HttpResponse::ptr rsp,
                                            mo1al::http::HttpSession::ptr session) {
        rsp->setBody("Glob matched: " + req->getPath() + "\r\n" + req->toString());
        return 0;
    });
}

// 服务器开启逻辑
void run(const std::string& ip, int port)
{
    // 1. 创建 HTTP 服务器(开启 keepalive)
    mo1al::http::HttpServer::ptr server(new mo1al::http::HttpServer(true));

    // 2. 组装监听地址并绑定
    mo1al::Address::ptr addr = mo1al::Address::LookupAny(ip + ":" + std::to_string(port));
    if(!addr)
    {
        SYLAR_LOG_ERROR(g_logger) << "invalid address: " << ip << ":" << port;
        return;
    }
    while(!server->bind(addr))
    {
        SYLAR_LOG_ERROR(g_logger) << "bind " << *addr << " fail, retry...";
        sleep(2);
    }

    // 3. 注册路由
    registerServlet(server->getServletDispatch());

    // 4. 启动服务器
    SYLAR_LOG_INFO(g_logger) << "server start on " << *addr;
    server->start();
}

int main(int argc, char** argv)
{
    std::string ip = "0.0.0.0";
    int port = 8020;
    if(argc > 1)
    {
        port = atoi(argv[1]);
    }

    mo1al::IOManager iom(2);
    iom.schedule(std::bind(run, ip, port));
    return 0;
}
