// log.h —— 日志模块：日志级别、日志事件、格式化器、输出器与日志器管理。
#pragma once
#include<string>
#include<memory>
#include<list>
#include<iostream>
#include<sstream>
#include<fstream>
#include<stdint.h>
#include<vector>
#include<tuple>
#include<set>
#include<map>
#include<functional>
#include <stdarg.h>
#include"sinleton.h"
#include"util.h"
#include<thread.h>
// 流式日志宏：构造日志事件并返回其字符串流，供 << 运算符写入日志内容
#define SYLAR_LOG_LEVEL(logger,level)\
    if(logger->getLevel()<=level)\
        mo1al::LogEventWrap(mo1al::LogEvent::ptr(new mo1al::LogEvent(logger,level,__FILE__,\
        __LINE__,0,mo1al::getThreadId(),mo1al::getFiberId(),time(0),mo1al::Thread::GetName()))).getSS()\

#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger,mo1al::LogLevel::DEBUG)
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger,mo1al::LogLevel::INFO)
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, mo1al::LogLevel::WARN)
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, mo1al::LogLevel::ERROR)
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, mo1al::LogLevel::FATAL)
// 格式化日志宏：按 printf 风格格式串写入日志内容
#define SYLAR_LOG_FORMAT_LEVEL(logger,level,fmt,...)\
    if(logger->getLevel()<=level){\
        mo1al::LogEventWrap(mo1al::LogEvent::ptr(new mo1al::LogEvent(logger,level,__FILE__,\
        __LINE__,0,mo1al::getThreadId(),mo1al::getFiberId(),time(0),"thread_0"))).getEvent()->format(fmt,__VA_ARGS);\
    }
#define SYLAR_LOG_FORMAT_DEBUG(logger,fmt,...)  SYLAR_LOG_FORMAT_LEVEL(logger,mo1al::LogLevel::DEBUG,fmt,...)
#define SYLAR_LOG_FORMAT_INFO(logger,fmt,...)  SYLAR_LOG_FORMAT_LEVEL(logger,mo1al::LogLevel::INFO,fmt,...)
#define SYLAR_LOG_FORMAT_WARN(logger,fmt,...)  SYLAR_LOG_FORMAT_LEVEL(logger,mo1al::LogLevel::WARN,fmt,...)
#define SYLAR_LOG_FORMAT_ERROR(logger,fmt,...)  SYLAR_LOG_FORMAT_LEVEL(logger,mo1al::LogLevel::ERROR,fmt,...)
#define SYLAR_LOG_FORMAT_FATAL(logger,fmt,...)  SYLAR_LOG_FORMAT_LEVEL(logger,mo1al::LogLevel::FATAL,fmt,...)
// 获取根日志器 / 具名日志器
#define SYLAR_LOG_ROOT()  mo1al::LoggerMgr::GetInstance()->getRoot()
#define SYLAR_LOG_NAME(name) mo1al::LoggerMgr::GetInstance()->getLogger(name)

namespace mo1al{

class Logger;

class LogLevel{
public:
    typedef std::shared_ptr<LogLevel>ptr;
    enum Level{
        UNKOWN = 0,
        DEBUG,INFO,WARN,ERROR,FATAL
    };
    // 日志级别转为字符串
    static const char* ToString(LogLevel::Level level);
    // 字符串转为日志级别（大小写不敏感）
    static LogLevel::Level FromString(const std::string&str);
};
// 日志事件：封装一条日志所需的全部信息（文件、行号、耗时、线程/协程 ID、时间、线程名、内容）
class LogEvent{
public:
    typedef std::shared_ptr<LogEvent>ptr;
    // 构造：记录日志元信息，正文通过 getSS() 流式写入
    LogEvent(std::weak_ptr<Logger>logger,LogLevel::Level level,const char*file,int32_t line,uint32_t elapse,
        uint32_t threadid,uint32_t fiberid,uint64_t time,std::string threadname);
    // 以下为各字段的读取接口
    const char* getFile()const{return m_file;}
    int32_t getLine()const{return m_line;}
    uint32_t getElapse()const{return m_elapse;}
    uint32_t getThreadId()const{return m_threadId;}
    uint32_t getFiberId() const {return m_fiberId;}
    uint64_t getTime()const {return m_time;}
    const std::string& getThreadName()const{return m_threadName;}
    std::string getContent()const{return m_ss.str();}
    std::stringstream& getSS(){return m_ss;}
    LogLevel::Level getLevel()const{return m_level;}
    std::weak_ptr<Logger> getLogger()const{return m_logger;}
    // 按 printf 格式写入日志内容
    void format(const char*fmt,...);
    void format(const char*fmt,va_list al);
private:
    const char* m_file = nullptr;
    int32_t m_line = 0;
    uint32_t m_elapse = 0;
    uint32_t m_threadId;
    uint32_t m_fiberId;
    uint64_t m_time;
    std::string m_threadName;
    std::stringstream m_ss; // 流式存储日志正文
    LogLevel::Level m_level;
    std::weak_ptr<Logger>m_logger;
};
// 日志格式化器：将 pattern 解析为若干 FormatItem，按序输出各日志字段
class LogFormatter{
public:
    typedef std::shared_ptr<LogFormatter>ptr;
    LogFormatter(const std::string&pattern):m_patterns(pattern){init();}

    std::string formatter(std::shared_ptr<Logger>logger,LogLevel::Level level,LogEvent::ptr event);

    std::ostream& formatter(std::ostream&ofs,std::shared_ptr<Logger>logger,LogLevel::Level level,LogEvent::ptr event);
    std::string getPattern()const{return m_patterns;}
public:
    // 格式化项基类：负责输出日志的单个字段
    class FormatItem
    {
    public:
        typedef std::shared_ptr<FormatItem>ptr;
        FormatItem(const std::string&str=""){};
        virtual ~FormatItem(){};
        virtual void format(std::ostream&os,std::shared_ptr<Logger>logger,LogLevel::Level level,LogEvent::ptr event)=0;
    };
    void init();
    bool is_error()const {return m_error;}
private:
    bool m_error = false;
    // 日志格式模板串
    std::string m_patterns;
    LogLevel::Level m_level=LogLevel::DEBUG;
    // 解析后的格式项列表
    std::vector<FormatItem::ptr>m_items;
};
// 日志输出器基类：将日志写入具体目标（标准输出、文件等）
class LogAppender{
    friend class Logger;
public:
    typedef std::shared_ptr<LogAppender>ptr;
    typedef mo1al::SpinLock MutexType;
    // 由派生类实现具体的写入逻辑
    virtual void log(std::shared_ptr<Logger>logger,LogLevel::Level level,LogEvent::ptr event)=0;
    virtual std::string toYamlString() = 0;
    void setFormatter(LogFormatter::ptr formatter);
    LogFormatter::ptr getFormatter();
    void setLevel(LogLevel::Level level) {m_level = level;}
protected:
    MutexType m_mutex;
    bool has_formatter = false;
    LogFormatter::ptr m_formatter;
    LogLevel::Level m_level=LogLevel::DEBUG;
};
// 日志器：持有若干输出器，按级别分发日志
class Logger: public std::enable_shared_from_this<Logger>{
    friend class LoggerManager;
public:
    typedef std::shared_ptr<Logger>ptr;
    typedef mo1al::SpinLock MutexType;
    Logger(const std::string&name="root");
    void log(LogLevel::Level level,LogEvent::ptr event);
    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);
    std::string getName() const{ return m_name;}
    LogLevel::Level getLevel()const{ return m_level;}
    void setLevel(LogLevel::Level level) { m_level = level;}
    void setFormatter(LogFormatter::ptr formatter) {
        MutexType::Lock lock(m_mutex);
        m_formatter = formatter;
        for(auto&i:m_appenders){
            MutexType::Lock ll(i->m_mutex); // 加输出器内部锁，避免并发修改
            if(!i->has_formatter){ // 仅当输出器未自定义 formatter 时才覆盖
                i->m_formatter = m_formatter;
            }
        }
    }
    void setFormatter(std::string formatter) {
        mo1al::LogFormatter::ptr new_val(new mo1al::LogFormatter(formatter));
        if(new_val->is_error()){
            return ;
        }
        setFormatter(new_val);
    }
    void addAppender(LogAppender::ptr appender)
    {
        MutexType::Lock lock(m_mutex);
        if(!appender->getFormatter())
        {
            MutexType::Lock ll(appender->m_mutex);
            appender->m_formatter=m_formatter;
        }
        m_appenders.push_back(appender);
    }
    void delAppender(LogAppender::ptr appender){
        MutexType::Lock lock(m_mutex);
        for(auto&i:m_appenders){
            if(i==appender){
                m_appenders.remove(appender);
            }
        }
    }
    void clearAppender(){
        MutexType::Lock lock(m_mutex);
        m_appenders.clear();
    }
    std::string toYamlString();
private:
    std::string m_name ; // 日志器名称
    std::list<LogAppender::ptr>m_appenders;
    LogFormatter::ptr m_formatter;
    LogLevel::Level m_level;
    Logger::ptr m_root; // 指向根日志器，非根日志器无输出器时回退到根
    MutexType m_mutex;
};

// 日志事件包装器：析构时自动触发日志写入（RAII）
class LogEventWrap
{
public:
    typedef std::shared_ptr<LogEventWrap>ptr;
    LogEventWrap(LogEvent::ptr e);
    ~LogEventWrap();
    std::stringstream& getSS()const;
    LogEvent::ptr getEvent()const{return m_event;}
private:
    LogEvent::ptr m_event;
};
// 日志器管理器：负责创建与查找日志器，并响应配置变更
class LoggerManager
{
public:
    typedef mo1al::SpinLock MutexType;
    LoggerManager();
    Logger::ptr getLogger(const std::string& name);
    void init();
    Logger::ptr getRoot() const { return m_root;}
    std::string toYamlString();
private:
    std::map<std::string,Logger::ptr>m_loggers; // 名称到日志器的映射（含根日志器）
    Logger::ptr m_root;
    MutexType m_mutex;
};

// 日志器管理器单例
typedef mo1al::Singleton<LoggerManager> LoggerMgr;

// 标准输出日志输出器
class StdCoutAppender:public LogAppender{
public:
    typedef std::shared_ptr<StdCoutAppender>ptr;
    void log(std::shared_ptr<Logger>logger,LogLevel::Level level,LogEvent::ptr event)override;
    std::string toYamlString()override;
};

// 文件日志输出器
class FileAppender:public LogAppender{
public:
    typedef std::shared_ptr<FileAppender>ptr;
    FileAppender(const std::string&name); // 构造时打开日志文件
    void log(std::shared_ptr<Logger>logger,LogLevel::Level level,LogEvent::ptr event)override;
    std::string toYamlString()override;
    bool reopen();
private:
    std::string m_filename;
    std::ofstream ofs;// 文件输出流
    uint64_t m_lastTime = 0;
};
// 以下为各日志字段对应的格式化项实现
class FileNameFormatItem:public LogFormatter::FormatItem{
public:
    using LogFormatter::FormatItem::FormatItem; // 复用父类构造函数
    void format(std::ostream&os,std::shared_ptr<Logger>logger,LogLevel::Level level,LogEvent::ptr event)override{
        os<<event->getFile();
    }
};
class LineFormatItem:public LogFormatter::FormatItem{
public:
    using LogFormatter::FormatItem::FormatItem; // 复用父类构造函数
    void format(std::ostream&os,std::shared_ptr<Logger>logger,LogLevel::Level level,LogEvent::ptr event)override{
        os<<event->getLine();
    }
};
class ElapseFormatItem:public LogFormatter::FormatItem{
public:
    using LogFormatter::FormatItem::FormatItem; // 复用父类构造函数
    void format(std::ostream&os,std::shared_ptr<Logger>logger,LogLevel::Level level,LogEvent::ptr event)override{
        os<<event->getElapse();
    }
};
class ThreadIdFormatItem:public LogFormatter::FormatItem{
public:
    using LogFormatter::FormatItem::FormatItem; // 复用父类构造函数
    void format(std::ostream&os,std::shared_ptr<Logger>logger,LogLevel::Level level,LogEvent::ptr event)override{
        os<<event->getThreadId();
    }
};
class FiberIdFormatItem:public LogFormatter::FormatItem{
public:
    using LogFormatter::FormatItem::FormatItem; // 复用父类构造函数
    void format(std::ostream&os,std::shared_ptr<Logger>logger,LogLevel::Level level,LogEvent::ptr event)override{
        os<<event->getFiberId();
    }
};
class ThreadNameFormatItem:public LogFormatter::FormatItem{
public:
    using LogFormatter::FormatItem::FormatItem; // 复用父类构造函数
    void format(std::ostream&os,std::shared_ptr<Logger>logger,LogLevel::Level level,LogEvent::ptr event)override{
        os<<event->getThreadName();
    }
};

// 日期时间格式化项：按 strftime 格式输出日志时间
class DateTimeFormatItem:public LogFormatter::FormatItem{
public:
    DateTimeFormatItem(const std::string&format="%Y-%m-%d %H-%M-%S"):m_format(format){
        if(m_format.empty()){
            m_format = "%Y-%m-%d %H-%M-%S";
        }
    }
    void format(std::ostream&os,std::shared_ptr<Logger>logger,LogLevel::Level level,LogEvent::ptr event)override
    {
        struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time,&tm);
        char buf[64];
        strftime(buf,sizeof(buf),m_format.c_str(),&tm);
        os<<buf;
    }
private:
    std::string m_format;
};

// 消息正文格式化项
class MessageFormatItem:public LogFormatter::FormatItem{
public:
    using LogFormatter::FormatItem::FormatItem; // 复用父类构造函数
    void format(std::ostream&os,std::shared_ptr<Logger>logger,LogLevel::Level level,LogEvent::ptr event)override{
        os<<event->getContent();
    }
};
// 日志级别格式化项
class LevelFormatItem:public LogFormatter::FormatItem{
public:
    using LogFormatter::FormatItem::FormatItem; // 复用父类构造函数
    void format(std::ostream&os,std::shared_ptr<Logger>logger,LogLevel::Level level,LogEvent::ptr event)override{
        os<<LogLevel::ToString(event->getLevel());
    }
};
// 日志器名称格式化项
class NameFormatItem:public LogFormatter::FormatItem{
public:
    using LogFormatter::FormatItem::FormatItem; // 复用父类构造函数
    void format(std::ostream&os,std::shared_ptr<Logger>logger,LogLevel::Level level,LogEvent::ptr event)override{
        os<<logger->getName() ;
    }
};
// 字符串格式化项：原样输出给定的字符串
class StringFormatItem:public LogFormatter::FormatItem{
public:
    StringFormatItem(const std::string str):m_string(str){}
    void format(std::ostream&os,std::shared_ptr<Logger>logger,LogLevel::Level level,LogEvent::ptr event)override
    {
        os<<m_string;
    }
private:
    std::string m_string;
};
// 换行格式化项
class NewLineFormatItem:public LogFormatter::FormatItem
{
public:
    NewLineFormatItem(const std::string&str):m_string(str){}
    void format(std::ostream&os,std::shared_ptr<Logger>logger,LogLevel::Level level,LogEvent::ptr event)override
    {
        os<<"\n";
    }
private:
    std::string m_string;
};
// 制表符格式化项
class TabFormatItem:public LogFormatter::FormatItem
{
public:
    TabFormatItem(const std::string&str):m_string(str){}
    void format(std::ostream&os,std::shared_ptr<Logger>logger,LogLevel::Level level,LogEvent::ptr event)override
    {
        os<<"\t";
    }
private:
    std::string m_string;
};


}