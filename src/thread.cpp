#include"thread.h"
#include"log.h"
namespace mo1al
{
static thread_local Thread* t_thread = nullptr; // 当前线程对应的 Thread 对象
static thread_local std::string t_thread_name = "UNKNOWN"; // 当前线程名称
static mo1al::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
Sem::Sem(uint32_t count)
{
    if(sem_init(&m_sem,0,count))
    {
        throw std::logic_error("sem init error");
    }
}
Sem::~Sem()
{
    if(sem_destroy(&m_sem))
    {
        // 析构函数默认 noexcept，不能抛异常，否则会直接 terminate
        SYLAR_LOG_ERROR(g_logger) << "sem destroy error";
    }
}
void Sem::wait()
{
    if(sem_wait(&m_sem))
    {
        throw std::logic_error("sem wait error");
    }
}
void Sem::notify()
{
    if(sem_post(&m_sem))
    {
        throw std::logic_error("sem post error");
    }
}
Thread* Thread::getThread()
{
    return t_thread;
}
const std::string& Thread::GetName()
{
    return t_thread_name;
}
void Thread::SetName(const std::string&name)
{
    if(t_thread){
        t_thread->m_name = name;
    }
    t_thread_name = name;
}
Thread::Thread(std::function<void()>cb,const std::string&name)
    : m_id(-1)
    , m_thread(0)
    , m_name(name)
    , m_cb(cb)
    ,sem()
{
    if(name.empty()){
        m_name = "UNKNOWN";
    }
    if(!m_cb){
        SYLAR_LOG_ERROR(g_logger)<<"Thread callback is empty, name="<<m_name;
        throw std::logic_error("Thread callback is empty");
    }
    int rt = pthread_create(&m_thread,nullptr,&Thread::run,this);
    if(rt){
        SYLAR_LOG_ERROR(g_logger)<<"pthread_create thread fail,rt ="<<rt<<"name ="<<name;
        throw std::logic_error("pthread_create error");
    }
    sem.wait(); // 阻塞直至线程启动完成
}
Thread::~Thread()
{
    if(m_thread){
        pthread_detach(m_thread);
    }
}
void Thread::join()
{
    if(m_thread){
        int rt = pthread_join(m_thread,nullptr);
        if(rt){
            SYLAR_LOG_ERROR(g_logger)<<"pthread_join thread fail,rt ="<<rt<<"name :"<<m_name;
            throw std::logic_error("thread join error");
        }
        m_thread = 0;
    }
}
void* Thread::run(void*arg)
{
    Thread* thread = (Thread*)arg;  // 将参数转回 Thread 指针
    t_thread = thread;
    thread->m_id = mo1al::getThreadId();
    t_thread_name = thread->m_name;
    pthread_setname_np(pthread_self(),thread->m_name.substr(0,15).c_str());
    // 先 swap 出回调再执行，避免 Thread 对象析构后访问其成员
    std::function<void()>cb;
    cb.swap(thread->m_cb);
    thread->sem.notify(); // 通知构造线程已启动完成
    cb();
    return 0;
}

}