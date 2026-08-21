// #include"t1.h"
// namespace mo1al
// {
// //定义一下日志
// static thread_local Thread* t_thread = nullptr;
// static thread_local std::string t_thread_name = "UNKOWN";
// static mo1al::Logger::ptr g_logger = SYLAR_LOG_NAME("system");// 使用哪个对象 root?
// Thread::Thread(const std::string&name,std::function<void()>cb)
// : m_name(name),m_cb(cb),m_id(-1),m_thread(0)
// {
//     if(m_name.empty()){
//         m_name = "UNKOWN";
//     }
//     if(!cb){
//         SYLAR_LOG_ERROR(g_logger) << "Thread callback is empty";
//         throw std::logic_error("Thread callback is empty");      
//     }
//     int rt = pthread_create(&m_thread,nullptr,&run,this); // 0 success
//     if(rt!=0){
//         SYLAR_LOG_ERROR(g_logger)<<"pthread create error"<<" name:"
//         <<m_name; //这里要不要返回出id呢 属于设计问题 这个id返回的是线程id 不写比较好
//         throw std::logic_error("pthread create error");
//     }
// }
// void* Thread::run(void*arg)
// {
//     Thread* thread = static_cast<Thread*>(arg);
//     // thread能够使用当前对象的所有成员变量
//     t_thread = thread; // 这个不是赋值了吗 不对吧 应该要使用移动赋值吧 不对是不是这个就是移动赋值
//     t_thread->m_id = mo1al::getThreadId();
//     t_thread_name = thread->m_name;  // ? 
//     // name 设置线程名的函数
//     pthread_setname_np(pthread_self(),thread->m_name.substr(0,15).c_str());
//     //cb
//     std::function<void()>cb;
//     cb.swap(thread->m_cb);
//     cb();
//     return 0;
// }
// Thread* Thread::GetThis()
// {
//     return t_thread;
// }
// const std::string& Thread::GetCurrentName()
// {
//     // return t_thread->m_name; 危险是访问空指针的成员变量导致访问非法内存
//     return t_thread_name;
// }
// void Thread::join()
// {
//     if(m_thread)
//     {   
//         int rt = pthread_join(m_thread,nullptr);
//         if(rt!=0){
//             SYLAR_LOG_ERROR(g_logger)<<"pthread join error"<<" name:"
//                 <<m_name;
//             throw std::logic_error("pthread join error");
//         }
//         m_thread = 0;
//     }
// }
// Thread::~Thread()
// {
//     if(m_thread){
//         pthread_detach(m_thread);
//     }
// }
// void Thread::SetName(const std::string&name)
// {
//     if(t_thread){
//         t_thread->m_name=name;
//     }
//     t_thread_name = name;
// }

// }