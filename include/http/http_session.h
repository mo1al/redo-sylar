// http_session.h —— HTTP 会话：服务器端接收请求、发送响应。
#pragma once
#include<memory>
#include"socket_stream.h"
#include"http_parser.h"
#include"http.h"
namespace mo1al
{
namespace http
{
// HTTP 会话（服务器端）：封装请求接收与响应发送
class HttpSession:public SocketStream
{
public:
    typedef std::shared_ptr<HttpSession>ptr;
    HttpSession(Socket::ptr sock,bool owner=true);
    HttpRequest::ptr recvRequest();
    int sendResponse(HttpResponse::ptr rsp);
};
}
}