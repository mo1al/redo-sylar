// fd_manager.h —— 文件描述符上下文管理：记录 fd 的 socket/非阻塞/超时等状态。
#pragma once
#include<memory>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include"thread.h"
#include"sinleton.h"
namespace mo1al
{
// 文件描述符上下文：记录 fd 是否为 socket、非阻塞标志与收发超时
class FdCtx: public std::enable_shared_from_this<FdCtx>
{
public:
    typedef std::shared_ptr<FdCtx>ptr;
    FdCtx(int fd);
    ~FdCtx();
    bool isInit()const{return m_isInit;}
    bool isSocket()const{return m_isSocket;}
    bool isClose()const{return m_isClosed;}
    void setUserNonblock(bool v){m_userNonblock=v;}
    bool getUserNonblock() const { return m_userNonblock;}
    void setSysNonblock(bool v){ m_sysNonblock=v;}
    bool getSysNonblock()const{return m_sysNonblock;}
    void setTimeout(int type,uint64_t v);
    uint64_t getTimeout(int type);
private:
    bool init();
private:
    int m_fd;
    bool m_isInit:1;
    bool m_isSocket:1;
    bool m_sysNonblock:1;
    bool m_userNonblock:1;
    bool m_isClosed:1;
    uint64_t m_recvTimeout;
    uint64_t m_sendTimeout;
};
// 文件描述符管理器：维护 fd 到上下文的映射
class FdManager
{
public:
    typedef RWmutex MutexType;
    FdManager();
    FdCtx::ptr get(int fd,bool auto_create=false);
    void del(int fd);
private:
    MutexType m_mutex;
    std::vector<FdCtx::ptr>m_datas;
};
typedef Singleton<FdManager>FdMgr;
}