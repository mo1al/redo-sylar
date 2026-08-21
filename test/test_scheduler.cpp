#include"sylar.h"
static mo1al::Logger::ptr g_logger = SYLAR_LOG_ROOT();
void test_scheduler()
{
    static int m_count = 5;
    SYLAR_LOG_INFO(g_logger)<<"test scheduler count = "<<m_count;
    sleep(1);
    if(--m_count>0)
    {
        mo1al::Scheduler::GetThis()->schedule(&test_scheduler);
    }
}
int main()
{   
    // 问题 ： 死锁+ name 
    mo1al::Scheduler sc(3,true,"test");
    sc.start(); // run idle 
    sleep(2);
    SYLAR_LOG_INFO(g_logger)<<"scheduler";
    sc.schedule(&test_scheduler); // 两个线程没拿到任务直接挂了
    sc.stop();
    return 0;
}
