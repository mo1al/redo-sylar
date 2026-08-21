// timer.h —— 定时器：基于有序集合的定时任务管理，支持循环与条件定时器。
#pragma once
#include<memory>
#include"thread.h"
#include<set>
namespace mo1al
{
class TimerManager;
class Timer : public std::enable_shared_from_this<Timer>
{
    friend class TimerManager;
public:
    typedef std::shared_ptr<Timer>ptr;
    bool cancel();
    bool refresh();
    bool reset(uint64_t ms,bool from_now);
private:
    Timer(uint64_t ms,std::function<void()>cb,bool recurring,TimerManager*manager);
    Timer(uint64_t next);
private:
    bool m_recurring = false; // 是否循环定时
    uint64_t m_ms = 0; // 执行周期（毫秒）
    uint64_t m_next = 0; // 精确的下次执行时间
    std::function<void()>m_cb;
    TimerManager* m_manager;
private:
    struct Comparator{
        bool operator()(const Timer::ptr&lhs,const Timer::ptr&rhs)const; // 按 m_next 升序比较
    };
};
// 定时器管理器：维护定时器集合，提供到期检测与调度
class TimerManager
{
    friend class Timer;
public:
    typedef RWmutex MutexType;
    TimerManager();
    virtual ~TimerManager();
    Timer::ptr addTimer(uint64_t ms,std::function<void()>cb,bool recurring=false);
    Timer::ptr addConditionTimer(uint64_t ms,std::function<void()>cb,std::weak_ptr<void>weak_cond,bool recurring=false);
    uint64_t getNextTimer();
    void listExpiredTimer(std::vector<std::function<void()>>&cbs);
    bool detectClockRollOver(uint64_t now_ms);
protected:
    virtual void onTimerInsertAtFront()=0;
    void addTimer(Timer::ptr timer,MutexType::WriteLock& lock);
private:
    MutexType m_mutex;
    std::set<Timer::ptr,Timer::Comparator>m_timers;
    bool m_tickled = false;
    uint64_t m_previousTime = 0;
};
}