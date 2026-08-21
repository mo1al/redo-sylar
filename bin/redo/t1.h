// #pragma once
// #include"sylar.h"
// // 写一个thread类
// /*
// 使用的是pthread 而不是c++11 提供的thread
// 完成什么功能
// 单例(thread_local 禁用赋值 拷贝 移动)
// 成员变量 id,name,thread,执行函数cb 
// 成员函数 构造传入一个name和cb 构造和析构 析构用join/detach  返回接口(成员变量)+修改成员变量
// */
// namespace mo1al
// {
// class Thread
// {
// public:
//     typedef std::shared_ptr<Thread>ptr;
//     Thread(const std::string&name,std::function<void()>cb);
//     ~Thread();
//     void join();
// private:
//     static void* run(void*arg);
// public:
//     // return
//     static Thread* GetThis() ; //设计为static
//     static const std::string& GetCurrentName(); // 返回的是当前线程名
//     const std::string& getLocalName()const {return m_name;} // 返回的是特定实例名
//     pid_t getId()const {return m_id;}
//     // change
//     static void SetName(const std::string&name);
// private:    
//     pthread_t m_thread;
//     pid_t m_id;
//     std::string m_name;
//     std::function<void()>m_cb;
// private:
//     Thread(const Thread&)=delete; // 复制数据没有破坏const属性
//     Thread& operator=(const Thread&)=delete; // 返回Thread& 是为了链式赋值
//     Thread(Thread&&)=delete; // 涉及到move 不能写出const Thread&&
// };
// }