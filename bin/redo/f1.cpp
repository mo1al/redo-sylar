#include"f1.h"
#include"log.h"
#include"config.h"
#include"macro.h"
#include<atomic>
// 日志 判断
namespace mo1al
{

static ConfigVar<uint32_t>::ptr g_config = Config::LookUp<uint32_t>("fiber.stack_size",128*1024,"fiber stack size");
static mo1al::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
static std::atomic<uint32_t>s_fiber_count{0};
static std::atomic<uint32_t>s_fiber_id{0};
static thread_local Fiber* t_fiber = nullptr; // 当前运行的携程
static thread_local Fiber::ptr t_thread_fiber = nullptr; // 主携程

class MemoryAlloctor
{
public:
    static void* alloc(size_t size)
    {
        return malloc(size);
    }
    static void dealloc(void*pt,size_t size)
    {
        free(pt);
    }
};
using Alloctor = MemoryAlloctor;
uint32_t Fiber::GetFiberId()
{
    if(t_fiber)
    {
        return t_fiber->getId();
    }
    return 0;
}
Fiber::Fiber()
{
    m_state = EXEC; // 显示执行中 
    SetThis(this);
    // t_thread_fiber = this->shared_from_this();
    s_fiber_count++;
    // 携程上下文 
    if(getcontext(&m_ctx)){
        SYLAR_ASSERT2(false,"GetContext error");
    }
    SYLAR_LOG_DEBUG(g_logger)<<"Fiber() id : "<<m_fiberid;
}
Fiber::Fiber(std::function<void()>cb,size_t stacksize)
    : m_cb(cb),m_fiberid(++s_fiber_id)
{
    ++s_fiber_count;
    m_stacksize = stacksize==0?g_config->getValue():stacksize; // 1m
    m_stack = Alloctor::alloc(m_stacksize);
    if(getcontext(&m_ctx)){
        SYLAR_ASSERT2(false,"GetContext error");       
    }
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_size = m_stacksize; // 这里写了个0
    m_ctx.uc_stack.ss_sp = m_stack;
    makecontext(&m_ctx,&MainFunc,0);
    SYLAR_LOG_DEBUG(g_logger)<<"Fiber::Fiber(cb,stacksize) id : "<<m_fiberid;
}
Fiber::~Fiber()
{
    --s_fiber_count;
    // 分两种析构 
    if(m_stack)
    {
        // 结束或异常那就回收内存
        SYLAR_ASSERT(m_state==INIT||m_state==TERM||m_state==EXCEPT);
        Alloctor::dealloc(m_stack,m_stacksize);
    }
    else
    {
        SYLAR_ASSERT(!m_cb);
        SYLAR_ASSERT(m_state==EXEC);
        Fiber* cur = t_fiber;
        if(cur){
            SetThis(nullptr);
        }
    }
    SYLAR_LOG_DEBUG(g_logger)<<"~Fiber() id : "<<m_fiberid;
    // 析构就不用设置状态位了 
}
void Fiber::reset(std::function<void()>cb)
{
    SYLAR_ASSERT(m_stack);
    SYLAR_ASSERT(m_state==INIT||m_state==TERM||m_state==EXCEPT);
    if(getcontext(&m_ctx)){
        SYLAR_ASSERT2(false,"getcontext error");
    }
    m_ctx.uc_link=nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    m_cb = cb;
    makecontext(&m_ctx,&MainFunc,0); 
    m_state = INIT;
}
void Fiber::SetThis(Fiber* f)
{
    t_fiber = f;
}
// 用法上没有问题 但是写出了很大的问题
Fiber* Fiber::GetThis()
{
    if(t_fiber){
        return t_fiber;
    }
    //创造一个主携程 并返回出去
    Fiber::ptr main_fiber(new Fiber());
    SYLAR_ASSERT(t_fiber = main_fiber.get());
    t_thread_fiber = main_fiber; 
    // main_fiber.reset(); // 局部变量自析构 冗余了代码
    return t_fiber; // 源码返回的是智能指针 我这里返回了裸指针 
    // 原因是 我不共享所有权 我只是给一个访问权
}
// 只实现了主携程到子携程 但是子携程内部也要切换
void Fiber::swapIn()
{   
    Fiber* pre = t_fiber;
    SetThis(this);
    SYLAR_ASSERT(m_state!=EXEC);
    m_state = EXEC;
    if(swapcontext(&pre->m_ctx,&m_ctx)) 
    {
        SYLAR_ASSERT2(false,"swapcontext error");
    }
}
// 子携程到主携程
void Fiber::swapOut()
{
    // SYLAR_ASSERT(m_state==EXEC);
    SetThis(t_thread_fiber.get());
    if(swapcontext(&m_ctx,&t_thread_fiber->m_ctx))
    {
        SYLAR_ASSERT2(false,"swapcontext error");
    }
}
void Fiber::MainFunc()
{
    Fiber::ptr cur = t_fiber->shared_from_this();
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
    // 切换携程 
    Fiber* raw = cur.get();
    cur.reset();
    raw->swapOut();
} 
void Fiber::yieldToHold()
{
    t_fiber->m_state = HOLD;
    t_fiber->swapOut();
}
void Fiber::yieldToReady()
{
    t_fiber->m_state = READY;
    t_fiber->swapOut();
}
    //
}