#include"http/servlet.h"
#include<fstream>
namespace mo1al
{
namespace http
{
// 首页显示
class ServiceServlet:public Servlet
{
public:
    typedef std::shared_ptr<ServiceServlet>ptr;
    ServiceServlet(const std::string&name);
    virtual int32_t handle(HttpRequest::ptr request,HttpResponse::ptr response,
            HttpSession::ptr session)override;
private:
    std::string m_name;
};
// C++ 知识点列表页
class CppServlet:public Servlet
{
public:
    typedef std::shared_ptr<CppServlet>ptr;
    CppServlet(const std::string&name);
    virtual int32_t handle(HttpRequest::ptr request,HttpResponse::ptr response,
            HttpSession::ptr session)override;
private:
    std::string m_name;
};
// QQ 二维码页
class QqServlet:public Servlet
{
public:
    typedef std::shared_ptr<QqServlet>ptr;
    QqServlet(const std::string&name);
    virtual int32_t handle(HttpRequest::ptr request,HttpResponse::ptr response,
            HttpSession::ptr session)override;
private:
    std::string m_name;
};
// 具体文章
class ArticleServlet:public Servlet
{
public:
    typedef std::shared_ptr<ArticleServlet>ptr;
    ArticleServlet(const std::string&name);
    virtual int32_t handle(HttpRequest::ptr request,HttpResponse::ptr response,
            HttpSession::ptr session)override;
private:
    std::string m_name;
};
// 静态资源（css/js/图片等）
class StaticFileServlet:public Servlet
{
public:
    typedef std::shared_ptr<StaticFileServlet>ptr;
    StaticFileServlet(const std::string&name);
    virtual int32_t handle(HttpRequest::ptr request,HttpResponse::ptr response,
            HttpSession::ptr session)override;
private:
    std::string m_name;
};
} // mo1al http
}