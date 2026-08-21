// sinleton.h —— 单例模板：基于局部静态变量的线程安全懒加载单例。
#pragma once
namespace mo1al
{
// 单例模板：GetInstance 返回进程内唯一的 T 实例（局部静态变量，首次调用时构造）。
template<class T>
class Singleton
{
public:
    static T* GetInstance(){
        static T t;
        return &t;
    }
private:
};
}