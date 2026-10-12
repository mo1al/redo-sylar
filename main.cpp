#include"sylar.h"
#include"service.h"
mo1al::Logger::ptr g_logger = SYLAR_LOG_ROOT();
void run()
{
    mo1al::Address::ptr address = mo1al::Address::LookupAnyIPAddress("0.0.0.0:8020");
    if(!address){
        SYLAR_LOG_ERROR(g_logger)<<"get address error";
    }
    mo1al::http::HttpServer::ptr http_server(new mo1al::http::HttpServer());
    while(!http_server->bind(address))
    {
        SYLAR_LOG_ERROR(g_logger)<<"bind"<<*address<<"fail";
        sleep(1);
    }
    mo1al::http::Servlet::ptr slt = std::make_shared<mo1al::http::ServiceServlet>("blog");
    mo1al::http::Servlet::ptr slt2 = std::make_shared<mo1al::http::ArticleServlet>("article");
    http_server->getServletDispatch()->addServlet("/",slt);
    mo1al::http::Servlet::ptr slt4 = std::make_shared<mo1al::http::CppServlet>("cpp");
    http_server->getServletDispatch()->addServlet("/cpp",slt4);
    mo1al::http::Servlet::ptr slt5 = std::make_shared<mo1al::http::QqServlet>("qq");
    http_server->getServletDispatch()->addServlet("/qq",slt5);
    // http_server->getServletDispatch()->addServlet("/post/",slt2);
    http_server->getServletDispatch()->addGlobServlet("/post/*",slt2);
    mo1al::http::Servlet::ptr slt3 = std::make_shared<mo1al::http::StaticFileServlet>("static");
    http_server->getServletDispatch()->addGlobServlet("/static/*",slt3);
    http_server->start();
}
int main()
{
    g_logger->setLevel(mo1al::LogLevel::INFO);
    mo1al::IOManager iom(3);
    iom.schedule(run);
    return 0;
}