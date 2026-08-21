// macro.h —— 常用宏定义：分支预测优化与断言。
#pragma once
#include<string.h>
#include<util.h>
#include<assert.h>

// 分支预测优化：SYLAR_LIKELY/SYLAR_UNLIKELY 提示编译器分支走向，利于流水线优化
#if defined __GNUC__ || defined __llvm__
# define SYLAR_LIKELY(x) __builtin_expect(!!(x),1)
# define SYLAR_UNLIKELY(x) __builtin_expect(!!(x),0)   // 提示编译器该分支为真的概率较低
#else
# define SYLAR_LIKELY(x) (x)
#define SYLAR_UNLIKELY(x) (x)
#endif

// 断言宏：条件不满足时记录错误日志、打印调用栈并触发断言
#define SYLAR_ASSERT(x)\
    if(SYLAR_UNLIKELY(!(x))){\
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())<<"ASSERTION : "#x\
            <<"\nbacktrace:\n"\
            << mo1al::BacktraceToString(10,2,"    ");\
            assert(x);\
    }
// 带附加信息的断言宏：失败时额外输出 f 的内容
#define SYLAR_ASSERT2(x,f)\
    if(SYLAR_UNLIKELY(!(x))){\
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())<<"ASSERTION : "#x\
            <<"\n"#f\
            <<"\nbacktrace:\n"\
            << mo1al::BacktraceToString(10,2,"    ");\
            assert(x);\
    }
