// scheduler.h —— 协程调度器：管理线程池与任务队列，调度协程/回调执行。
#pragma once
#include<vector>
#include<memory>
#include<list>
#include"thread.h"
#include"fiber.h"
#include<atomic>
namespace mo1al
{
class Scheduler
{
public:
    typedef std::shared_ptr<Scheduler>ptr;
    typedef Mutex MutexType;
    Scheduler(size_t threads=1,bool use_caller=true,const std::string&name="");
    virtual ~Scheduler();
    const std::string& getName()const {return m_name;}
    static Scheduler* GetThis();
    static Fiber* GetCurrentFiber();
    bool hasIdleThread()const{return m_idleThreadCount>0;}
    void start();
    void stop();
    // 调度单个协程或回调
    // move优化
    template <class FiberOrCb>
    void schedule(FiberOrCb cb,int thread=-1)
    {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = scheduleNoLock(std::move(cb),thread);
        }
        if(need_tickle)
        {
            tickle();
        }
    }
    // 批量调度
    template <class InputIterator>
    void schedule(InputIterator begin,InputIterator end)
    {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while(begin!=end)
            {
                need_tickle = scheduleNoLock(&*begin,-1) || need_tickle;
                begin++;
            }
            if(need_tickle)
            {
                tickle();
            }
        }
    }
protected:
    virtual void tickle();
    virtual bool stopping();
    virtual void idle();
    void run();
    void SetThis();
private:
    // move优化
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb cb,int thread)
    {
        bool need_tickle =  m_fibers.empty();
        FiberAndThread ft(std::move(cb),thread);
        if(ft.cb||ft.fiber)
        {
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }
    // 任务包装：协程或回调，可指定执行线程
    struct FiberAndThread
    {
        Fiber::ptr fiber=nullptr;
        std::function<void()>cb=nullptr;
        int thread = -1; // 指定线程id

        FiberAndThread()=default;

        FiberAndThread(Fiber::ptr f,int thr)
        : fiber(f),thread(thr)
        {

        }
        FiberAndThread(Fiber::ptr*f,int thr)
        : fiber(f?std::move(*f):nullptr),thread(thr)
        {

        }
        FiberAndThread(std::function<void()>f,int thr)
        : cb(f),thread(thr)
        {

        }
        FiberAndThread(std::function<void()>*f,int thr)
        : cb(f?std::move(*f):nullptr),thread(thr)
        {

        }
        void reset()
        {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };
private:
    std::vector<Thread::ptr>m_threads;
    std::string m_name;
    MutexType m_mutex;
    std::list<FiberAndThread>m_fibers;
    mo1al::Fiber::ptr m_rootFiber;
protected:
    // std::vector<int>m_threadIds;
    std::atomic<size_t> m_threadCount ={0};
    std::atomic<size_t> m_activeThreadCount = {0};
    std::atomic<size_t> m_idleThreadCount={0};
    bool m_stopping = true;
    bool m_autostop = false;
    int m_rootThread = 0;
};
}