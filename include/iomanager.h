// iomanager.h —— IO 调度器：基于 epoll 的 IO 事件管理，结合定时器与协程调度。
#pragma once
#include"scheduler.h"
#include"timer.h"
namespace mo1al
{
class IOManager : public Scheduler,public TimerManager
{
public:
    std::shared_ptr<IOManager>ptr;
    typedef RWmutex MutexType;
    // IO 事件类型
    enum Event
    {
        NONE = 0x0, // 无事件
        READ = 0x1, // 可读（EPOLLIN）
        WRITE = 0x4, // 可写（EPOLLOUT）
    };
public:
    // 文件描述符上下文：记录fd的读写事件与等待的协程/回调
    struct FdContext
    {
        typedef Mutex MutexType;
        struct EventContext
        {
            Scheduler* scheduler = nullptr; // 所属调度器，用于跨线程调度回原任务
            Fiber::ptr fiber;
            std::function<void()>cb;
        };
        EventContext& getContext(Event event);
        void resetContext(EventContext& ctx);
        void triggerEvent(Event event);
        EventContext read;
        EventContext write;
        int fd=0;
        Event events = NONE;
        MutexType mutex;
    };
public:
    IOManager(size_t threads=1,bool use_caller=true,const std::string&name="");
    ~IOManager();
    int addEvent(int fd,Event event,std::function<void()>cb=nullptr);
    bool delEvent(int fd,Event event);
    bool cancelEvent(int fd,Event event);
    bool cancelAll(int fd);
    static IOManager* GetThis();
    void resizeContext(int size);
private:
    void onTimerInsertAtFront() override;
    virtual void tickle()override;
    virtual bool stopping()override;
    bool stopping(uint64_t& timeout);
    virtual void idle() override;
private:
    int m_pendingEventCount = 0;
    MutexType m_mutex;
    int m_epfd ;
    int m_ticklefds[2];
    std::vector<FdContext*>m_fdcontext;
};
}