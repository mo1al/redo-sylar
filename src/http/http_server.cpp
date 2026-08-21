#include"http_server.h"
#include"log.h"
namespace mo1al
{
namespace http
{
static mo1al::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
HttpServer::HttpServer(bool keepalive,IOManager*work,IOManager*acceptwork)
    : m_isKeepalive(keepalive),m_dispatch(new ServletDispatch),TcpServer(work,acceptwork)
{

}
void HttpServer::handleClient(Socket::ptr client)
{
    http::HttpSession::ptr session(new http::HttpSession(client));
    do
    {
        auto req = session->recvRequest();
        if(!req){
            SYLAR_LOG_DEBUG(g_logger) << "recv http request fail, errno="
                << errno << " errstr=" << strerror(errno)
                << " cliet:" << *client << " keep_alive=" << m_isKeepalive;
            break;         
        }

        http::HttpResponse::ptr rsp(new http::HttpResponse
            (req->getVersion(),req->isClose()||!m_isKeepalive));
        m_dispatch->handle(req, rsp, session);
        session->sendResponse(rsp);
    } while (m_isKeepalive);
    session->close();
}
}
}