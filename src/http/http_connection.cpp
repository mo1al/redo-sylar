#include"http_connection.h"
#include<string>
#include"log.h"
namespace mo1al
{
namespace http
{
static Logger::ptr g_logger = SYLAR_LOG_NAME("system");
std::string HttpResult::toString()const
{
    std::stringstream ss;
    ss<<"[HttpResult result=" << result
       << " error=" << error
       << " response=" << (response ? response->toString() : "nullptr")
       << "]";
    return ss.str();
}

HttpConnection::HttpConnection(Socket::ptr sock,bool owner)
    :SocketStream(sock,owner)
{

}
HttpResponse::ptr HttpConnection::recvResponse()
{
    HttpResponseParser::ptr parser(new HttpResponseParser);
    uint64_t buffer_size = HttpResponseParser::GetHttpResponseBufferSize();
    std::shared_ptr<char>buffer(new char[buffer_size+1],[](char*ptr){
        delete[] ptr;
    });
    char* data = buffer.get();
    int offset = 0; // 缓冲区中尚未解析完的数据长度
    do{
        int len = read(data+offset,buffer_size-offset);
        if(len<=0){
            close();
            return nullptr;
        }
        len+=offset;
        data[len] = '\0';
        size_t nparser = parser->execute(data,len,false);
        offset = len-nparser;
        if(parser->hasError()){
            close();
            return nullptr;
        }
        if(offset==buffer_size){
            close();
            return nullptr;
        }
        if(parser->isFinished()){
            break;
        }
    }while(true);
    auto& client_parser = parser->getParser();
    if(client_parser.chunked){
        int len = offset;
        std::string body;
        do{
            do
            {
                int rt = read(data+len,buffer_size-len);
                if(rt<=0){
                    close();
                    return nullptr;
                }
                len += rt;
                data[len] = '\0';
                size_t npaser = parser->execute(data,len,true);
                if(parser->hasError()){
                    close();
                    return nullptr;
                }
                len -= npaser;
                if(len==buffer_size){ // 缓冲区已满但未解析完成，视为异常
                    close();
                    return nullptr;
                }
            }while(!parser->isFinished());
            len -= 2;
            if(client_parser.content_len<=len){ // 当前缓冲区已包含完整块数据
                body.append(data,client_parser.content_len);
                memmove(data,data+client_parser.content_len,len-client_parser.content_len);
                len -= client_parser.content_len;
            }   
            else{
                body.append(data,len);
                int left = client_parser.content_len-len;
                while(left>0){
                    int rt = read(data,left>buffer_size?buffer_size:left);
                    if(rt<=0){
                        close();
                        return nullptr;
                    }
                    body.append(data,rt);
                    left-=rt;
                }
                len = 0;
            }
        }while(!client_parser.chunks_done);
        parser->getData()->setBody(body);
    }
    else{
        int64_t length = parser->getContentLength();
        if(length>0){
            std::string body;
            body.resize(length);
            int len = 0; // 已拷贝到 body 的字节数
            if(length>offset)
            {
                memcpy(&body[0],data,offset);
                len = offset;
            }
            else{
                memcpy(&body[0],data,length);
                len = length;
            }
            length-=offset;
            if(length>0){
                if(readFixSize(&body[len],length)<=0){
                    close();
                    return nullptr;
                }
            }
            parser->getData()->setBody(body);
        }
    }
    return parser->getData();
}
int HttpConnection::sendRequest(HttpRequest::ptr rsp)
{
    std::stringstream ss;
    ss<<*rsp;
    std::string data = ss.str();
    return writeFixSize(data.c_str(),data.size());
}
HttpResult::ptr HttpConnection::DoGet(const std::string& url
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers
                            , const std::string& body) {
    Uri::ptr uri = Uri::Create(url);
    if(!uri) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL
                , nullptr, "invalid url: " + url);
    }
    return DoGet(uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoGet(Uri::ptr uri
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers
                            , const std::string& body) {
    return DoRequest(HttpMethod::GET, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoPost(const std::string& url
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers
                            , const std::string& body) {
    Uri::ptr uri = Uri::Create(url);
    if(!uri) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL
                , nullptr, "invalid url: " + url);
    }
    return DoPost(uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoPost(Uri::ptr uri
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers
                            , const std::string& body) {
    return DoRequest(HttpMethod::POST, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoRequest(HttpMethod method
                            , const std::string& url
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers
                            , const std::string& body) {
    Uri::ptr uri = Uri::Create(url);
    if(!uri) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL
                , nullptr, "invalid url: " + url);
    }
    return DoRequest(method, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoRequest(HttpMethod method
                            , Uri::ptr uri
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers
                            , const std::string& body) {
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setPath(uri->getPath());
    req->setQuery(uri->getQuery());
    req->setFragment(uri->getFragment());
    req->setMethod(method);
    bool has_host = false;
    for(auto& i : headers) {
        if(strcasecmp(i.first.c_str(), "connection") == 0) {
            if(strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                req->setClose(false);
            }
            continue;
        }

        if(!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
            has_host = !i.second.empty();
        }

        req->setHeader(i.first, i.second);
    }
    if(!has_host) {
        req->setHeader("Host", uri->getHost());
    }
    req->setBody(body);
    return DoRequest(req, uri, timeout_ms);
}

HttpResult::ptr HttpConnection::DoRequest(HttpRequest::ptr req
                            , Uri::ptr uri
                            , uint64_t timeout_ms) {
    Address::ptr addr = uri->createAddress();
    if(!addr) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_HOST
                , nullptr, "invalid host: " + uri->getHost());
    }
    Socket::ptr sock =  Socket::CreateTcp(addr);
    if(!sock) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::CREATE_SOCKET_ERROR
                , nullptr, "create socket fail: " + addr->toString()
                        + " errno=" + std::to_string(errno)
                        + " errstr=" + std::string(strerror(errno)));
    }
    if(!sock->connect(addr)) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::CONNECT_FAIL
                , nullptr, "connect fail: " + addr->toString());
    }
    sock->setRecvTimeout(timeout_ms);
    HttpConnection::ptr conn = std::make_shared<HttpConnection>(sock);
    int rt = conn->sendRequest(req);
    if(rt == 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER
                , nullptr, "send request closed by peer: " + addr->toString());
    }
    if(rt < 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR
                    , nullptr, "send request socket error errno=" + std::to_string(errno)
                    + " errstr=" + std::string(strerror(errno)));
    }
    auto rsp = conn->recvResponse();
    if(!rsp) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT
                    , nullptr, "recv response timeout: " + addr->toString()
                    + " timeout_ms:" + std::to_string(timeout_ms));
    }
    return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok");
}

HttpResult::ptr ConnectionPool::doGet(const std::string& url
                    , uint64_t timeout_ms
                    , const std::map<std::string, std::string>& headers
                    , const std::string& body )
{
    return doRequest(HttpMethod::GET, url, timeout_ms, headers, body);
}
HttpResult::ptr ConnectionPool::doGet(Uri::ptr uri
                    , uint64_t timeout_ms
                    , const std::map<std::string, std::string>& headers
                    , const std::string& body)
{
    std::stringstream ss;
    ss << uri->getPath()
       << (uri->getQuery().empty() ? "" : "?")
       << uri->getQuery()
       << (uri->getFragment().empty() ? "" : "#")
       << uri->getFragment();
    return doGet(ss.str(), timeout_ms, headers, body);
}
HttpResult::ptr ConnectionPool::doPost(const std::string& url
                    , uint64_t timeout_ms
                    , const std::map<std::string, std::string>& headers
                    , const std::string& body)
{
    return doRequest(HttpMethod::POST, url, timeout_ms, headers, body);
}
HttpResult::ptr ConnectionPool::doPost(Uri::ptr uri
                    , uint64_t timeout_ms
                    , const std::map<std::string, std::string>& headers
                    , const std::string& body )
{
    std::stringstream ss;
    ss<<uri->getPath()
        <<(uri->getQuery().empty()?"":"?")
        <<uri->getQuery()
        <<(uri->getFragment().empty()?"":"#")
        <<uri->getFragment();
    return doPost(ss.str(),timeout_ms,headers,body);
}
HttpResult::ptr ConnectionPool::doRequest(HttpMethod method
                    , const std::string& url
                    , uint64_t timeout_ms
                    , const std::map<std::string, std::string>& headers
                    , const std::string& body)
{
    // 构造请求
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setPath(url);
    req->setMethod(method);
    req->setClose(false);
    bool has_host = false;
    for(auto&i:headers)
    {
        if(strcasecmp(i.first.c_str(),"connection")==0){
            if(strcasecmp(i.second.c_str(),"keep-alive")==0){
                req->setClose(false);
                continue;
            }
        }
        if(!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
            has_host = !i.second.empty();
        }
        req->setHeader(i.first,i.second);
    }
    // 未指定 host 时使用默认值
    if(!has_host)
    {
        if(!m_vhost.empty()){
            req->setHeader("host",m_vhost);
        }
        else{
            req->setHeader("host",m_host);
        }
    }
    req->setBody(body);
    return doRequest(req,timeout_ms);
}
HttpResult::ptr ConnectionPool::doRequest(HttpMethod method
                    , Uri::ptr uri
                    , uint64_t timeout_ms
                    , const std::map<std::string, std::string>& headers
                    , const std::string& body)
{
    std::stringstream ss;
    ss << uri->getPath()
       << (uri->getQuery().empty() ? "" : "?")
       << uri->getQuery()
       << (uri->getFragment().empty() ? "" : "#")
       << uri->getFragment();
    return doRequest(method, ss.str(), timeout_ms, headers, body);
}

// 发送请求并接收响应，设置超时
HttpResult::ptr ConnectionPool::doRequest(HttpRequest::ptr req
                    , uint64_t timeout_ms)
{
    auto conn = getConnection(); 
    if(!conn){
        return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_GET_CONNECTION
                , nullptr, "pool host:" + m_host + " port:" + std::to_string(m_port));
    }
    auto sock = conn->getSocket();
    if(!sock){
        return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_INVALID_SOCKET
                , nullptr, "connection socket error pool host:" + m_host + " port:" + std::to_string(m_port));
    }
    sock->setSendTimeout(timeout_ms);
    auto rt = conn->sendRequest(req);
    if(rt == 0) { // 对端已关闭连接
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER
                , nullptr, "send request closed by peer: " + sock->getRemoteAddress()->toString());
    }
    if(rt < 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR
                    , nullptr, "send request socket error errno=" + std::to_string(errno)
                    + " errstr=" + std::string(strerror(errno)));
    }
    HttpResponse::ptr rsp = conn->recvResponse();
    if(!rsp){
        return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT
                    , nullptr, "recv response timeout: " + sock->getRemoteAddress()->toString()
                    + " timeout_ms:" + std::to_string(timeout_ms));  
    }
    return std::make_shared<HttpResult>((int)HttpResult::Error::OK,rsp,"ok");
}

HttpConnection::ptr ConnectionPool::getConnection()
{
    // 遍历连接池，剔除失效连接
    uint64_t now_ms = mo1al::GetCurrentMs();
    std::vector<HttpConnection*>invalid_vector;
    HttpConnection* cur = nullptr;
    Mutex::Lock lock(m_mutex);
    while(!m_conns.empty())
    {
        auto it =  *m_conns.begin();
        m_conns.pop_front();
        // 连接已断开
        if(!it->isConnected()){
            invalid_vector.push_back(it);
            continue;
        }
        if(it->m_createtime+m_maxalivetime>now_ms){
            invalid_vector.push_back(it);
            continue;
        }
        cur = it;
        break;
    }
    m_connnum-=invalid_vector.size();
    lock.unlock();
    for(auto& i:invalid_vector){
        delete i;
    }
    // 池中无可用连接则新建
    if(!cur)
    {
        IPAddress::ptr addr = Address::LookupAnyIPAddress(m_host);
        if(!addr){
            SYLAR_LOG_ERROR(g_logger) << "get addr fail: " << m_host;
            return nullptr;
        }
        addr->setPort(m_port);
        Socket::ptr sock = Socket::CreateTcp(addr);
        if(!sock){
            SYLAR_LOG_ERROR(g_logger) << "create sock fail: " << *addr;
            return nullptr;
        }
        if(!sock->connect(addr)){
            SYLAR_LOG_ERROR(g_logger) << "sock connect fail: " << *addr;
            return nullptr;
        }
        cur = new HttpConnection(sock);
        ++m_connnum;
    }   
    return HttpConnection::ptr(cur,std::bind(&ConnectionPool::releaseConnection,std::placeholders::_1,this));
}
void ConnectionPool::releaseConnection(HttpConnection* ptr,ConnectionPool* pool)
{
    ++ptr->m_request;
    if(!ptr->isConnected()||(ptr->m_createtime+pool->m_maxalivetime>mo1al::GetCurrentMs())||(ptr->m_request>=pool->m_maxrequests)){
        delete ptr;
        --pool->m_connnum;
        return ;
    }
    MutexType::Lock lock(pool->m_mutex);
    pool->m_conns.push_back(ptr);
}
ConnectionPool::ConnectionPool(const std::string&host,const std::string&vhost,uint32_t port,
        uint32_t max_size,uint32_t max_alivetime,uint32_t max_request)
    : m_host(host),m_vhost(vhost),m_port(port),m_maxsize(max_size),m_maxalivetime(max_alivetime),m_maxrequests(max_request)
{

}
HttpConnection::~HttpConnection()
{
    SYLAR_LOG_DEBUG(g_logger)<<"~HttpConnection";
}
}
}