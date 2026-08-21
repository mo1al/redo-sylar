// util.h —— 通用工具函数：线程/协程 ID 获取、调用栈回溯、时间戳获取。
#pragma once
#include<pthread.h>
#include"unistd.h"
#include"sys/types.h"
#include"sys/syscall.h"
#include<stdio.h>
#include<stdint.h>
#include<vector>
#include<string>
namespace mo1al{
    // 获取当前线程 ID
    pid_t getThreadId();
    // 获取当前协程 ID
    uint32_t getFiberId();
    // 获取调用栈，结果写入 bt，skip 为跳过的栈帧数
    void Backtrace(std::vector<std::string>&bt,int size,int skip=1);
    // 将调用栈格式化为字符串，prefix 为每行前缀
    std::string BacktraceToString(int size,int skip=2,const std::string&prefix="");
    // 获取当前时间戳（毫秒）
    uint64_t GetCurrentMs();
    // 获取当前时间戳（微秒）
    uint64_t GetCurrentUs();
}