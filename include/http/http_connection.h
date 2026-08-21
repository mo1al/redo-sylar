// http_connection.h —— HTTP 客户端连接与连接池。
#pragma once
#include<memory>
#include<list>
#include<atomic>
#include<string>
#include"thread.h"
#include"socket_stream.h"
#include"http_parser.h"
#include"http.h"
#include"uri.h"
namespace mo1al
{
namespace http
{
// HTTP 请求结果：包含错误码、错误信息与响应
struct HttpResult
{
    typedef std::shared_ptr<HttpResult>ptr;
    // 错误码
    enum class Error{
        OK = 0,
        // 非法 URL
        INVALID_URL = 1,
        // 无法解析 HOST
        INVALID_HOST = 2,
        // 连接失败
        CONNECT_FAIL = 3,
        // 连接被对端关闭
        SEND_CLOSE_BY_PEER = 4,
        // 发送请求产生 Socket 错误
        SEND_SOCKET_ERROR = 5,
        // 超时
        TIMEOUT = 6,
        // 创建 Socket 失败
        CREATE_SOCKET_ERROR = 7,
        // 从连接池中取连接失败
        POOL_GET_CONNECTION = 8,
        // 无效的连接
        POOL_INVALID_CONNECTION = 9,
        // 无效的 Socket
        POOL_INVALID_SOCKET = 10,
    };
    int result;
    std::string error;
    HttpResponse::ptr response;
    HttpResult(int _result,HttpResponse::ptr _response
        ,std::string _error)
        :result(_result),response(_response),error(_error)
    {}
    std::string toString()const;
};
class ConnectionPool;
// HTTP 连接（客户端方）
class HttpConnection:public SocketStream
{
friend class ConnectionPool;
public:
    typedef std::shared_ptr<HttpConnection>ptr;
    HttpConnection(Socket::ptr sock,bool owner=true);
    ~HttpConnection();
    HttpResponse::ptr recvResponse();
    int sendRequest(HttpRequest::ptr rsp);
private:
    uint64_t m_createtime=0;
    uint64_t m_request=0;
public:
    static HttpResult::ptr DoGet(const std::string& url
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers = {}
                            , const std::string& body = "");

    static HttpResult::ptr DoGet(Uri::ptr uri
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers = {}
                            , const std::string& body = "");

    static HttpResult::ptr DoPost(const std::string& url
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers = {}
                            , const std::string& body = "");

    static HttpResult::ptr DoPost(Uri::ptr uri
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers = {}
                            , const std::string& body = "");

    static HttpResult::ptr DoRequest(HttpMethod method
                            , const std::string& url
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers = {}
                            , const std::string& body = "");

    static HttpResult::ptr DoRequest(HttpMethod method
                            , Uri::ptr uri
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers = {}
                            , const std::string& body = "");

    static HttpResult::ptr DoRequest(HttpRequest::ptr req
                            , Uri::ptr uri
                            , uint64_t timeout_ms);

};

// HTTP 连接池：复用客户端连接以提升效率
class ConnectionPool
{
public:
    typedef std::shared_ptr<ConnectionPool>ptr;
    typedef Mutex MutexType;
    ConnectionPool(const std::string&host,const std::string&vhost,uint32_t port,
        uint32_t max_size,uint32_t max_alivetime,uint32_t max_request);
    HttpConnection::ptr getConnection();
    static void releaseConnection(HttpConnection* ptr,ConnectionPool*pool);
public:
    HttpResult::ptr doGet(const std::string& url
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers = {}
                            , const std::string& body = "");

    HttpResult::ptr doGet(Uri::ptr uri
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers = {}
                            , const std::string& body = "");

    HttpResult::ptr doPost(const std::string& url
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers = {}
                            , const std::string& body = "");

    HttpResult::ptr doPost(Uri::ptr uri
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers = {}
                            , const std::string& body = "");

    HttpResult::ptr doRequest(HttpMethod method
                            , const std::string& url
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers = {}
                            , const std::string& body = "");

    HttpResult::ptr doRequest(HttpMethod method
                            , Uri::ptr uri
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers = {}
                            , const std::string& body = "");

    HttpResult::ptr doRequest(HttpRequest::ptr req
                            , uint64_t timeout_ms);
private:
    std::string m_host;
    std::string m_vhost;
    uint32_t m_port;
    std::list<HttpConnection*>m_conns;
    uint32_t m_maxsize;
    uint32_t m_maxalivetime;
    uint32_t m_maxrequests;
    std::atomic<int>m_connnum{0};
    MutexType m_mutex;
};




}
}