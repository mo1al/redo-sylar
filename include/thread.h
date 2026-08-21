// thread.h —— 线程相关基础设施：信号量、各类锁（互斥/读写/自旋）与线程封装。
#pragma once
#include<thread>
#include<pthread.h>
#include<memory>
#include<functional>
#include<string>
#include"util.h"
#include"semaphore.h"
#include<inttypes.h>
#include"nocopyable.h"
namespace mo1al
{
// POSIX 信号量封装：用于线程构造时同步，确保线程真正启动后才返回
class Sem : public Nocopyable
{
public:
    Sem(uint32_t count=0);
    ~Sem();
    void wait();
    void notify();
private:
    Sem(Sem&&)=delete;
private:
    sem_t m_sem;
};

// RAII 锁包装器：构造时加锁，析构时解锁
template<class T>
class ScopeLock
{
public:
    ScopeLock(T&mutex):m_mutex(mutex)
    {
        lock();
    }
    ~ScopeLock()
    {
        unlock();
    }
    void lock()
    {
        if(!is_lock)
        {
            m_mutex.lock();
            is_lock = true;
        }
    }
    void unlock()
    {
        if(is_lock)
        {
            m_mutex.unlock();
            is_lock = false;
        }
    }
private:
    T& m_mutex;
    bool is_lock = false;
};

// RAII 读锁包装器
template<class T>
class ReadScopeLock
{
public:
    ReadScopeLock(T&mutex):m_mutex(mutex)
    {
        lock();
    }
    ~ReadScopeLock()
    {
        unlock();
    }
    void lock()
    {
        if(!is_lock)
        {
            m_mutex.rdlock();
            is_lock = true;
        }
    }
    void unlock()
    {
        if(is_lock)
        {
            m_mutex.unlock();
            is_lock = false;
        }
    }
private:
    T& m_mutex;
    bool is_lock = false;
};

// RAII 写锁包装器
template<class T>
class WriteScopeLock
{
public:
    WriteScopeLock(T&mutex):m_mutex(mutex)
    {
        lock();
    }
    ~WriteScopeLock()
    {
        unlock();
    }
    void lock()
    {
        if(!is_lock)
        {
            m_mutex.wrlock();
            is_lock = true;
        }
    }
    void unlock()
    {
        if(is_lock)
        {
            m_mutex.unlock();
            is_lock = false;
        }
    }
private:
    T& m_mutex;
    bool is_lock = false;
};

// 互斥锁：封装 pthread_mutex
class Mutex : public Nocopyable
{
public:
    typedef ScopeLock<Mutex>Lock;
    Mutex(){
        pthread_mutex_init(&m_mutex,nullptr);
    }
    ~Mutex(){
        pthread_mutex_destroy(&m_mutex);
    }
    void lock(){
        pthread_mutex_lock(&m_mutex);
    }
    void unlock(){
        pthread_mutex_unlock(&m_mutex);
    }
private:
    pthread_mutex_t m_mutex;
};

// 读写锁：封装 pthread_rwlock
class RWmutex : public Nocopyable
{
public:
    // RAII 读写锁别名：构造时加锁，析构时自动解锁
    typedef ReadScopeLock<RWmutex>ReadLock;
    typedef WriteScopeLock<RWmutex>WriteLock;
    RWmutex(){
        pthread_rwlock_init(&m_lock,nullptr);
    }
    ~RWmutex(){
        pthread_rwlock_destroy(&m_lock);
    }
    void unlock(){
        pthread_rwlock_unlock(&m_lock);
    }
    void rdlock(){
        pthread_rwlock_rdlock(&m_lock);
    }
    void wrlock(){
        pthread_rwlock_wrlock(&m_lock);
    }
private:
    pthread_rwlock_t m_lock;
};

// 自旋锁：封装 pthread_spinlock
class SpinLock : Nocopyable
{
public:
    typedef ScopeLock<SpinLock>Lock;
    SpinLock(){
        pthread_spin_init(&m_mutex,0);
    }
    ~SpinLock(){
        pthread_spin_destroy(&m_mutex);
    }
    void lock(){
        pthread_spin_lock(&m_mutex);
    }
    void unlock(){
        pthread_spin_unlock(&m_mutex);
    }
private:
    pthread_spinlock_t m_mutex;
};

// 空锁：不提供任何互斥语义，用于无锁场景占位
class NullMutex : Nocopyable
{
public:
    typedef std::shared_ptr<NullMutex>ptr;
    NullMutex(){}
    ~NullMutex(){}
    void lock(){}
    void unlock(){}
};

// 线程封装：创建并管理一个 pthread 线程
class Thread
{
public:
    typedef std::shared_ptr<Thread>ptr;
    Thread(std::function<void()>cb,const std::string&name);
    ~Thread();
    pid_t getId()const { return m_id; }
    const std::string& getName() const { return m_name;}
    void join();
    static const std::string& GetName();
    static Thread* getThread();
    static void SetName(const std::string&name);
private:
    pid_t m_id;
    pthread_t m_thread;
    std::string m_name;
    std::function<void()>m_cb;
    Sem sem; // 线程启动同步信号量
    static int count;
private:
    Thread(const Thread&)=delete;
    Thread(Thread&&)=delete;
    Thread& operator=(const Thread&)=delete;
    static void* run(void* arg);
};
}