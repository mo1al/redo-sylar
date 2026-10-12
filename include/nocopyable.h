// nocopyable.h —— 不可拷贝基类：删除拷贝构造与拷贝赋值，禁止派生类被拷贝。
#pragma once
namespace mo1al
{
// 不可拷贝基类，继承后对象禁止拷贝与赋值。
class Nocopyable
{
public:
    Nocopyable() =default;
    ~Nocopyable() =default;
    Nocopyable(const Nocopyable&)=delete;
    Nocopyable& operator=(const Nocopyable&)=delete;
};
}