#include"sylar.h"
static mo1al::Logger::ptr g_logger = SYLAR_LOG_ROOT();
int count = 0;
mo1al::RWmutex mutex;
void run1()
{
    SYLAR_LOG_INFO(g_logger) << "name: " << mo1al::Thread::GetName()
                            << " this.name: " << mo1al::Thread::getThread()->getName()
                            << " id: " << mo1al::getThreadId()
                            << " this.id: " << mo1al::Thread::getThread()->getId();
    for(size_t i=0;i<100000;i++){
        mo1al::RWmutex::WriteLock lock(mutex);
        ++count;
    }
}
void func2()
{
    for(int i=0;i<1000;i++)
    {
        SYLAR_LOG_INFO(g_logger)<<"xxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
}
void func3()
{
    for(int i=0;i<1000;i++){
        SYLAR_LOG_INFO(g_logger)<<"----------------------------";
    }
}
int main(int argc, char** argv) {
    YAML::Node root = YAML::LoadFile("/home/mo1al/redo-sylar/bin/conf/log2.yaml"); // file 
    mo1al::Config::LoadFromYaml(root);
    SYLAR_LOG_INFO(g_logger) << "thread test begin"; // cout 

    std::vector<mo1al::Thread::ptr> thrs;
    for(int i = 0; i < 1; ++i) {
        mo1al::Thread::ptr thr(new mo1al::Thread(&func2, "name_" + std::to_string(i * 2)));
        mo1al::Thread::ptr thr2(new mo1al::Thread(&func3, "name_" + std::to_string(i * 2 + 1)));
        thrs.push_back(thr);
        thrs.push_back(thr2);
    }

    for(size_t i = 0; i < thrs.size(); ++i) {
        thrs[i]->join();
    }
    SYLAR_LOG_INFO(g_logger) << "thread test end";
    SYLAR_LOG_INFO(g_logger) << "count=" << count;
    

    return 0;
}