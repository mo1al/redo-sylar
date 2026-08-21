#pragma once
#include<memory>
#include<functional>
#include<ucontext.h>
#include<string>
// 携程 类
/*
成员变量
构造和析构 
接口 
*/
// 自己写了一遍->看代码补了一下->测试->ai改了一下 debug的一些知识
namespace mo1al
{
class Fiber : public std::enable_shared_from_this<Fiber>
{
public:
    typedef std::shared_ptr<Fiber>ptr;
    // 设置当前携程为正在运行的携程
    static void SetThis(Fiber* f); 
    // 获取当前正在运行的携程
    static Fiber* GetThis();
    void swapIn();
    void swapOut();
    void reset(std::function<void()>cb);
    static void yieldToHold();
    static void yieldToReady();
    static void MainFunc();
    enum State
    {
        INIT,EXEC,
        HOLD,TERM,READY,EXCEPT
    };
    ~Fiber();
    uint32_t getId()const {return m_fiberid;}
    static uint32_t GetFiberId();
private:    
    Fiber();
public:
    Fiber(std::function<void()>cb,size_t stacksize=0); 
private:
    void* m_stack=nullptr;
    size_t m_stacksize=0;
    ucontext_t m_ctx;
    std::function<void()>m_cb;
    State m_state=INIT;
    uint32_t m_fiberid=0;
};
    //
}
