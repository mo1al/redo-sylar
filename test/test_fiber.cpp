#include"sylar.h"
static mo1al::Logger::ptr g_logger = SYLAR_LOG_ROOT();
void func1()
{
    SYLAR_LOG_INFO(g_logger)<<"subFiber run";
    mo1al::Fiber::GetThis()->YieldToHold();
    SYLAR_LOG_INFO(g_logger)<<"subFiber end";
    mo1al::Fiber::GetThis()->YieldToHold();
}
void test_fiber()
{   
    {
        mo1al::Fiber::GetThis();
        SYLAR_LOG_INFO(g_logger)<<"main begin";
        mo1al::Fiber::ptr subFiber(new mo1al::Fiber(func1));
        subFiber->swapIn(); 
        SYLAR_LOG_INFO(g_logger)<<"main swapin after ";
        subFiber->swapIn(); 
        SYLAR_LOG_INFO(g_logger)<<"main end";
        subFiber->swapIn(); 
        // subFiber->swapIn(); // 第二次直接触发
    }
    // SYLAR_LOG_INFO(g_logger)<<"main end2";
}
int main()
{
    std::vector<mo1al::Thread::ptr>vec;
    for(size_t i=0;i<3;i++)
    {
        vec.push_back(mo1al::Thread::ptr(new mo1al::Thread(test_fiber,"name_"+std::to_string(i))));
    }
    for(size_t i=0;i<3;i++)
    {
        vec[i]->join();
    }

    // test_fiber();
    return 0;
}