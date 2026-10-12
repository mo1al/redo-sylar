#include"fiber.h"
#include"config.h"
#include"macro.h"
#include"scheduler.h"
#include<atomic>
namespace mo1al
{
static mo1al::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
static std::atomic<uint64_t> s_fiber_id{0};
static std::atomic<uint64_t> s_fiber_count{0};
// 指向当前线程的主协程
static thread_local Fiber::ptr t_thread_fiber = nullptr;
// 指向当前正在运行的协程
static thread_local Fiber* t_fiber = nullptr;
static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
    Config::LookUp<uint32_t>("fiber.stack_size",1024*1024,"fiber stack size"); // 协程栈大小配置项（默认 1MB）
// 协程栈内存分配器：基于 malloc/free
class MallocStackAllocator
{
public:
    static void* Alloc(size_t size)
    {
        return malloc(size);
    }
    static void Dealloc(void*vp,size_t size)
    {
        return free(vp);
    }
};   
using StackAllocator = MallocStackAllocator;
uint64_t Fiber::GetFiberId()
{
    if(t_fiber){
        return t_fiber->getId();
    }
    return 0;
}
// 主协程构造：仅由 GetThis 调用，无需分配独立栈
Fiber::Fiber()
{
    m_state = EXEC;
    SetThis(this); // 主协程即线程本身
    if(getcontext(&m_ctx)){
        SYLAR_ASSERT2(false,"getcontext error");
    }
    ++s_fiber_count;
    SYLAR_LOG_DEBUG(g_logger)<<"Fiber::Fiber() id : "<<m_id;
}
// 子协程构造：分配栈并绑定回调
Fiber::Fiber(std::function<void()>cb,size_t stacksize,bool use_caller)
: m_id(++s_fiber_id),m_cb(cb)
{
    ++s_fiber_count;
    m_stacksize = stacksize==0?g_fiber_stack_size->getValue():stacksize;
    m_stack = StackAllocator::Alloc(m_stacksize);
    if(getcontext(&m_ctx)){
        SYLAR_ASSERT2(false,"getcontext error");
    }
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    if(use_caller){
        makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
    }
    else{
        makecontext(&m_ctx,&Fiber::MainFunc,0); // 将上下文入口指向协程主函数
    }
    SYLAR_LOG_DEBUG(g_logger)<<"fiber(cb,stacksize) id : "<<m_id;
}
Fiber::~Fiber()
{
    --s_fiber_count;
    if(m_stack){
        SYLAR_ASSERT(m_state==TERM||m_state==INIT||m_state==EXCEPT); // 析构时协程必须已结束或从未运行
        StackAllocator::Dealloc(m_stack,m_stacksize);
    }
    else{
        SYLAR_ASSERT(!m_cb);
        SYLAR_ASSERT(m_state==EXEC);
        Fiber* cur = t_fiber;
        if(cur){
            SetThis(nullptr);
        }
    }
    SYLAR_LOG_DEBUG(g_logger)<<"~Fiber fiber id : "<<m_id;
}
// 重置协程回调与状态，复用已分配的栈空间
void Fiber::reset(std::function<void()>cb)
{
    SYLAR_ASSERT(m_stack);
    SYLAR_ASSERT(m_state==TERM||m_state==INIT||m_state==EXCEPT);
    if(getcontext(&m_ctx)){
        SYLAR_ASSERT2(false,"getcontext error");
    }
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    m_cb = cb;
    makecontext(&m_ctx,&Fiber::MainFunc,0);
    m_state = INIT;
    m_nextState = HOLD;
}
// 由主协程切换到当前协程（use_caller 模式）
void Fiber::call()
{
    SetThis(this);
    m_state = EXEC;
    if(swapcontext(&t_thread_fiber->m_ctx,&m_ctx)){
        SYLAR_ASSERT2(false,"swapcontext error");
    }
}
void Fiber::back()
{
    SetThis(t_thread_fiber.get());
    if(swapcontext(&m_ctx,&t_thread_fiber->m_ctx)){
        SYLAR_ASSERT2(false,"swapcontext error");
    }
}
// 主协程切换到当前子协程
void Fiber::swapIn()
{
    SetThis(this);
    SYLAR_ASSERT(m_state!=EXEC);
    m_state = EXEC;
    if(swapcontext(&(Scheduler::GetCurrentFiber()->m_ctx),&m_ctx)){
        SYLAR_ASSERT2(false,"swapcontext error");
    }
}
// 切回主协程，当前协程让出执行权
void Fiber::swapOut()
{
    SetThis(Scheduler::GetCurrentFiber());
    if(swapcontext(&m_ctx,&Scheduler::GetCurrentFiber()->m_ctx)){
        SYLAR_ASSERT2(false,"swapcontext error");
    }
}
void Fiber::SetThis(Fiber*f)
{
    t_fiber = f;
}
Fiber::ptr Fiber::GetThis()
{
    if(t_fiber){
        return t_fiber->shared_from_this();
    }
    Fiber::ptr main_fiber(new Fiber); // 构造主协程
    SYLAR_ASSERT(t_fiber==main_fiber.get());
    t_thread_fiber = main_fiber; // 记录主协程
    return t_fiber->shared_from_this();
}
// 让出执行权并置为就绪态
void Fiber::YieldToReady()
{
    Fiber::ptr cur = GetThis();
    cur->m_nextState = READY; // 记录目标状态，由调度器在 swapIn 返回后落定
    cur->swapOut();
}
// 让出执行权并置为挂起态
void Fiber::YieldToHold()
{
    Fiber::ptr cur = GetThis();
    cur->m_nextState = HOLD;  // 记录目标状态，由调度器在 swapIn 返回后落定
    cur->swapOut();
}
uint64_t Fiber::TotalFibers()
{
    return s_fiber_count;
}
void Fiber::MainFunc()
{
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur);
    try
    {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    }
    catch(std::exception&e)
    {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger)<<"Fiber Except"<<e.what();
    }
    catch(...)
    {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger)<<"Fiber Except";
    }
    auto raw = cur.get();
    cur.reset(); // 释放当前引用
    raw->swapOut();
    SYLAR_ASSERT2(false,"Never Reach");
}
void Fiber::CallerMainFunc()
{
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what();
    } catch (...) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except";
    }
    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->back();
    SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}
}