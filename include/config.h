// config.h —— 配置模块：基于 YAML 的配置变量系统，支持类型转换与变更监听。
#pragma once
#include<memory>
#include<string>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <boost/lexical_cast.hpp>
#include <vector>
#include <list>
#include <map>
#include <set>
#include<log.h>
#include"yaml-cpp/yaml.h"
namespace mo1al
{
// 配置变量的基类
class ConfigVarBase
{
public:
    typedef std::shared_ptr<ConfigVarBase>ptr;

    ConfigVarBase(const std::string&name,const std::string&description=""):
    m_name(name),m_description(description)
    {
        // 名称统一转换为小写
        std::transform(m_name.begin(),m_name.end(),m_name.begin(),::tolower);
    }
    virtual ~ConfigVarBase(){}

    const std::string& getName()const{return m_name;}
    const std::string& getDescription() const{return m_description;}

    virtual std::string toString() = 0;
    virtual bool fromString(const std::string& val) = 0;
    // 获取配置项类型名
    virtual std::string getTypeName() const = 0;
protected:
    // 配置参数的名称
    std::string m_name;
    // 配置参数的描述
    std::string m_description;
};

// 通用类型转换：F -> T（基于 boost::lexical_cast）
template<class F,class T>
class LexicalCast
{
public:
    T operator() (const F&v)
    {
        return boost::lexical_cast<T>(v);
    }
    
};
template<class T>
class LexicalCast<std::string,std::vector<T>>
{
public:
    std::vector<T> operator()(const std::string&v)
    {
        YAML::Node node = YAML::Load(v);
        typename std::vector<T> vec;
        std::stringstream ss;
        for(size_t i=0;i<node.size();i++)
        {
            ss.str("");
            ss<<node[i];
            vec.push_back(LexicalCast<std::string,T>()(ss.str()));
        }
        return vec;
    }
};
template<class T>
class LexicalCast<std::vector<T>,std::string>
{
public:
    std::string operator()(const std::vector<T>&v)
    {
        YAML::Node node(YAML::NodeType::Sequence);
        for(auto&i:v)
        {
            node.push_back(YAML::Load(LexicalCast<T,std::string>()(i)));
        }
        std::stringstream ss;
        ss<<node;
        return ss.str();
    }
};
template<class T>
class LexicalCast<std::string,std::map<std::string,T>>
{
public:
    std::map<std::string,T> operator() (const std::string&v)
    {
        YAML::Node node = YAML::Load(v);
        std::stringstream ss;
        typename std::map<std::string,T>m;
        for(auto it=node.begin();it!=node.end();it++) 
        {
            ss.str("");
            ss<<it->second;
            m.insert({it->first.Scalar(),LexicalCast<std::string,T>()(ss.str())});
        }
        return m;
    }
};
template<class T>
class LexicalCast<std::map<std::string,T>,std::string>
{
public:
    std::string operator() (const std::map<std::string,T>&v)
    {
        YAML::Node node(YAML::NodeType::Map);
        for(auto&i:v){
            node[i.first] = YAML::Load(LexicalCast<T,std::string>()(i.second));
        }
        std::stringstream ss;
        ss<<node;
        return ss.str();
    }
};
template<class T>
class LexicalCast<std::string,std::set<T>>
{
public:
    std::set<T> operator()(const std::string&v)
    {
        YAML::Node node = YAML::Load(v);
        typename std::set<T> s;
        std::stringstream ss;
        for(size_t i=0;i<node.size();i++)
        {
            ss.str("");
            ss<<node[i];
            s.insert(LexicalCast<std::string,T>()(ss.str()));
        }
        return s;
    }
};
template<class T>
class LexicalCast<std::set<T>,std::string>
{
public:
    std::string operator()(const std::set<T>&v)
    {
        YAML::Node node(YAML::NodeType::Sequence);
        for(auto&i:v)
        {
            node.push_back(YAML::Load(LexicalCast<T,std::string>()(i)));
        }
        std::stringstream ss;
        ss<<node;
        return ss.str();
    }
};

// 具体配置变量：持有配置值、变更监听与类型转换
template <class T,class FromStr=LexicalCast<std::string,T>,class ToStr=LexicalCast<T,std::string>>
class ConfigVar:public ConfigVarBase
{
public:
    typedef mo1al::RWmutex MutexType;
    typedef std::shared_ptr<ConfigVar>ptr;
    typedef std::function<void(const T&old_value,const T&new_value)>on_change_cb;

    ConfigVar(const std::string& name,const T& default_value,const std::string& description = "")
        :ConfigVarBase(name, description)
        ,m_val(default_value) 
    {}
    
    std::string toString()override
    {
        try
        {
            MutexType::ReadLock lock(m_mutex);
            return ToStr()(m_val); // ToStr是个类 重载了()函数
        }
        catch(std::exception&e)
        {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())<<"ConfigVar::tostring exceptions"<<e.what()<<"convert:"<<typeid(m_val).name()<<"to string";
        }
        return "";
    }
    bool fromString(const std::string& val)override
    {
        try
        {
            setValue(FromStr()(val));
            return true;
        }
        catch(const std::exception& e)
        {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())<<"ConfigVar::fromstring exception:"<<e.what()<<";convert: string to "<<typeid(T).name();
        }
        return false;
    }

    // 返回配置项的类型名
    std::string getTypeName() const override
    {
        return typeid(T).name(); 
    }
    const T getValue()
    {
        MutexType::ReadLock lock(m_mutex);
        return m_val;
    }
    void setValue(const T&v)
    {
        {
            MutexType::ReadLock lock(m_mutex);
            if(v==m_val){
                return ;
            }
            for(auto&i:this->m_cbs){
                i.second(m_val,v);
            }
        }
        MutexType::WriteLock lock(m_mutex);
        m_val = v;
    }
    // on_change_cb的增删方法 
    uint64_t addListener(on_change_cb cb){
        static std::atomic<uint64_t> add_count{0};
        MutexType::WriteLock lock(m_mutex); 
        uint64_t key = add_count.fetch_add(1); // 原子自增生成唯一 key
        m_cbs[key] = cb;
        return key;
    }   
    void delListener(uint64_t key){
        MutexType::WriteLock lock(m_mutex);
        m_cbs.erase(key);
    }
    void clearListener(){
        MutexType::WriteLock lock(m_mutex);
        m_cbs.clear();
    }
    on_change_cb getListener(uint64_t key)
    {
        MutexType::ReadLock lock(m_mutex);
        auto it = m_cbs.find(key);
        return it==m_cbs.end()?nullptr:it->second; 
    }
private:
    MutexType m_mutex;
    T m_val;
    std::map<uint64_t,on_change_cb>m_cbs;
};
class Config
{
public:
    typedef std::map<std::string,ConfigVarBase::ptr>ConfigVarMap;
    template<class T>
    static typename ConfigVar<T>::ptr LookUp(const std::string&name,const T& default_value,const std::string& description="" )
    {
        mo1al::RWmutex::WriteLock lock(GetMutex());
        auto it = GetDatas().find(name); 
        if(it!=GetDatas().end()){
            auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
            if(tmp){
                // 找到了并存在的情况 
                SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"Lookup name= "<<name<<" exists";
                return tmp;
            }
            else{
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())<<"Lookup name=" << name << " exists but type not "<<
                typeid(T).name()<<"real type"<<it->second->getTypeName()<<" "<<it->second->getTypeName();
                return nullptr;
            }
        }
        if(name.find_last_not_of("abcdefghijklmnopqrstuvwxyz._1234567890")!=std::string::npos)
        {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())<<"Look Up name invalid "<<name;
            throw std::invalid_argument(name);
        }
        // 没找到也要注册 
        typename ConfigVar<T>::ptr v(new ConfigVar<T>(name,default_value,description));
        GetDatas()[name]= v; // shared_ptr 到基类指针的隐式转换
        return v;
    }
    template<class T>
    static typename ConfigVar<T>::ptr  LookUp(const std::string&name)
    {
        mo1al::RWmutex::ReadLock lock(GetMutex());
        auto it = GetDatas().find(name);
        if(it==GetDatas().end())
        {
            return nullptr;
        }
        return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
    }

    static ConfigVarBase::ptr LookUpBase(const std::string&name);
    // 两个加载方法 
    static void LoadFromYaml(const YAML::Node&root);
    static void LoadFromConDir(const std::string&path,bool force = false);
    // 逐一访问map里面的每一个指针
    static void visit(std::function<void(ConfigVarBase::ptr)>cb);
private:
    static ConfigVarMap& GetDatas(){
        static ConfigVarMap s_datas;
        return s_datas ; 
    }
    static mo1al::RWmutex& GetMutex(){
        static RWmutex m_mutex;
        return m_mutex;
    }
};

}