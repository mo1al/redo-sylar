#include"http.h"
#include"log.h"
void test_http()
{
    mo1al::http::HttpRequest::ptr req(new mo1al::http::HttpRequest());
    req->setHeader("host","www.mo1al.com");
    req->setBody("hello mo1al");
    req->dump(std::cout)<<std::endl;

    mo1al::http::HttpResponse::ptr res(new mo1al::http::HttpResponse());
    res->setBody("hello mo1al");
    res->setHeader("X-X","mo1al");
    res->setStatus((mo1al::http::HttpStatus)400);
    res->setClose(false);

    res->dump(std::cout) << std::endl;
}
int main()
{   
    test_http();
    return 0;
}