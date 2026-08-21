// http_server.h —— HTTP 服务器：在 TCP 服务器之上处理 HTTP 请求。
#pragma once
#include"tcpserver.h"
#include"http_session.h"
#include"servlet.h"
namespace mo1al
{
namespace http
{
// HTTP 服务器：接受连接并按 Servlet 分发处理
class HttpServer:public TcpServer
{
public:
    typedef std::shared_ptr<HttpServer>ptr;

    HttpServer(bool keepalive=false,
        IOManager*work=IOManager::GetThis(),
        IOManager*acceptwork=IOManager::GetThis());

    ServletDispatch::ptr getServletDispatch() const { return m_dispatch;}
    void setServletDispatch(ServletDispatch::ptr v) { m_dispatch = v;}

protected:
    virtual void handleClient(Socket::ptr client)override;
private:
    bool m_isKeepalive;
    ServletDispatch::ptr m_dispatch;
};
}
}