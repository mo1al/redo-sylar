// servlet.h —— Servlet 分发：按路径匹配并分发 HTTP 请求到处理函数。
#pragma once
#include"http.h"
#include"http_session.h"
#include"unordered_map"
#include"thread.h"
namespace mo1al
{
namespace http
{
// Servlet 基类：定义请求处理接口
class Servlet
{
public:
    typedef std::shared_ptr<Servlet>ptr;
    virtual ~Servlet(){}
    Servlet(std::string name):m_name(name){}
    virtual int32_t handle(HttpRequest::ptr request,HttpResponse::ptr response,
        HttpSession::ptr session) = 0;
    const std::string& getName()const {return m_name;}
private:
    std::string m_name;
};
// 函数式 Servlet：将回调函数包装为 Servlet
class FunctionServlet:public Servlet
{
public:
    typedef std::shared_ptr<FunctionServlet>ptr;
    typedef std::function<int32_t(HttpRequest::ptr request,HttpResponse::ptr response,
            HttpSession::ptr session)>callback;
    FunctionServlet(callback cb);
    virtual int32_t handle(HttpRequest::ptr request,HttpResponse::ptr HttpResponse,
            HttpSession::ptr session)override;
private:
    callback m_cb;
};
// Servlet 分发器：精确匹配 + glob 匹配 + 默认 Servlet
class ServletDispatch : Servlet
{
public:
    typedef std::shared_ptr<ServletDispatch>ptr;
    typedef RWmutex RWMutexType;
    ServletDispatch();
    virtual int32_t handle(HttpRequest::ptr request,HttpResponse::ptr response,
            HttpSession::ptr session)override;
public:
    void addServlet(const std::string& uri, Servlet::ptr slt);
    void addServlet(const std::string& uri, FunctionServlet::callback cb);

    void addGlobServlet(const std::string& uri, Servlet::ptr slt);
    void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);

    void delServlet(const std::string& uri);
    void delGlobServlet(const std::string& uri);

    Servlet::ptr getDefault() const { return m_default;}
    void setDefault(Servlet::ptr v) { m_default = v;}

    Servlet::ptr getServlet(const std::string& uri);
    Servlet::ptr getGlobServlet(const std::string& uri);
    Servlet::ptr getMatchedServlet(const std::string& uri);
private:
    std::unordered_map<std::string,Servlet::ptr>m_datas;
    std::vector<std::pair<std::string,Servlet::ptr>>m_globs;
    RWMutexType m_mutex;
    Servlet::ptr m_default;
};
// 404 默认 Servlet
class NotFoundServlet : public Servlet
{
public:
    typedef std::shared_ptr<NotFoundServlet>ptr;
    NotFoundServlet(const std::string&name);
    virtual int32_t handle(HttpRequest::ptr request,HttpResponse::ptr response,
            HttpSession::ptr session)override;
private:
    std::string m_name;
    std::string m_content;
};
}
}