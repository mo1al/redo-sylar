#include "address.h"
#include "log.h"

mo1al::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test() {
    std::vector<mo1al::Address::ptr> addrs;

    SYLAR_LOG_INFO(g_logger) << "begin";
    bool v = mo1al::Address::Lookup(addrs, "www.baidu.com:http");
    SYLAR_LOG_INFO(g_logger) << "end";
    if(!v) {
        SYLAR_LOG_ERROR(g_logger) << "lookup fail";
        return;
    }
    for(size_t i = 0; i < addrs.size(); ++i) {
        SYLAR_LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
    }
}
void test_ipv4() {
    auto addr = mo1al::IPAddress::Create("127.0.0.8");
    if(addr) {
        SYLAR_LOG_INFO(g_logger) << addr->toString();
    }
}

int main(int argc, char** argv) {
    test_ipv4();
    //test_iface();
    // test();
    return 0;
}
