#include"hook.h"
#include"iomanager.h"
#include"fiber.h"
#include"fd_manager.h"
#include"log.h"
#include"config.h"
static mo1al::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
namespace mo1al
{
static thread_local bool t_hook_enable = false;
static mo1al::ConfigVar<int>::ptr g_tcp_connect_timeout =
    mo1al::Config::LookUp("tcp.connect.timeout",5000,"tcp connect timeout");
#define HOOK_FUN(XX)\
    XX(sleep)\
    XX(usleep)\
    XX(nanosleep)\
    XX(socket)\
    XX(connect)\
    XX(accept)\
    XX(read)\
    XX(readv)\
    XX(recv)\
    XX(recvfrom)\
    XX(recvmsg)\
    XX(write)\
    XX(writev)\
    XX(send)\
    XX(sendto)\
    XX(sendmsg)\
    XX(close)\
    XX(fcntl)\
    XX(ioctl)\
    XX(getsockopt)\
    XX(setsockopt)
// 通过 dlsym 解析 libc 原始函数地址
void hook_init()
{
    static bool is_inited = false;
    if(is_inited){
        return ;
    }
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT,#name);
    HOOK_FUN(XX);
#undef XX
}

static uint64_t s_connect_timeout = -1;
struct _HookIniter
{
    _HookIniter(){
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getValue();
        g_tcp_connect_timeout->addListener([](const int&old_value,const int&new_value){
            SYLAR_LOG_INFO(g_logger)<< "tcp connect timeout changed from "
                                        << old_value << " to " << new_value;
            s_connect_timeout = new_value;
        });
    }
};
static _HookIniter s_hook_initer;

bool is_hook_enable()
{
    return t_hook_enable;
}
void set_hook_enable(bool flag)
{
    t_hook_enable = flag;
}
}

struct timer_info{
    int cancelled = 0 ;
};

// IO 操作的统一封装：处理非阻塞、超时与协程让出
template<typename OriginFun,typename... Args>
static ssize_t do_io(int fd,OriginFun fun,const char*hook_fun_name,uint32_t event,
    int timeout_so,Args&&... args)
{
    // hook 未启用时直接调用原始函数
    if(!mo1al::t_hook_enable){
        return fun(fd,std::forward<Args>(args)...);
    }
    mo1al::FdCtx::ptr ctx = mo1al::FdMgr::GetInstance()->get(fd);
    if(!ctx){
        return fun(fd,std::forward<Args>(args)...);
    }
    if(ctx->isClose()){
        errno = EBADF;
        return -1;
    }
    if(!ctx->isSocket()||ctx->getUserNonblock()){ // 非 socket 或用户已设非阻塞，直接调用原始函数
        return fun(fd,std::forward<Args>(args)...);
    }
    uint64_t to = ctx->getTimeout(timeout_so);
    std::shared_ptr<timer_info> tinfo(new timer_info);
retry:
    ssize_t n = fun(fd,std::forward<Args>(args)...);
    while(n==-1&&errno==EINTR){ // 被信号中断则重试
        n = fun(fd,std::forward<Args>(args)...);
    }
    if(n==-1&&errno==EAGAIN){ // 资源暂不可用，转入异步等待
        mo1al::IOManager* iom = mo1al::IOManager::GetThis();
        mo1al::Timer::ptr timer;
        std::weak_ptr<timer_info> winfo(tinfo);
        if(to!=(uint64_t)-1){
            // 设置了超时时间，注册超时定时器
            timer = iom->addConditionTimer(to,[winfo,iom,fd,event](){
                auto t = winfo.lock();
                if(!t||t->cancelled){ // 已被取消说明 IO 已完成
                    return;
                }
                t->cancelled = ETIMEDOUT; // 标记超时并取消对应事件
                iom->cancelEvent(fd,(mo1al::IOManager::Event)event);
            },winfo);
        }
        int rt = iom->addEvent(fd,(mo1al::IOManager::Event)event); // 注册事件等待 IO 就绪
        if(rt){
            SYLAR_LOG_ERROR(g_logger) << hook_fun_name << " addEvent("
                << fd << ", " << event << ")";
            if(timer){
                timer->cancel();
            }
            return -1;
        }
        else{
            // 事件注册成功，让出协程等待 IO 就绪或超时
            SYLAR_LOG_DEBUG(g_logger)<<"do io <"<<hook_fun_name<<">";
            mo1al::Fiber::YieldToHold();
            SYLAR_LOG_DEBUG(g_logger)<<"do io <"<<hook_fun_name<<">";
            /*
            让出 CPU 等待 IO 事件或定时器超时：
            - IO 就绪后由调度器重新调度，goto retry 重新执行 IO
            - 超时则标记 cancelled 并返回 -1
            */
            if(timer){
                timer->cancel();
            }
            if(tinfo->cancelled){
                errno = tinfo->cancelled;
                return -1;
            }
            goto retry; // IO 就绪，重试
        }
    }
    return n; // 正常返回
}

extern "C"
{
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX
unsigned int sleep(unsigned int seconds)
{
    if(!mo1al::is_hook_enable){
        return sleep_f(seconds);
    }
    mo1al::IOManager* iom = mo1al::IOManager::GetThis();
    mo1al::Fiber::ptr fiber = mo1al::Fiber::GetThis();
    // 定时器到期后将协程重新加入调度队列
    iom->addTimer(seconds*1000,std::bind(
        (void(mo1al::Scheduler::*)(mo1al::Fiber::ptr,int thread))&mo1al::IOManager::schedule,iom,fiber,-1)
    );
    // 定时器仅负责计时，恢复执行由调度器完成
    mo1al::Fiber::YieldToHold();
    return 0;
}
int usleep(useconds_t usec)
{
    if(!mo1al::is_hook_enable){
        return usleep_f(usec);
    }
    mo1al::IOManager* iom = mo1al::IOManager::GetThis();
    mo1al::Fiber::ptr fiber = mo1al::Fiber::GetThis();
    iom->addTimer(usec/1000,std::bind((void(mo1al::Scheduler::*)(mo1al::Fiber::ptr,int thread))
        &mo1al::IOManager::schedule,iom,fiber,-1)
    );
    mo1al::Fiber::YieldToHold();
    return 0;
}
int nanosleep(const struct timespec *req, struct timespec *rem)
{
    if(!mo1al::is_hook_enable){
        return nanosleep_f(req,rem);
    }
    int time_out_ms = req->tv_sec*1000+req->tv_nsec/1000/1000;
    mo1al::IOManager* iom = mo1al::IOManager::GetThis();
    mo1al::Fiber::ptr fiber = mo1al::Fiber::GetThis();
    iom->addTimer(time_out_ms,std::bind((void(mo1al::Scheduler::*)(mo1al::Fiber::ptr,int thread))
        &mo1al::IOManager::schedule,iom,fiber,-1)
        );
    mo1al::Fiber::YieldToHold();
    return 0;
}

// socket
int socket(int domain,int type,int protocol)
{
    if(!mo1al::t_hook_enable){
        return socket_f(domain,type,protocol);
    }
    int fd = socket_f(domain,type,protocol);
    if(fd==-1)
    {
        return fd;
    }
    mo1al::FdMgr::GetInstance()->get(fd,true);
    return fd;
}
// 带超时的 connect 实现
int connect_with_timeout(int fd,const struct sockaddr*addr,socklen_t addrlen,uint64_t timeout_ms)
{
    if(!mo1al::t_hook_enable){
        return connect_f(fd,addr,addrlen);
    }
    mo1al::FdCtx::ptr ctx = mo1al::FdMgr::GetInstance()->get(fd); // 不自动创建，找不到即按原逻辑处理
    if(!ctx||ctx->isClose()){
        errno = EBADF;
        return -1;
    }
    if(!ctx->isSocket()){
        return connect_f(fd,addr,addrlen);
    }
    // 用户设置了非阻塞，直接调用原始函数
    if(ctx->getUserNonblock()){
        return connect_f(fd,addr,addrlen);
    }
    int n = connect_f(fd,addr,addrlen);  // 非阻塞 connect 返回 -1 且 errno=EINPROGRESS 表示连接进行中
    if(n==0){ // 连接成功
        return 0;
    }else if(n!=-1||errno!=EINPROGRESS){
        return n;
    }
    // 连接进行中，转入异步等待
    mo1al::IOManager* iom = mo1al::IOManager::GetThis();
    mo1al::Timer::ptr timer;
    std::shared_ptr<timer_info>tinfo(new timer_info);
    std::weak_ptr<timer_info>winfo(tinfo);

    if(timeout_ms!=(uint64_t)-1){
        timer = iom->addConditionTimer(timeout_ms,[winfo,iom,fd](){
            auto t = winfo.lock();
            if(!t||t->cancelled){
                return ;
            }
            t->cancelled = ETIMEDOUT;
            iom->cancelEvent(fd,mo1al::IOManager::WRITE);
        },winfo);
    }
    int rt = iom->addEvent(fd,mo1al::IOManager::WRITE);
    if(rt==0){
        mo1al::Fiber::YieldToHold();
        if(timer){
            timer->cancel();
        }
        if(tinfo->cancelled){
            errno = tinfo->cancelled;
            return -1;
        }
    }
    else{
        if(timer){
            timer->cancel();
        }
        SYLAR_LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";
    }
    int error = 0;
    socklen_t len = sizeof(int);
    if(-1==getsockopt(fd,SOL_SOCKET,SO_ERROR,&error,&len)){
        return -1;
    }
    if(!error){ // 连接成功
        return 0;
    }
    else{
        errno = error;
        return -1;
    }
}

int connect(int sockfd,const struct sockaddr*addr,socklen_t addrlen)
{
    return connect_with_timeout(sockfd, addr, addrlen, mo1al::s_connect_timeout);
}
int accept(int s,struct sockaddr*addr,socklen_t*addrlen)
{
    int fd = do_io(s,accept_f,"accept",mo1al::IOManager::READ,SO_RCVTIMEO,addr,addrlen);
    if(fd>=0){
        // 为新连接创建 FdCtx 以便统一管理
        mo1al::FdMgr::GetInstance()->get(fd,true);
    }
    return fd;
}
// read
ssize_t read(int fd,void*buf,size_t count)
{
    return do_io(fd,read_f,"read",mo1al::IOManager::READ,SO_RCVTIMEO,buf,count);
}
ssize_t readv(int fd,const struct iovec*iov,int iovcnt)
{
    return do_io(fd,readv_f,"readv",mo1al::IOManager::READ,SO_RCVTIMEO,iov,iovcnt);
}
ssize_t recv(int sockfd,void*buf,size_t len,int flags)
{
    return do_io(sockfd,recv_f,"recv",mo1al::IOManager::READ,SO_RCVTIMEO,buf,len,flags);
}
ssize_t recvfrom(int sockfd,void*buf,size_t len,int flags,struct sockaddr*src_addr,socklen_t*addrlen)
{
    return do_io(sockfd,recvfrom_f,"recvfrom",mo1al::IOManager::READ,SO_RCVTIMEO,buf,len,flags,src_addr,addrlen);
}
ssize_t recvmsg(int sockfd,struct msghdr*msg,int flags)
{
    return do_io(sockfd,recvmsg_f,"recvmsg",mo1al::IOManager::READ,SO_RCVTIMEO,msg,flags);
}
// write
ssize_t write(int fd,const void*buf,size_t count)
{
    return do_io(fd,write_f,"write",mo1al::IOManager::WRITE,SO_RCVTIMEO,buf,count);
}
ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, writev_f, "writev", mo1al::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}
ssize_t send(int s, const void *msg, size_t len, int flags) {
    return do_io(s, send_f, "send", mo1al::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags);
}
ssize_t sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen) {
    return do_io(s, sendto_f, "sendto", mo1al::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
}
ssize_t sendmsg(int s, const struct msghdr *msg, int flags) {
    return do_io(s, sendmsg_f, "sendmsg", mo1al::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}

int close(int fd)
{
    if(!mo1al::t_hook_enable){
        return close_f(fd);
    }
    mo1al::FdCtx::ptr ctx = mo1al::FdMgr::GetInstance()->get(fd);
    if(ctx){
        mo1al::IOManager* iom = mo1al::IOManager::GetThis();
        if(iom){
            iom->cancelAll(fd);
        }
        mo1al::FdMgr::GetInstance()->del(fd);
    }
    return close_f(fd);
}

// 统一管理非阻塞标志：底层始终非阻塞，向用户屏蔽差异
int fcntl(int fd,int cmd,...)
{
    va_list va;
    va_start(va,cmd);
    switch (cmd)
    {
        // int 参数
        case F_SETFL:
        {
            int arg = va_arg(va,int);
            va_end(va);
            mo1al::FdCtx::ptr ctx = mo1al::FdMgr::GetInstance()->get(fd);
            if(!ctx||ctx->isClose()||!ctx->isSocket()){
                return fcntl_f(fd,cmd,arg);
            }
            ctx->setUserNonblock(arg&O_NONBLOCK);
            // 底层始终非阻塞，仅记录用户视角的非阻塞标志
            if(ctx->getSysNonblock()){
                arg |=  O_NONBLOCK;
            }
            else{
                arg &= ~O_NONBLOCK;
            }
            return fcntl_f(fd,cmd,arg);
        }
                break;
        case F_GETFL:
            {
                va_end(va);
                int arg = fcntl_f(fd,cmd);
                mo1al::FdCtx::ptr ctx = mo1al::FdMgr::GetInstance()->get(fd);
                if(!ctx||ctx->isClose()||!ctx->isSocket()){
                    return arg;
                }
                if(ctx->getUserNonblock()){
                    return arg|O_NONBLOCK;
                }
                else{
                    return arg&~O_NONBLOCK;
                }
            }
                break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
#ifdef  F_SETPIPE_SZ
        case F_SETPIPE_SZ:
#endif
        {
            int arg = va_arg(va,int);
            va_end(va);
            return fcntl_f(fd,cmd,arg);
        }
        break;
        // void 参数
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
#ifdef  F_GETPIPE_SZ
        case F_GETPIPE_SZ:
#endif
        {
            va_end(va);
            return fcntl_f(fd,cmd);
        }
        // struct flock* 参数
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
            {
                struct flock* arg = va_arg(va,struct flock*);
                va_end(va);
                return fcntl_f(fd,cmd,arg);
            }
            break;
        // struct f_owner_exlock* 参数
        case F_GETOWN_EX:
        case F_SETOWN_EX:
            {
                struct f_owner_exlock* arg = va_arg(va,struct f_owner_exlock*);
                va_end(va);
                return fcntl_f(fd,cmd,arg);
            }
            break;
        default:
            va_end(va);
            return fcntl_f(fd,cmd);
    }
}

// ioctl：FIONBIO 时记录用户非阻塞标志
int ioctl(int d,unsigned long int request,...)
{
    va_list va;
    va_start(va,request);
    void* arg = va_arg(va,void*);
    va_end(va);

    if(FIONBIO==request)
    {
        bool user_nonblock = !!*(int*)arg;
        mo1al::FdCtx::ptr ctx = mo1al::FdMgr::GetInstance()->get(d);
        if(!ctx||ctx->isClose()||!ctx->isSocket())
        {
            return ioctl_f(d,request,arg);
        }
        ctx->setUserNonblock(user_nonblock);
    }
    return ioctl_f(d,request,arg);
}

int getsockopt(int sockfd,int level,int optname,void* optval,socklen_t*optlen)
{
    return getsockopt_f(sockfd,level,optname,optval,optlen);
}

// setsockopt：记录 SO_RCVTIMEO/SO_SNDTIMEO 超时配置
int setsockopt(int sockfd,int level,int optname,const void*optval,socklen_t optlen)
{
    if(!mo1al::t_hook_enable){
        return setsockopt_f(sockfd,level,optname,optval,optlen);
    }
    if(level==SOL_SOCKET){
        if(optname==SO_RCVTIMEO||optname==SO_SNDTIMEO){
            mo1al::FdCtx::ptr ctx = mo1al::FdMgr::GetInstance()->get(sockfd);
            if(ctx){
                const timeval* v = (const timeval*)optval;
                ctx->setTimeout(optname,v->tv_sec*1000+v->tv_usec/1000);
            }
        }
    }
    return setsockopt_f(sockfd,level,optname,optval,optlen);
}
}