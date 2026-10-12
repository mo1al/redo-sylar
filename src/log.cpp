#include"log.h"
#include"config.h"
#include<iostream>
namespace mo1al{
// 将日志级别枚举转为字符串
const char* LogLevel::ToString(LogLevel::Level level)
{
    switch(level){
    #define XX(name)\
    case LogLevel::name:\
        return #name;\
        break;
        XX(LogLevel::DEBUG);
        XX(LogLevel::INFO);
        XX(LogLevel::WARN);
        XX(LogLevel::ERROR);
        XX(LogLevel::FATAL);
    #undef XX
    default:
        return "UNKOWN";
    }
    return "UNKOWN";
}
LogLevel::Level LogLevel::FromString(const std::string&str)
{
    #define XX(level,v)\
    if(str==#v){\
        return level;\
    }
    // 大小写均支持
    XX(LogLevel::DEBUG,debug);
    XX(LogLevel::INFO,info);
    XX(LogLevel::WARN,warn);
    XX(LogLevel::ERROR,error);
    XX(LogLevel::FATAL,fatal);

    XX(LogLevel::DEBUG, DEBUG);
    XX(LogLevel::INFO, INFO);
    XX(LogLevel::WARN, WARN);
    XX(LogLevel::ERROR, ERROR);
    XX(LogLevel::FATAL, FATAL);
    #undef XX
    return UNKOWN;
}
LogEvent::LogEvent(std::weak_ptr<Logger>logger,LogLevel::Level level,const char*file,int32_t line,uint32_t elapse,
    uint32_t threadid,uint32_t fiberid,uint64_t time,std::string threadname):
    m_logger(logger),m_level(level),m_file(file),m_line(line),m_elapse(elapse),m_threadId(threadid),
    m_fiberId(fiberid),m_time(time),m_threadName(threadname)
{

}
void LogEvent::format(const char* fmt,...)
{
    va_list al;
    va_start(al,fmt);
    format(fmt,al);
    va_end(al);
}

void LogEvent::format(const char*fmt,va_list al)
{
    char*buffer = nullptr;
    int len = vasprintf(&buffer,fmt,al);
    if(len!=-1)
    {
        m_ss<<std::string(buffer,len);
        free(buffer);
    }
}
// 将日志事件按各格式项输出到字符串
std::string LogFormatter::formatter(std::shared_ptr<Logger>logger,LogLevel::Level level,LogEvent::ptr event)
{
    // 使用字符串流收集输出结果
    std::stringstream ss;
    if(m_level<=level){ // 级别过滤：低于格式化器级别的日志不输出
        for(auto&i:m_items){
            i->format(ss,logger,level,event); // 依次执行各格式项
        }
    }
    return ss.str();
}
std::ostream& LogFormatter::formatter(std::ostream&ofs,std::shared_ptr<Logger>logger,LogLevel::Level level,LogEvent::ptr event)
{
    if(m_level<=level){ 
        for(auto&i:m_items){
            i->format(ofs,logger,level,event);
        }
    }
    return ofs; // 返回输出流以便链式调用
}

// 解析 pattern：支持 %xxx、%xxx{fmt} 与 %%（转义为字面 %）
void LogFormatter::init(){
    // 三元组：<原始字符串, 格式化串, 类型>，type 0 为普通字符串，1 为格式化项
    std::vector<std::tuple<std::string,std::string,int>>vec;
    std::string n_str;
    for(size_t i=0;i<m_patterns.size();i++){
        if(m_patterns[i]!='%'){
            n_str.append(1,m_patterns[i]); // 累积普通字符
            continue;
        }
        // %% 转义为单个 %
        if(m_patterns[i+1]=='%'&&i+1<m_patterns.size()){
            n_str.append(1,m_patterns[i+1]);
            continue;
        }
        std::string str;
        std::string fmt;
        int status = 0;
        size_t n = i+1;
        size_t fmt_begin = 0;
        while(n<m_patterns.size()){
            if(status==0&&!isalpha(m_patterns[n])&&m_patterns[n]!='{'&&m_patterns[n]!='}'){
                str=m_patterns.substr(i+1,n-i-1); // 截取格式项名称
                break;
            }
            if(status==0){
                if(m_patterns[n]=='{'){
                    // 遇到 { 时截取格式项名称，并记录 fmt 起始位置
                    str = m_patterns.substr(i+1,n-i-1);
                    fmt_begin = n+1;
                    status = 1;
                    n++;
                    continue;
                }
            }
            if(status==1){
                if(m_patterns[n]=='}'){
                    fmt = m_patterns.substr(fmt_begin,n-fmt_begin);
                    status = 0;
                    n++;
                    break;
                }
            }
            n++;
            if(n==m_patterns.size()){  // 无 {} 时也需截取完整格式项
                if(str.empty()){
                    str=m_patterns.substr(i+1);
                }
            }
        }
        if(status==0){
            if(!n_str.empty()){
                vec.push_back(std::make_tuple(n_str,"",0));
                n_str.clear();
            }
            vec.push_back(std::make_tuple(str,fmt,1));
            i = n-1;
        }
        else{
            // 括号不匹配，记录错误
            vec.push_back(std::make_tuple(str,"<pattern_error>",0));
            m_error = true;
        }
    }
    if(!n_str.empty()){
        vec.push_back(std::make_tuple(n_str,"",0));
    }
    // 工厂模式 
    static std::map<std::string,std::function<FormatItem::ptr(const std::string&str)>> m =
    {
    #define XX(str,C)\
    {#str,[](const std::string&fmt){return FormatItem::ptr(new C(fmt));}}
    XX(m, MessageFormatItem),           //m:消息
    XX(p, LevelFormatItem),             //p:日志级别
    XX(r, ElapseFormatItem),            //r:累计毫秒数
    XX(c, NameFormatItem),              //c:日志名称
    XX(t, ThreadIdFormatItem),          //t:线程id
    XX(n, NewLineFormatItem),           //n:换行
    XX(d, DateTimeFormatItem),          //d:时间
    XX(f, FileNameFormatItem),          //f:文件名
    XX(l, LineFormatItem),              //l:行号
    XX(T, TabFormatItem),               //T:Tab
    XX(F, FiberIdFormatItem),           //F:协程id
    XX(N, ThreadNameFormatItem),        //N:线程名称
    // 拓展只需要写一个FormatItem子类+XX()
    #undef XX
    };
    for(auto&[str,fmt,type]:vec){
        if(type==0){
            m_items.push_back(FormatItem::ptr(new StringFormatItem(str)));
        }
        else{
            auto it = m.find(str);
            if(it==m.end()){
                // 未知格式项，记录错误
                m_items.push_back(FormatItem::ptr(new StringFormatItem("<error_format,str:"+str+">")));
                m_error = true;
            }
            else{
                // 命中格式项，构造并加入列表
                m_items.push_back(it->second(fmt));
            }
        }
    }
}
Logger::Logger(const std::string&name)  :
m_name(name),m_level(LogLevel::DEBUG),m_formatter(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%T%m%n"))
{

}
void Logger::log(LogLevel::Level level,LogEvent::ptr event){
    if(level>=m_level){ // 级别过滤：低于日志器级别的日志不输出
        MutexType::Lock lock(m_mutex);
        auto self = shared_from_this();
        if(!m_appenders.empty()){
            // 分发给各输出器处理
            for(auto&i:m_appenders){
                i->log(self,level,event); // 输出器内部自行加锁
            }
        } else if(m_root){
            m_root->log(level,event);
        }
    }
}

void Logger::debug(LogEvent::ptr event){
    log(LogLevel::DEBUG,event);
}
void Logger::info(LogEvent::ptr event){
    log(LogLevel::INFO,event);
}
void Logger::warn(LogEvent::ptr event){
    log(LogLevel::WARN,event);
}
void Logger::error(LogEvent::ptr event){
    log(LogLevel::ERROR,event);
}
void Logger::fatal(LogEvent::ptr event){
    log(LogLevel::FATAL,event);
}

std::string Logger::toYamlString(){
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["name"] = m_name;
    if(m_level!=LogLevel::UNKOWN){
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_formatter){
        node["formatter"] = m_formatter->getPattern();
    }
    for(auto&i:m_appenders){
        node["appenders"].push_back(YAML::Load(i->toYamlString()));
    }
    std::stringstream ss;
    ss<<node;
    return ss.str();
}

LogEventWrap::LogEventWrap(LogEvent::ptr e):m_event(e)
{

}
LogEventWrap::~LogEventWrap()
{
    // weak_ptr避免循环引用的问题
    auto logger = m_event->getLogger().lock(); // 从 weak_ptr 提升为 shared_ptr
    if(logger){
        logger->log(m_event->getLevel(),m_event);
    }
}
LoggerManager::LoggerManager()
{
    m_root.reset(new Logger);
    m_root->addAppender(LogAppender::ptr(new StdCoutAppender));
    m_loggers[m_root->m_name] = m_root;
    
}
Logger::ptr LoggerManager::getLogger(const std::string& name)
{
    MutexType::Lock lock(m_mutex);
    auto it = m_loggers.find(name); // 查找已有日志器
    if(it != m_loggers.end()) {
        return it->second;
    }
    // 不存在则新建，并指向根日志器
    Logger::ptr logger(new Logger(name));
    logger->m_root = m_root;
    m_loggers[name] = logger;
    return logger;
}
void LoggerManager::init()
{

}
std::stringstream& LogEventWrap::getSS()const
{
    return m_event->getSS();
}

void LogAppender::setFormatter(LogFormatter::ptr formatter)
{
    MutexType::Lock lock(m_mutex);
    m_formatter=formatter;
    if(m_formatter) has_formatter = true;
    else has_formatter = false;
}
LogFormatter::ptr LogAppender::getFormatter()
{
    MutexType::Lock lock(m_mutex);
    return m_formatter;
}
void StdCoutAppender::log(std::shared_ptr<Logger>logger,LogLevel::Level level,LogEvent::ptr event)
{
    if(level>=m_level)
    {
        MutexType::Lock lock(m_mutex);
        m_formatter->formatter(std::cout,logger,level,event);
    }
}
std::string StdCoutAppender::toYamlString(){
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    if(m_level!=LogLevel::UNKOWN){
        node["level"] = LogLevel::ToString(m_level);
    }
    if(has_formatter&&m_formatter){
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss<<node;
    return ss.str();
}
FileAppender::FileAppender(const std::string&name) : m_filename(name)
{
    reopen();
}
void FileAppender::log(std::shared_ptr<Logger>logger,LogLevel::Level level,LogEvent::ptr event)
{
    if(level>=m_level)
    {
        uint64_t now = event->getTime();
        if(now>(m_lastTime+3)){
            reopen();
            m_lastTime = now;
        }
        MutexType::Lock lock(m_mutex);
        if(!m_formatter->formatter(ofs,logger,level,event))
        {
            std::cout<<"error"<<std::endl;
        }
    }
}
std::string FileAppender::toYamlString(){
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["file"] = m_filename;
    if(m_level!=LogLevel::UNKOWN) node["level"] = LogLevel::ToString(m_level);
    if(has_formatter&&m_formatter){
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss<<node;
    return ss.str();
}
bool FileAppender::reopen()
{
    MutexType::Lock lock(m_mutex);
    if(ofs){
        ofs.close();
    }
    ofs.open(m_filename.c_str(),std::ios::app);
    return ofs.is_open();
}

// 日志输出器配置结构：type 1 为文件，2 为标准输出
struct LogAppenderDefine
{
    int type = 0; // 1 File 2 Cout
    LogLevel::Level level = LogLevel::UNKOWN;
    std::string formatter;
    std::string file;
    bool operator==(const LogAppenderDefine&th) const
    {
        return type==th.type&&level==th.level&&formatter==th.formatter&&file==th.file;
    }
};

// 日志器配置结构
struct LogDefine
{
    std::string name;
    LogLevel::Level level = LogLevel::UNKOWN;
    std::string formatter;
    std::vector<LogAppenderDefine>appenders;
    bool operator==(const LogDefine&th)const
    {
        return name==th.name&&level==th.level&&formatter==th.formatter&&appenders==th.appenders;
    }
    bool operator<(const LogDefine&th)const
    {
        return name<th.name;
    }
};

// 从 YAML 字符串解析日志器配置
template<>
class LexicalCast<std::string,LogDefine>
{
public:
    LogDefine operator()(const std::string&v){
        YAML::Node node = YAML::Load(v);
        LogDefine ld;
        if(!node["name"].IsDefined()){
            std::cout<<"log config error, name is null "<<node<<std::endl;
            throw std::logic_error("log config name is null");
        }
        ld.name = node["name"].as<std::string>();
        ld.level = LogLevel::FromString(node["level"].IsDefined()?node["level"].as<std::string>():"");
        if(node["formatter"].IsDefined()){
           ld.formatter = node["formatter"].as<std::string>();
        }
        if(node["appenders"].IsDefined()){
            for(size_t x=0;x<node["appenders"].size();x++){
                auto a = node["appenders"][x];
                if(!a["type"].IsDefined()){
                    std::cout << "log config error: appender type is null, " << a << std::endl;
                    continue;
                }
                std::string type = a["type"].as<std::string>();
                LogAppenderDefine lad;
                if(type=="FileLogAppender"){
                    lad.type = 1;
                    if(!a["file"].IsDefined()){
                        std::cout<<"log config error : fileappender file is null"<<a<<std::endl;
                        continue;
                    }
                    lad.file = a["file"].as<std::string>();
                    if(a["formatter"].IsDefined()){
                        lad.formatter = a["formatter"].as<std::string>();
                    }
                }
                else if(type=="StdoutLogAppender"){
                    lad.type = 2;
                    if(a["formatter"].IsDefined()){
                        lad.formatter = a["formatter"].as<std::string>();
                    }
                }
                else{
                    std::cout<<"log config error appender type is invalid"<<a<<std::endl;
                    continue;
                }
                ld.appenders.push_back(lad);
            }
        }
        return ld;
    }
};
// 将日志器配置序列化为 YAML 字符串
template<>
class LexicalCast<LogDefine,std::string>
{
public:
    std::string operator()(const LogDefine&ld){
        YAML::Node node;
        node["name"] = ld.name;
        if(ld.level!=LogLevel::UNKOWN){
            node["level"] = LogLevel::ToString(ld.level);
        }
        if(!ld.formatter.empty()) {
            node["formatter"] = ld.formatter;
        }
        for(auto&i:ld.appenders){
            YAML::Node n;
            if(i.type==1){
                n["type"] = "FileLogAppender";
                n["file"] = i.file;
            }
            else if(i.type==2){
                n["type"] = "StdoutLogAppender";
            }
            if(i.level!=LogLevel::UNKOWN){
                n["level"] = LogLevel::ToString(i.level);
            }
            if(!i.formatter.empty()){
                n["formatter"] = i.formatter;
            }
            node["appender"].push_back(n);
        }
        std::stringstream ss;
        ss<<node;
        return ss.str();
    }
};
// 全局日志配置项：logs 配置发生变更时触发回调
mo1al::ConfigVar<std::set<LogDefine>>::ptr g_log_defines =
        mo1al::Config::LookUp("logs",std::set<LogDefine>(),"logs config") ;

// 日志配置初始化器：监听 logs 配置变更，动态应用新增、修改与删除
struct LogIniter
{
    LogIniter()
    {
        g_log_defines->addListener([](const std::set<LogDefine>old_value,const std::set<LogDefine>new_value){
            // 处理配置变更：新增与修改
            for(auto&i:new_value){
                auto it = old_value.find(i);
                mo1al::Logger::ptr logger;
                if(it==old_value.end()){ // 新增：旧配置中不存在
                    logger = SYLAR_LOG_NAME(i.name);
                }
                else{
                    if(!(*it==i)){ // 已存在但内容变化，视为修改
                        logger = SYLAR_LOG_NAME(i.name);
                    }
                    else{ // 内容一致，跳过
                        continue;
                    }
                }
                logger->setLevel(i.level);
                if(!i.formatter.empty()){
                    logger->setFormatter(i.formatter);
                }
                logger->clearAppender();
                for(auto&ap:i.appenders){
                    mo1al::LogAppender::ptr  a ; // 根据类型创建对应输出器
                    if(ap.type==1){
                        a.reset(new FileAppender(ap.file));
                    }
                    else if(ap.type==2){
                        a.reset(new StdCoutAppender);
                    }
                    a->setLevel(ap.level);
                    if(!ap.formatter.empty()){
                        mo1al::LogFormatter::ptr matter(new mo1al::LogFormatter(ap.formatter));
                        if(matter->is_error()){
                            std::cout<<"log.name :"<< i.name<<"appender type"<<ap.type
                            <<"formatter"<<ap.formatter<<"is invalid"<<std::endl;
                        }
                        else{
                            a->setFormatter(matter);
                        }
                    }
                    logger->addAppender(a);
                }
            }
            // 处理删除：旧配置中存在但新配置中缺失的日志器
            for(auto&i:old_value){
                auto it = new_value.find(i);
                if(it==new_value.end()){ // 新配置中不存在，则删除
                    auto logger = SYLAR_LOG_NAME(i.name);
                    logger->setLevel(LogLevel::Level(100)); // 设为不可达级别以禁用该日志器
                    logger->clearAppender();
                }
            }
        });
    }
};
static LogIniter __log_init;
std::string LoggerManager::toYamlString(){
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    for(auto&i:m_loggers){
        node.push_back(YAML::Load(i.second->toYamlString()));
    }
    std::stringstream ss;
    ss<<node;
    return ss.str();
}
}