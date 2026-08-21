// http_parser.h —— HTTP 解析器封装：包装 ragel 生成的请求/响应解析器。
#pragma once
#include"http.h"
#include"httpclient_parser.h"
#include"http11_parser.h"
#include<memory>
namespace mo1al
{
namespace http
{
// HTTP 请求解析器：封装 ragel 请求解析器
class HttpRequestParser
{
public:
    typedef std::shared_ptr<HttpRequestParser>ptr;
    HttpRequestParser();
    size_t execute(char*data,size_t len);
    int isFinished();
    int hasError();
    void setError(int v){ m_error = v;}
    HttpRequest::ptr getData()const{return m_data;}
    uint64_t getContentLength();
    const http_parser& getParser()const {return m_parser;}
public:
    static uint64_t GetRequestBufferSize();
    static uint64_t GetRequestBodySize();
private:
    http_parser m_parser;
    HttpRequest::ptr m_data;
    // 1001 无效版本 / 1002 无效字段
    int m_error ;
};

// HTTP 响应解析器：封装 ragel 响应解析器
class HttpResponseParser
{
public:
    typedef std::shared_ptr<HttpResponseParser>ptr;
    HttpResponseParser();
    size_t execute(char*data,size_t len,bool chunked);
    int isFinished();
    int hasError();
    void setError(int v){m_error=v;}
    HttpResponse::ptr getData()const{return m_data;}
    uint64_t getContentLength();
    const httpclient_parser& getParser()const {return m_parser;}
public:
    static uint64_t GetHttpResponseBufferSize();
    static uint64_t GetHttpResponseMaxBodySize();
private:
    httpclient_parser m_parser;
    HttpResponse::ptr m_data;
    // 1000 无效方法
    int m_error;
};
}
}