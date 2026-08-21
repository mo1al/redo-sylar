#include"http_server.h"
#include"address.h"
#include"log.h"
static mo1al::Logger::ptr g_logger = SYLAR_LOG_ROOT();
void test_server()
{
    mo1al::http::HttpServer::ptr server(new mo1al::http::HttpServer());
    mo1al::Address::ptr address = mo1al::Address::LookupAny("0.0.0.0:8020");
    while(!server->bind(address))
    {
        sleep(2);
    }
    auto sd = server->getServletDispatch();
    sd->addServlet("/sylar/xx", [](mo1al::http::HttpRequest::ptr req
                ,mo1al::http::HttpResponse::ptr rsp
                ,mo1al::http::HttpSession::ptr session) {
            rsp->setBody(req->toString());
            return 0;
    });

    sd->addGlobServlet("/sylar/*", [](mo1al::http::HttpRequest::ptr req
                ,mo1al::http::HttpResponse::ptr rsp
                ,mo1al::http::HttpSession::ptr session) {
            rsp->setBody("Glob:\r\n" + req->toString());
            return 0;
    });
    server->start();
}
int main()
{
    mo1al::IOManager iom(2);
    iom.schedule(test_server);
    return 0;
}