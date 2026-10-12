#include"scheduler.h"
#include"macro.h"
#include"log.h"
#include"hook.h"
namespace mo1al
{
static thread_local Scheduler* t_scheduler = nullptr; // 当前线程所属的调度器
static thread_local Fiber* t_scheduler_fiber = nullptr; // 当前正在运行的协程
static thread_local Logger::ptr g_logger = SYLAR_LOG_NAME("system");
// 这里可以讲一讲 区分use_caller 创建调度器线程也去参与调度任务 
// 设计角度 mainfiber subfiber rootfiber 
Scheduler::Scheduler(size_t threads,bool use_caller,const std::string&name)
    : m_name(name)
{
    SYLAR_ASSERT(threads>0);
    if(use_caller)
    {
        // use_caller 模式下，主线程同时作为调度器的一个执行线程
        mo1al::Fiber::GetThis(); // 初始化主线程的主协程
        --threads;
        SYLAR_ASSERT(GetThis()==nullptr);
        t_scheduler = this;
        // 主线程的调度协程
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run,this),0,true)); // 调度协程
        t_scheduler_fiber = m_rootFiber.get();
        mo1al::Thread::SetName(m_name);
        m_rootThread = mo1al::getThreadId();
        // m_threadIds.push_back(m_rootThread);
    }
    else{
        m_rootThread = -1;
    }
    m_threadCount = threads;
}
Scheduler::~Scheduler()
{
    SYLAR_ASSERT(m_stopping);
    if(GetThis()==this)
    {
        t_scheduler = nullptr;
    }
}
Scheduler* Scheduler::GetThis()
{
    return t_scheduler;
}
Fiber* Scheduler::GetCurrentFiber()
{
    return t_scheduler_fiber;
}
void Scheduler::start()
{
    // 置停止标志并创建工作线程
    MutexType::Lock lock(m_mutex);
    if(m_stopping==false)
    {
        return ;
    }
    m_stopping = false;
    SYLAR_ASSERT(m_threads.empty());
    m_threads.resize(m_threadCount);
    for(size_t i=0;i<m_threadCount;i++){
        // run:idle/exec 
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run,this),m_name+"_"+std::to_string(i)));
        // m_threadIds.push_back(m_threads[i]->getId());
    }
}
void Scheduler::stop()
{
    m_autostop = true;
    // use_caller 且 仅主线程 参与调度
    if(m_rootFiber&&m_threadCount==0&&
        (m_rootFiber->GetState()==Fiber::INIT||m_rootFiber->GetState()==Fiber::TERM))
    {
        SYLAR_LOG_INFO(g_logger)<<this<<" Scheduler stop";
        m_stopping = true;
        if(stopping()){
            return ;
        }
    }
    if(m_rootThread!=-1) // use_caller == true
    {
        // use_caller 模式下由主线程负责销毁
        SYLAR_ASSERT(GetThis()==this);
    }
    else
    {
        SYLAR_ASSERT(GetThis()!=this);  // 非 use_caller 模式下须由其它线程触发销毁
    }
    m_stopping = true;
    // threadcount -- use_caller 
    for(size_t i=0;i<m_threadCount;i++)
    {
        // 唤醒了就去执行任务 
        tickle();
    }
    if(m_rootFiber)
    {
        tickle();
    }
    if(m_rootFiber){ // use_caller 模式下还有任务未完成
        if(!stopping()){ // 仍有任务，让调度协程继续执行
            m_rootFiber->call();
        }
    }
    // 回收全部工作线程
    std::vector<Thread::ptr>thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }
    for(auto&i:thrs)
    {
        i->join();
    }
}
void Scheduler::SetThis()
{
    t_scheduler = this;
}
void Scheduler::run()
{
    SYLAR_LOG_INFO(g_logger)<<"run  start";
    set_hook_enable(true);
    SetThis();
    // 非主线程需创建自身的主协程 因为只有主线程自己创建了子协程 
    if(mo1al::getThreadId()!=m_rootThread){
        t_scheduler_fiber = mo1al::Fiber::GetThis().get();
    }
    Fiber::ptr idlefiber(new Fiber(std::bind(&mo1al::Scheduler::idle,this)));
    Fiber::ptr cbfiber;
    FiberAndThread ft;
    while(true)
    {
        ft.reset(); // 每轮开始前重置任务
        bool tickle_me = false;
        bool is_active = false;
        {
            // 从任务队列取一个可执行任务
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            while(it!=m_fibers.end())
            {
                // 指定了线程且不是当前线程，跳过
                if(it->thread!=-1&&it->thread!=mo1al::getThreadId()){
                    ++it;
                    tickle_me = true;
                    continue;
                }
                SYLAR_ASSERT(it->fiber||it->cb);
                // 正在执行的协程跳过，待下一轮处理
                if(it->fiber&&it->fiber->GetState()==Fiber::EXEC){
                    ++it;
                    continue;
                }   
                ft = *it;
                it = m_fibers.erase(it); // erase 返回下一元素迭代器，不能再对已失效的 it 执行 ++（use-after-free）
                m_activeThreadCount++;
                is_active = true;
                break;
            }
        }
        if(tickle_me){ // 存在其它线程可执行的任务，唤醒空闲线程
            tickle();
        }
        // 执行取到的任务
        if(ft.fiber){
            if(ft.fiber->GetState()==Fiber::TERM||ft.fiber->GetState()==Fiber::EXCEPT){
                --m_activeThreadCount;
                continue;
            }
            ft.fiber->swapIn();
            --m_activeThreadCount;
            // 协程让出(swapOut)时仍为 EXEC，其目标状态记录在 m_nextState。
            // 这里在 swapIn 返回（上下文已保存）之后统一落定状态，
            // 使“上下文保存” happens-before“状态变为 HOLD/READY”，
            // 从而其它线程只有在上下文保存完成后才能看到并再次 swapIn 该协程，
            // 避免并发 swapcontext 破坏协程栈。
            if(ft.fiber->GetState()!=Fiber::TERM&&ft.fiber->GetState()!=Fiber::EXCEPT){
                ft.fiber->SetState(ft.fiber->m_nextState);
            }
            if(ft.fiber->GetState()==Fiber::READY){
                schedule(ft.fiber);
            }
            ft.reset(); // 执行完重置，防止悬空
        }
        else if(ft.cb){
            if(cbfiber){
                cbfiber->reset(ft.cb); // 复用协程，状态置为 INIT
            }
            else{
                cbfiber.reset(new Fiber(ft.cb)); // 首次创建协程
            }
            ft.reset();
            cbfiber->swapIn();
            --m_activeThreadCount;
            if(cbfiber->GetState()!=Fiber::TERM&&cbfiber->GetState()!=Fiber::EXCEPT){
                cbfiber->SetState(cbfiber->m_nextState); // 同 ft.fiber 分支：上下文保存后再落定状态
            }
            if(cbfiber->GetState()==Fiber::READY){
                schedule(cbfiber);
                cbfiber.reset();
            }
            else if(cbfiber->GetState()==Fiber::TERM||cbfiber->GetState()==Fiber::EXCEPT){
                cbfiber->reset(nullptr); // 复用栈空间
            }
            else{
                cbfiber.reset();
            }
        }
        else{
            // 无任务可执行，进入空闲协程
            if(is_active){
                --m_activeThreadCount;
                continue;
            }
            // 退出调度循环
            if(idlefiber->GetState()==Fiber::TERM||idlefiber->GetState()==Fiber::EXCEPT){
                SYLAR_LOG_INFO(g_logger)<<"idle term";
                break; 
            }
            ++m_idleThreadCount;
            idlefiber->swapIn();
            --m_idleThreadCount;
            if(idlefiber->GetState()!=Fiber::TERM&&idlefiber->GetState()!=Fiber::EXCEPT){
                idlefiber->SetState(Fiber::HOLD);
            }
        }
    }
}

void Scheduler::tickle()
{
    SYLAR_LOG_INFO(g_logger)<<"tickle";
}
bool Scheduler::stopping()
{
    MutexType::Lock lock(m_mutex);
    return m_autostop&&m_stopping&&m_fibers.empty()&&
        m_activeThreadCount==0;
}
void Scheduler::idle()
{
    // Scheduler的idle是配合mainfunc的 
    SYLAR_LOG_INFO(g_logger)<<"idle";
    while(!stopping()) {
        mo1al::Fiber::YieldToHold();
    }
}
}