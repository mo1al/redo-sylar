#include"iomanager.h"
#include"macro.h"
#include"log.h"
#include"fcntl.h"
#include"unistd.h"
#include<sys/epoll.h>
static mo1al::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
namespace mo1al
{
IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event)
{
    switch (event)
    {
    case IOManager::READ:
        return read;
    case IOManager::WRITE:
        return write;
    default:
        SYLAR_ASSERT2(false,"getcontext");
    }
}
void IOManager::FdContext::resetContext(IOManager::FdContext::EventContext& ctx)
{
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}
void IOManager::FdContext::triggerEvent(IOManager::Event event)
{
    SYLAR_ASSERT(events&event);
    events = (Event)(events&~event); // 触发后清除该事件标志
    EventContext& ctx = getContext(event);
    if(ctx.cb){
        ctx.scheduler->schedule(&ctx.cb);
    }
    else if(ctx.fiber){
        ctx.scheduler->schedule(&ctx.fiber);
    }
    ctx.scheduler = nullptr;
    return ;
}
IOManager::IOManager(size_t threads,bool use_caller,const std::string&name)
    : Scheduler(threads,use_caller,name)
{
    // 使用管道实现跨线程唤醒机制
    m_epfd = epoll_create(5000);
    SYLAR_ASSERT(m_epfd>0);
    int rt = pipe(m_ticklefds); // 创建唤醒管道
    SYLAR_ASSERT(!rt);
    epoll_event event;
    memset(&event,0,sizeof(epoll_event));
    event.events = EPOLLIN | EPOLLET; // 监听管道读端（边沿触发）
    event.data.fd = m_ticklefds[0];
    rt = fcntl(m_ticklefds[0],F_SETFL,O_NONBLOCK); // 设置读端为非阻塞
    SYLAR_ASSERT(!rt);
    rt = epoll_ctl(m_epfd,EPOLL_CTL_ADD,m_ticklefds[0],&event); // 将管道读端加入 epoll
    SYLAR_ASSERT(!rt);
    resizeContext(32);
    Scheduler::start();
}
IOManager::~IOManager()
{
    Scheduler::stop();
    close(m_epfd);
    close(m_ticklefds[0]);
    close(m_ticklefds[1]);
    for(size_t i=0;i<m_fdcontext.size();i++)
    {
        if(m_fdcontext[i]){
            delete m_fdcontext[i];
        }
    }
}
void IOManager::resizeContext(int size)
{
    m_fdcontext.resize(size);
    for(size_t i=0;i<m_fdcontext.size();i++)
    {
        if(!m_fdcontext[i])
        {
            m_fdcontext[i] = new FdContext();
            m_fdcontext[i]->fd = i;
        }
    }
}
int IOManager::addEvent(int fd,Event event,std::function<void()>cb)
{
    FdContext* ctx = nullptr;
    MutexType::ReadLock lock1(m_mutex);
    if((int)m_fdcontext.size()>fd){
        ctx = m_fdcontext[fd];
        lock1.unlock();
    }
    else{
        lock1.unlock();
        MutexType::WriteLock lock2(m_mutex);
        resizeContext(fd*1.5);
        ctx = m_fdcontext[fd];
    }
    FdContext::MutexType::Lock lock(ctx->mutex);
    if(ctx->events&event){
        SYLAR_LOG_ERROR(g_logger)<<"addevent assert fd="<<fd
                    << " event="<<(EPOLL_EVENTS)event
                    << " fd_ctx.event="<<(EPOLL_EVENTS)ctx->events;
        SYLAR_ASSERT(!(ctx->events&event));
    }

    int op = ctx->events?EPOLL_CTL_MOD:EPOLL_CTL_ADD;
    epoll_event epevent;
    epevent.events = EPOLLET | ctx->events | event;
    epevent.data.ptr = ctx; // 事件就绪时通过该指针定位 FdContext

    int rt = epoll_ctl(m_epfd,op,fd,&epevent); // 注册/修改 fd 的 epoll 监听
    if(rt){
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
            << (EPOLL_EVENTS)ctx->events;
        return -1;
    }

    ++m_pendingEventCount; // 待处理事件计数加一
    ctx->events = (Event)(event|ctx->events);
    FdContext::EventContext& event_ctx = ctx->getContext(event); // 获取该事件对应的上下文
    SYLAR_ASSERT(!event_ctx.fiber&&!event_ctx.cb&&!event_ctx.scheduler);
    event_ctx.scheduler = Scheduler::GetThis();
    if(cb){
        event_ctx.cb.swap(cb);
    }
    else{
        event_ctx.fiber = Fiber::GetThis();
        SYLAR_ASSERT2(event_ctx.fiber->GetState()==Fiber::EXEC,"state = "
        <<event_ctx.fiber->GetState());
    }
    return 0;
}

bool IOManager::delEvent(int fd,Event event)
{
    FdContext* fd_ctx = nullptr;
    MutexType::ReadLock lock1(m_mutex);
    if((int)m_fdcontext.size()<=fd){
        return false;
    }
    fd_ctx = m_fdcontext[fd];
    lock1.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    Event new_event = (Event)(fd_ctx->events&~event); // 移除目标事件后的事件集合
    int op = new_event?EPOLL_CTL_MOD:EPOLL_CTL_DEL;

    epoll_event epevent;
    epevent.events = EPOLLET|new_event;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd,op,fd,&epevent); // 更新 epoll 监听
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    --m_pendingEventCount;
    fd_ctx->events = new_event;
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);
    return true;
}
bool IOManager::cancelEvent(int fd,Event event)
{
    FdContext* fd_ctx = nullptr;
    MutexType::ReadLock lock1(m_mutex);
    if((int)m_fdcontext.size()<=fd){
        return false;
    }
    fd_ctx = m_fdcontext[fd];
    lock1.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    Event new_event = (Event)(fd_ctx->events&~event); // 移除目标事件后的事件集合
    int op = new_event?EPOLL_CTL_MOD:EPOLL_CTL_DEL;

    epoll_event epevent;
    epevent.events = EPOLLET|new_event;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd,op,fd,&epevent); // 更新 epoll 监听
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;
    return true;
}
bool IOManager::cancelAll(int fd)
{
    FdContext* fd_ctx = nullptr;
    MutexType::ReadLock lock1(m_mutex);
    if((int)m_fdcontext.size()<=fd){
        return false;
    }
    fd_ctx = m_fdcontext[fd];
    lock1.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!fd_ctx->events){
        return false;
    }
    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd,op,fd,&epevent); // 更新 epoll 监听
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }
    if(fd_ctx->events & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }
    if(fd_ctx->events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }
    SYLAR_ASSERT(fd_ctx->events == 0);
    return true;
}
IOManager* IOManager::GetThis()
{
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}
void IOManager::tickle(){
    if(!hasIdleThread()){ // 无空闲线程则无需唤醒
        return;
    }
    int rt = write(m_ticklefds[1],"T",1);
    SYLAR_ASSERT(rt==1);
}
bool IOManager::stopping(uint64_t& timeout)
{
    timeout = getNextTimer();
    return timeout==~0ull&&Scheduler::stopping()&&m_pendingEventCount==0;
}
bool IOManager::stopping(){
    uint64_t timeout = 0;
    return stopping(timeout);
}
void IOManager::idle()
{
    // idle 主循环：epoll_wait 等待事件，处理唤醒管道与定时器，并触发就绪的 fd 事件
    const int MAX_EVENTS = 256;
    epoll_event* events = new epoll_event[MAX_EVENTS]();
    std::shared_ptr<epoll_event>shared_event(events,[](epoll_event*ptr){
        delete[] ptr;
    });
    while(true)
    {
        uint64_t next_timeout = 0;
        if(stopping(next_timeout)){
            SYLAR_LOG_INFO(g_logger)<<"m_name: "<<Scheduler::getName()<<";idle stopping exit";
            break;
        }
        int rt = 0;
        do{
            static const int MAX_TIMEOUT = 3000;
            if(next_timeout!=~0ull){
                next_timeout = (int)next_timeout>MAX_TIMEOUT?MAX_TIMEOUT:next_timeout;
            }
            else{
                next_timeout = MAX_TIMEOUT;
            }
            rt = epoll_wait(m_epfd,events,MAX_EVENTS,(int)next_timeout);
            if(rt<0&&errno==EINTR){

            }
            else break;
        }while(true);

        // 收集已到期定时器的回调
        std::vector<std::function<void()>>cbs;
        TimerManager::listExpiredTimer(cbs);
        if(!cbs.empty()){
            Scheduler::schedule(cbs.begin(),cbs.end());
            cbs.clear();
        }
        for(size_t i=0;i<rt;i++)
        {
            epoll_event& event = events[i];
            if(event.data.fd==m_ticklefds[0]){
                uint8_t dummy;
                // 读空管道；非阻塞模式下返回 -1 并设置 errno 为 EAGAIN
                while(read(m_ticklefds[0],&dummy,1)==1);
                continue;
            }
            FdContext* fd_context = (FdContext*)event.data.ptr;
            FdContext::MutexType::Lock lock(fd_context->mutex);
            if(event.events&(EPOLLERR|EPOLLHUP)){
                event.events |= EPOLLIN | EPOLLOUT; // 出错或挂起时同时监听读写以便尽快处理
            }
            int real_event = NONE;
            if(event.events&EPOLLIN){
                real_event |= READ;
            }
            if(event.events&EPOLLOUT){
                real_event |= WRITE;
            }
            if((fd_context->events&real_event)==NONE){
                continue;
            }
            int left_events = fd_context->events&~real_event;
            int op = left_events?EPOLL_CTL_MOD:EPOLL_CTL_DEL;
            event.events = EPOLLET|left_events;

            int rt2 = epoll_ctl(m_epfd,op,fd_context->fd,&event);
            if(rt2) {
                SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                    << op << ", " << fd_context->fd << ", " << (EPOLL_EVENTS)event.events << "):"
                    << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                continue;
            }
            if(real_event&READ){
                fd_context->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if(real_event&WRITE){
                fd_context->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }
        // 让出执行权，等待下一轮 epoll_wait
        auto cur = Fiber::GetThis();
        Fiber* raw = cur.get();
        cur.reset();
        raw->swapOut();
    }
}
void IOManager::onTimerInsertAtFront()
{
    tickle();
}
}