#include"timer.h"
namespace mo1al{
bool Timer::Comparator::operator()(const Timer::ptr&lhs,const Timer::ptr&rhs)const
{
    if(!lhs&&!rhs)return false;
    if(!lhs)return true;
    if(!rhs)return false;
    if(lhs->m_next<rhs->m_next)return true;
    if(lhs->m_next>rhs->m_next)return false;
    return lhs.get() < rhs.get();
}
Timer::Timer(uint64_t ms,std::function<void()>cb,bool recurring,TimerManager*manager)
    : m_ms(ms),m_cb(cb),m_recurring(recurring),m_manager(manager)
{
    m_next = mo1al::GetCurrentMs()+m_ms;
}
Timer::Timer(uint64_t next)
    : m_next(next)
{

}
bool Timer::cancel()
{
    TimerManager::MutexType::WriteLock lock(m_manager->m_mutex);
    if(m_cb){
        m_cb = nullptr;
        auto it = m_manager->m_timers.find(shared_from_this());
        if(it!=m_manager->m_timers.end()){
            m_manager->m_timers.erase(it);
        }
        return true;
    }
    return false;
}
bool Timer::refresh()
{
    TimerManager::MutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_cb)return false;
    auto it = m_manager->m_timers.find(shared_from_this());
    if(it==m_manager->m_timers.end()){
        return false;
    }
    m_manager->m_timers.erase(it);
    m_next = mo1al::GetCurrentMs()+m_ms;
    m_manager->m_timers.insert(shared_from_this());
    return true;
}
bool Timer::reset(uint64_t ms,bool from_now)
{
    if(ms==m_ms&&!from_now){return true;}
    TimerManager::MutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_cb)return false;
    auto it = m_manager->m_timers.find(shared_from_this());
    if(it==m_manager->m_timers.end()){
        return false;
    }
    m_manager->m_timers.erase(it);
    uint64_t start = 0;
    if(from_now){
        start = mo1al::GetCurrentMs();
    }
    else{
        start = m_next-m_ms;
    }
    m_ms = ms;
    m_next = m_ms+start;
    m_manager->addTimer(shared_from_this(),lock);
    return true;
}

TimerManager::TimerManager()
{

}
TimerManager::~TimerManager()
{

}
uint64_t TimerManager::getNextTimer()
{
    MutexType::ReadLock lock(m_mutex);
    m_tickled = false;
    if(m_timers.empty()) return ~0ull;
    const Timer::ptr begin = *m_timers.begin();
    uint64_t now_ms = mo1al::GetCurrentMs();
    if(now_ms>begin->m_next){
        return 0;
    }
    else{
        return begin->m_next-now_ms;
    }
}
void TimerManager::addTimer(Timer::ptr timer,MutexType::WriteLock& lock)
{
    auto it = m_timers.insert(timer).first;
    bool is_front = (it==m_timers.begin())&&!m_tickled;
    if(is_front){
        m_tickled = true;  // 标记已触发过 onTimerInsertAtFront，避免重复调用
    }
    lock.unlock();
    if(is_front){
        onTimerInsertAtFront(); // 通知定时器插入队首（内部可能加锁）
    }
}
Timer::ptr TimerManager::addTimer(uint64_t ms,std::function<void()>cb,bool recurring)
{
    MutexType::WriteLock lock(m_mutex);
    Timer::ptr timer(new Timer(ms,cb,recurring,this));
    // addTimer 内部已解锁；lock 析构时因 is_lock 为 false 不会二次解锁
    addTimer(timer,lock);
    return timer;
}
static void OnTimer(std::weak_ptr<void>weak_cond,std::function<void()>cb)
{
    auto tmp = weak_cond.lock();
    if(tmp){
        cb();
    }
}
Timer::ptr TimerManager::addConditionTimer(uint64_t ms,std::function<void()>cb,
    std::weak_ptr<void>weak_cond,bool recurring)
{
    return TimerManager::addTimer(ms,std::bind(&OnTimer,weak_cond,cb),recurring);
}
bool TimerManager::detectClockRollOver(uint64_t now_ms)
{
    bool rollover = false;
    if(now_ms<m_previousTime&&now_ms+1000*60*60<m_previousTime) rollover = true;
    m_previousTime = now_ms;
    return rollover;
}
void TimerManager::listExpiredTimer(std::vector<std::function<void()>>&cbs)
{
    uint64_t now_ms = mo1al::GetCurrentMs();
    std::vector<Timer::ptr>expired;
    {
        MutexType::ReadLock lock1(m_mutex);
        if(m_timers.empty()) return;
    }
    MutexType::WriteLock lock2(m_mutex);
    if(m_timers.empty()) return;
    bool rollover = detectClockRollOver(now_ms);
    if(!rollover&&((*m_timers.begin())->m_next>now_ms)) return;

    // 定位已到期定时器的区间（左闭右开）
    Timer::ptr timer(new Timer(now_ms));
    auto it = rollover?m_timers.end():m_timers.upper_bound(timer);
    expired.insert(expired.begin(),m_timers.begin(),it);
    // 移除已到期定时器，循环定时器重新插入
    m_timers.erase(m_timers.begin(),it);
    cbs.reserve(expired.size());
    for(auto&p:expired){
        cbs.push_back(p->m_cb);
        if(p->m_recurring){
            p->m_next = now_ms+p->m_ms;
            m_timers.insert(p);
        }
        else{
            p->m_cb=nullptr; // 断开回调，避免回调中持有的智能指针无法释放
        }
    }
}
}