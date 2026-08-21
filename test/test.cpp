#include"include/log.h"
#include"include/util.h"
#include<iostream>
#include<ctime>

int main(){
    // 1. 创建 logger
    auto logger = std::make_shared<mo1al::Logger>("root");

    // 2. 创建 appender 并设置 formatter
    auto appender = std::make_shared<mo1al::StdCoutAppender>();
    appender->setFormatter(std::make_shared<mo1al::LogFormatter>("%d%T%f%T%p%n"));
    logger->addAppender(appender);

    // 3. 创建日志事件
    auto event = std::make_shared<mo1al::LogEvent>(
        logger,
        mo1al::LogLevel::INFO,
        __FILE__, __LINE__,
        0, mo1al::getThreadId(),mo1al::getFiberId(),
        time(0),
        "thread-0"
    );

    // 4. 记录日志
    logger->log(mo1al::LogLevel::INFO, event);
    // std::cout<<"\n";
    std::cout << "hello mo1al!" << std::endl;
    SYLAR_LOG_INFO(logger);
    SYLAR_LOG_ERROR(logger);
    return 0;
}