// fiber.h —— 协程：基于 ucontext 的用户态协程，支持切换、让出与状态管理。
#pragma once
#include<memory>
#include<functional>
#include<atomic>
#include<ucontext.h>
#include<thread.h>

namespace mo1al
{
class Scheduler;
class Fiber : public std::enable_shared_from_this<Fiber>
{
friend class Scheduler; // 允许调度器访问 Fiber 私有成员
public:
    typedef std::shared_ptr<Fiber>ptr;
    // 协程状态机 
    enum State
    {
        INIT, // 初始化
        HOLD, // 挂起
        EXEC, // 执行中
        TERM, // 终止
        READY, // 就绪
        EXCEPT  // 异常
    };
private:
    Fiber();
public:
    Fiber(std::function<void()>cb,size_t stacksize=0,bool user_caller=false);
    ~Fiber();
    // 重置协程回调与状态，复用已分配的栈空间
    void reset(std::function<void()>cb);
    // 切换到当前协程
    void swapIn();
    // 切回主协程
    void swapOut();
    // 由主协程切换到当前协程（use_caller 模式）
    void call();
    // 切回调用方协程（use_caller 模式）
    void back();
public:
    // 设置当前协程
    static void SetThis(Fiber*f);
    static Fiber::ptr GetThis();
    // 让出执行权并置为就绪态
    static void YieldToReady();
    // 让出执行权并置为挂起态
    static void YieldToHold();
    static uint64_t TotalFibers();
    static void MainFunc();
    static void CallerMainFunc();
    uint64_t getId()const{ return m_id;}
    static uint64_t GetFiberId();
    State GetState()const { return m_state.load(std::memory_order_acquire);}
    void SetState(State state) {m_state.store(state,std::memory_order_release);}
private:
    uint64_t m_id = 0;
    uint32_t m_stacksize = 0;
    ucontext_t m_ctx;
    std::atomic<State> m_state{INIT};
    // 协程让出(swapOut)后希望进入的状态。让出时协程仍为 EXEC，
    // 由调度器在 swapIn 返回后统一落定，保证“上下文保存”与“状态可见”有序，
    // 避免其它线程在协程尚未保存完上下文时提前 swapIn（并发 swapcontext 导致栈破坏）。
    State m_nextState{HOLD};
    void* m_stack = nullptr;
    std::function<void()>m_cb;
};
}