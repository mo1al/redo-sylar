#include"config.h"
namespace mo1al
{

static mo1al::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

ConfigVarBase::ptr Config::LookUpBase(const std::string&name)
{
    mo1al::RWmutex::ReadLock lock(GetMutex());
    auto it = GetDatas().find(name);
    return it==GetDatas().end()?nullptr:it->second;
}

// 将嵌套的 YAML 配置展平为「路径 -> 节点」列表
static void ListAllMember(const std::string&prefix,const YAML::Node&node,std::list<std::pair<std::string,const YAML::Node>>&output)
{
    if(prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz_.0123456789")!=std::string::npos) // 路径含非法字符则跳过
    {
        SYLAR_LOG_ERROR(g_logger)<<"config invalid name:"<<prefix<<":"<<node;
        return ;
    }
    output.push_back(std::make_pair(prefix,node));
    if(node.IsMap()){ // 递归展开 map 节点
        for(auto it = node.begin();it!=node.end();it++){
            ListAllMember(prefix.empty()?it->first.Scalar():(prefix+"."+it->first.Scalar()),it->second,output); // 拼接子路径并递归
        }
    }
}
void Config::LoadFromYaml(const YAML::Node&root)
{
    std::list<std::pair<std::string,const YAML::Node>>all_nodes; // 存放展平后的节点
    ListAllMember("",root,all_nodes);
    for(auto&i:all_nodes)
    {
        std::string key = i.first;
        if(key.empty()) // 跳过根节点
        {
            continue;
        }
        std::transform(key.begin(),key.end(),key.begin(),::tolower); // 名称统一小写
        ConfigVarBase::ptr var = LookUpBase(key);
        if(var)
        {
            if(i.second.IsScalar()) // 标量直接转换
            {
                var->fromString(i.second.Scalar()); // 将字符串转换为目标类型
            }
            else // 复杂结构先序列化为字符串再转换
            {
                std::stringstream ss;
                ss<<i.second;
                var->fromString(ss.str());
            }
        }
    }
}
void Config::LoadFromConDir(const std::string&path,bool force)
{

}
void Config::visit(std::function<void(ConfigVarBase::ptr)>cb)
{
    mo1al::RWmutex::ReadLock lock(GetMutex());
    ConfigVarMap& m = GetDatas();
    for(auto it=m.begin();it!=m.end();it++)
    {
        cb(it->second);
    }
}

}