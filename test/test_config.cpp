#include"config.h"
#include"log.h"

#if 1
mo1al::ConfigVar<int>::ptr g_int_value_config = mo1al::Config::LookUp("system.port",(int)8080,"system port");
mo1al::ConfigVar<float>::ptr g_float_value_config = mo1al::Config::LookUp("system.value",(float)10.2f,"system value");
mo1al::ConfigVar<std::vector<int>>::ptr g_int_vec_value_config = mo1al::Config::LookUp("system.int_vec",std::vector<int>{1,2},"system int vec");
void test_yaml(YAML::Node& root)
{
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<root;
}
void print_yaml(const YAML::Node& node, int level) { // 递归遍历 node 是当前处理的yaml节点 level 当前节点的深度
    if(node.IsScalar()) { //标量
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level*4 , ' ')
            << node.Scalar() << " - " << node.Type() << " - " << level;
    } else if(node.IsNull()) { // 空值
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level*4, ' ')
            << "NULL - " << node.Type() << " - " << level;
    } else if(node.IsMap()) { // 映射
        for(auto it = node.begin();
                it != node.end(); ++it) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level*4, ' ')
                    << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    } else if(node.IsSequence()) { //列表
        for(size_t i = 0; i < node.size(); ++i) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level*4, ' ')
                << i << " - " << node[i].Type() << " - " << level; // type = int 
            print_yaml(node[i], level + 1);
        }
    }
}
void test_config()
{
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before : " << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before : " << g_float_value_config->toString();
#define XX(g_var,name,prefix)\
    {\
        auto&v = g_var->getValue();\
        for(auto&i:v){\
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name ": " << i;\
        }\
    }
    XX(g_int_vec_value_config, int_vec, before);       
    YAML::Node root = YAML::LoadFile("/home/mo1al/redo-sylar/bin/conf/log.yaml");
    mo1al::Config::LoadFromYaml(root); // static的好处  LoadFromYaml string->T 
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after : " << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after : " << g_float_value_config->toString();    
    XX(g_int_vec_value_config, int_vec, after);
}

#endif

class Person
{
public:
    std::string name = "";
    int age = 0;
    bool sex = false;
    Person(){}
    std::string toString() const
    {
        std::stringstream ss;
        ss<<"[Person ["<< "name : " <<name 
        << " age : "<<age
        << " sex : "<<sex<<"]]";
        return ss.str();
    }
    bool operator==(const Person&old) const
    {
        return old.name==name&&old.age==age&&old.sex==sex;
    }
};
namespace mo1al
{
template<>
class LexicalCast<std::string,Person>
{
public:
    Person operator()(const std::string&v)
    {   
        YAML::Node node(YAML::Load(v));
        Person p;
        p.name = node["name"].as<std::string>();
        p.age = node["age"].as<int>();
        p.sex = node["sex"].as<bool>();
        return p;
    }
};
template<>
class LexicalCast<Person,std::string>
{
public:
    std::string operator()(const Person &p)
    {   
        YAML::Node node;
        // 结构体的创建
        node["name"] = p.name;
        node["age"] = p.age;
        node["sex"] = p.sex;
        std::stringstream ss;
        ss<<node;
        return ss.str();
    }
};
}

mo1al::ConfigVar<Person>::ptr g_person =
    mo1al::Config::LookUp("class.person", Person(), "system person");

mo1al::ConfigVar<std::map<std::string, Person> >::ptr g_person_map =
    mo1al::Config::LookUp("class.map", std::map<std::string, Person>(), "system person");

mo1al::ConfigVar<std::map<std::string, std::vector<Person> > >::ptr g_person_vec_map =
    mo1al::Config::LookUp("class.vec_map", std::map<std::string, std::vector<Person> >(), "system person");

void test_class() {
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_person->getValue().toString() << " - " << g_person->toString();

#define XX_PM(g_var, prefix) \
    { \
        auto m = g_person_map->getValue(); \
        for(auto& i : m) { \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) <<  prefix << ": " << i.first << " - " << i.second.toString(); \
        } \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) <<  prefix << ": size=" << m.size(); \
    }

    g_person->addListener([](const Person& old_value, const Person& new_value){
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "old_value=" << old_value.toString()
                << " new_value=" << new_value.toString();
    });

    XX_PM(g_person_map, "class.map before");
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_person_vec_map->toString();

    YAML::Node root = YAML::LoadFile("/home/mo1al/redo-sylar/bin/conf/test.yml");
    mo1al::Config::LoadFromYaml(root);

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_person->getValue().toString() << " - " << g_person->toString();
    XX_PM(g_person_map, "class.map after");
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_person_vec_map->toString();
}

void test_log()
{
    static mo1al::Logger::ptr system_log = SYLAR_LOG_NAME("system");
    // 试试root能不能输出
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"hello root"<<std::endl;
    SYLAR_LOG_INFO(system_log)<<"hello system"<<std::endl; //第一层问题 为什么不输出呢 
    std::cout<<"默认设置"<<std::endl;
    std::cout<<mo1al::LoggerMgr::GetInstance()->toYamlString()<<std::endl;
    // std::cout<<"......."<<std::endl;
    std::cout<<"加载文件ing"<<std::endl;
    // 第3层问题 什么时候加载出自己的system的呢
    YAML::Node root = YAML::LoadFile("/home/mo1al/redo-sylar/bin/conf/log.yaml");
    mo1al::Config::LoadFromYaml(root);
    // std::cout << "=============" << std::endl;
    std::cout<<"加载yaml文件后"<<std::endl;
    std::cout << mo1al::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    // std::cout << "=============" << std::endl;
    std::cout<<"展示yaml文件"<<std::endl;
    std::cout << root << std::endl;
    // std::cout<<"111111111"<<std::endl;
    std::cout<<"修改formatter后"<<std::endl;
    system_log->setFormatter("%d - %m%n");
    std::cout << mo1al::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    SYLAR_LOG_INFO(system_log)<<"hello system" ; //第二层问题 为什么appender的formatter没有修改成功呢 修改成功但是子节点的appender没有一起修改
}

int main()
{
    // YAML::Node root = YAML::LoadFile("/home/mo1al/redo-sylar/bin/conf/log.yaml");
    // test_yaml(root);
    // std::cout<<"----------------------------------------------------------"<<"\n";
    // print_yaml(root,0);
    // test_config();
    // test_class();
    // test_log();
    mo1al::Config::visit([](mo1al::ConfigVarBase::ptr var){
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"name = "<<var->getName()<<
            " descreption = "<<var->getDescription()<< " typename = "<<var->getTypeName()
                << " value = "<< var->toString();
    }); // 通过内部的实现传过来的 
    return 0;
}