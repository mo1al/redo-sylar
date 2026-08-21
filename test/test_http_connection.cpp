#include <iostream>
#include "http_connection.h"
#include "log.h"
#include "iomanager.h"
#include "http_parser.h"
#include <fstream>

static mo1al::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_pool()
{
    mo1al::http::ConnectionPool::ptr pool(new mo1al::http::ConnectionPool("www.baidu.com","",80,10,1000*30,5));
    mo1al::IOManager::GetThis()->addTimer(1000,[pool](){
        auto r = pool->doGet("/blog",3000);
        SYLAR_LOG_INFO(g_logger) << r->toString();
    },true);
}

void run() {
    mo1al::Address::ptr addr = mo1al::Address::LookupAnyIPAddress("www.baidu.com:80");
    if(!addr) {
        SYLAR_LOG_INFO(g_logger) << "get addr error";
        return;
    }

    mo1al::Socket::ptr sock = mo1al::Socket::CreateTcp(addr);
    bool rt = sock->connect(addr);
    if(!rt) {
        SYLAR_LOG_INFO(g_logger) << "connect " << *addr << " failed";
        return;
    }

    mo1al::http::HttpConnection::ptr conn(new mo1al::http::HttpConnection(sock));
    mo1al::http::HttpRequest::ptr req(new mo1al::http::HttpRequest);
    req->setPath("/blog/");
    req->setHeader("host", "www.baidu.com");
    SYLAR_LOG_INFO(g_logger) << "req:" << std::endl
        << *req;

    conn->sendRequest(req);
    auto rsp = conn->recvResponse();

    if(!rsp) {
        SYLAR_LOG_INFO(g_logger) << "recv response error";
        return;
    }
    SYLAR_LOG_INFO(g_logger) << "rsp:" << std::endl
        << *rsp;

    // std::ofstream ofs("rsp.dat");
    // ofs << *rsp;

    SYLAR_LOG_INFO(g_logger) << "=========================";

    // auto r = mo1al::http::HttpConnection::DoGet("http://www.baidu.com", 3000);
    // SYLAR_LOG_INFO(g_logger) << "result=" << r->result
    //     << " error=" << r->error
    //     << " rsp= " << (r->response ? r->response->toString() : "");
    test_pool();
}

int main(int argc, char** argv) {
    mo1al::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
