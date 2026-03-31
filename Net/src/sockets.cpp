#include "sockets.h"
#include "../Common/Types.h"
#include "InetAddress.h"
#include <netinet/tcp.h>
#include <sys/signalfd.h>
#include <iostream>
#include "../Common/LogFilter.h"
#include <sys/stat.h>
#include <sys/uio.h>
#include "PollerType.h"
namespace yy
{
namespace net
{
namespace sockets
{
int createTcpSocketOrDie(sa_family_t family)
{
    auto listenfd=::socket(family,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,IPPROTO_TCP);
    if(listenfd<0)
    {
        LOG_SYSFATAL("sockets::createNonblockingOrDie");
    }
    return listenfd;
}
int createUdpSocketOrDie(sa_family_t family)
{
    int fd=::socket(family,SOCK_DGRAM|SOCK_NONBLOCK|SOCK_CLOEXEC,IPPROTO_UDP);
    if(fd<0)
    {
        LOG_SYSFATAL("sockets::createNonblockingOrDie");
    }
    return fd;
}
int createEpollFdOrDie(int flags)
{
    int epfd=::epoll_create1(flags);
    if(epfd<0)
    {
        LOG_SYSFATAL("sockets::createNonblockingOrDie");
    }
    return epfd;
}
int createEventFdOrDie(size_t count,int flags)
{
    int fd=::eventfd(static_cast<unsigned int>(count),flags);
    if(fd<0)
    {
        LOG_SYSFATAL("sockets::createNonblockingOrDie");
    }
    return fd;
}
int createTimerFdOrDie(clockid_t clock_id,int flags)
{
    int fd=::timerfd_create(clock_id,flags);
    if(fd<0)
    {
        LOG_SYSFATAL("sockets::createNonblockingOrDie");
    }
    return fd;
}
int  setSignalOrDie(int fd,sigset_t* sigset,int flags)
{

    int signfd=::signalfd(fd,sigset,flags);
    if(signfd<0)
    {
        LOG_SYSFATAL("sockets::createNonblockingOrDie");
    }
    return signfd;
}


int timerfd_settime(int fd,int flags,const struct timespec* interval,const struct timespec* value)
{
/*
    struct timespec {
        time_t tv_sec;  // 秒（seconds）
        long   tv_nsec; // 纳秒（nanoseconds），范围 0 ~ 999,999,999
    };

    struct itimerspec {
        struct timespec it_interval; // 重复间隔（周期性定时器用）
        struct timespec it_value;    // 首次超时时间（一次性/周期性定时器都需要）
    };        
*/
    struct itimerspec NewValue;
    memZero(&NewValue,sizeof NewValue);
    if(interval!=NULL)NewValue.it_interval=(*interval);
    if(value!=NULL)NewValue.it_value=(*value);
    

    return timerfd_settime(fd,flags,NewValue);
}
int timerfd_settime(int fd,int flags,const struct itimerspec& new_ts)
{ 
    return ::timerfd_settime(fd,flags,&new_ts,NULL);
}

void bindOrDie(int fd,const Address& addr)
{
    if(::bind(fd,addr.sockAddr(),addr.len())==-1)
    {
        LOG_ERRNO(errno);
        LOG_SYSFATAL("sockets::bindOrDie");
    }

// @brief 这里bind传入的长度是sockaddr_in6的长度，也就是最大的长度，兼容ipv4和ipv6,因为bind的第三个参数要求接收多少字节数据，但是解释多少数据
//          由传入addr的family属性决定
}
void listenOrDie(int fd,int queue_size)
{
    auto ret=::listen(fd,queue_size);
    if(ret<0)
    {
        LOG_ERRNO(errno);
        LOG_SYSFATAL("sockets::listenOrDie");
    }
}
int connect(int fd,const Address& addr)
{
    return ::connect(fd,addr.sockAddr(),addr.len());
}
struct sockaddr_in6 getLocalAddr(int sockfd)
{
    struct sockaddr_in6 localaddr;
    memZero(&localaddr,sizeof localaddr);
    socklen_t addrlen=static_cast<socklen_t>(sizeof localaddr);
    if (::getsockname(sockfd,reinterpret_cast<struct sockaddr*>(&localaddr),&addrlen)<0)
    {
        LOG_WARN("sockets::getLocalAddr");
    }
    return localaddr;
}

struct sockaddr_in6 getPeerAddr(int sockfd)
{
    struct sockaddr_in6 peeraddr;
    memZero(&peeraddr,sizeof peeraddr);
    socklen_t addrlen=static_cast<socklen_t>(sizeof peeraddr);
    if (::getpeername(sockfd, reinterpret_cast<struct sockaddr*>(&peeraddr),&addrlen)<0)
    {
        LOG_WARN("sockets::getPeerAddr");
    }
    return peeraddr;
}
bool isSelfConnect(int sockfd)
{
    struct sockaddr_in6 localaddr=getLocalAddr(sockfd);
    struct sockaddr_in6 peeraddr=getPeerAddr(sockfd);
    if (localaddr.sin6_family==AF_INET)
    {
        const struct sockaddr_in* laddr4=reinterpret_cast<struct sockaddr_in*>(&localaddr);
        const struct sockaddr_in* raddr4=reinterpret_cast<struct sockaddr_in*>(&peeraddr);
        return laddr4->sin_port==raddr4->sin_port
            && laddr4->sin_addr.s_addr==raddr4->sin_addr.s_addr;
    }
    else if (localaddr.sin6_family==AF_INET6)
    {
        return localaddr.sin6_port==peeraddr.sin6_port
            &&memcmp(&localaddr.sin6_addr,&peeraddr.sin6_addr,sizeof localaddr.sin6_addr) == 0;
    }
    else
    {
        return false;
    }
}
bool setNonBlocking(int fd) 
{
    int flags=fcntl(fd, F_GETFL,0);
    if (flags==-1) 
    {
        return false;
    }
    flags|=O_NONBLOCK;
    if (fcntl(fd,F_SETFL,flags) == -1) 
    {
        return false;
    } 
    return true;
}
bool isNonBlocking(int fd) 
{
    int flags=fcntl(fd,F_GETFL);
    if (flags==-1) {
        return false;
    }
    return (flags&O_NONBLOCK)!=0;
}

void setKeepAlive(int fd,bool on,int idleSeconds, 
                  int intervalSeconds,int maxProbes)
{
    // 1. 开启保活功能（必须）
    int optval=on?1:0;
    int ret=::setsockopt(fd,SOL_SOCKET,SO_KEEPALIVE,
                         &optval,sizeof(optval));
    if(ret<0) 
    {
        LOG_ERRNO(errno);
    }
    if(!on)return;
    
    ret=::setsockopt(fd,IPPROTO_TCP,TCP_KEEPIDLE,&idleSeconds,sizeof(idleSeconds));// @prief 探测开始前的等待时间
    if(ret<0) 
    {
        LOG_ERRNO(errno);
    }    
    ret=::setsockopt(fd,IPPROTO_TCP,TCP_KEEPINTVL,&intervalSeconds,sizeof(intervalSeconds));// @prief 探测间隔时间
    if(ret<0) 
    {
        LOG_ERRNO(errno);
    }
    ret=::setsockopt(fd,IPPROTO_TCP,TCP_KEEPCNT,&maxProbes,sizeof(maxProbes));// @prief 探测次数
    if(ret<0) 
    {
        LOG_ERRNO(errno);
    }
}
void setTcpNoDelay(int fd,bool on)
{
    int optval = on ? 1 : 0;
    int ret=::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
                &optval, static_cast<socklen_t>(sizeof optval));
    if(ret<0) 
    {
        LOG_ERRNO(errno);
    }  
}
bool setSocketBufferSize(int fd,size_t BufSize)
{
    assert(BufSize>0);
    int ret=::setsockopt(fd,SOL_SOCKET,SO_RCVBUF,&BufSize,static_cast<socklen_t>(sizeof BufSize));
    return ret>=0;
}
void reuseAddrOrDie (int fd,bool on)
{
    // 根据on参数设置optval的值，1表示启用，0表示禁用
    int optval=on?1:0;
    auto ret=::setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,
            &optval,static_cast<socklen_t>(sizeof optval));
    if(ret<0)
    {
        LOG_ERRNO(errno);
        LOG_SYSFATAL("sockets::reuseAddrOrDie");
    }
}
void reusePortOrDie(int fd,bool on)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(fd,SOL_SOCKET,SO_REUSEPORT,
                            &optval,static_cast<socklen_t>(sizeof optval)); 
    if(ret<0)
    {
        LOG_ERRNO(errno);
        LOG_SYSFATAL("sockets::reusePortOrDie");
    }                               
}

void set_CloseOnExec(int fd)
{
    int flags=fcntl(fd,F_GETFL,0);
    if(flags==-1){
        LOG_ERRNO(errno);
    }
    if(fcntl(fd,F_SETFL,flags|FD_CLOEXEC)==-1){
        LOG_ERRNO(errno);
    }
    // @brief 避免在子进程继承文件描述符，比如监听套接字，导致异常        
}

ssize_t readv(int fd, const iovec* iovec, int count)
{
    return ::readv(fd,iovec,count);
    
}
int sockfd_has_error(int fd)
{
    int error;
    socklen_t len=sizeof(error);
    // @note 由于error是小整数，所以从size_t到socklen_t转换是安全的，不会溢出
    auto ret=::getsockopt(fd,SOL_SOCKET,SO_ERROR,&error,&len);
    if(ret==0)
    {
        if(error!=0)
        {
            return 1;
        }
        else
        {
            return 0;
        }            
    }
    else
    {
        LOG_ERRNO(errno);
        return -1;
    }
    assert(false&&"不可能执行到这");
    return -2;
}
void OnlyIpv6OrDie(int fd,bool ipv6_only)
{
    int optval=ipv6_only?1:0;
    auto ret=::setsockopt(fd,SOL_IPV6,IPV6_V6ONLY,&optval,sizeof(optval));
    if(ret<0)
    {
        LOG_ERRNO(errno);
        LOG_SYSFATAL("sockets::OnlyIpv6OrDie");
    }
} 
void close(int fd)
{
    if(::close(fd)<0)
    {
        LOG_ERRNO(errno);
    }
}
void shutdown(int fd,int how)
{
    if(::shutdown(fd,how)<0)
    {
        LOG_ERRNO(errno);
    }
}
void daemonize(){
    pid_t pid=fork();
    if(pid==-1){
        perror("[pid] false");
        exit(1);
    }
    if(pid>0){
        exit(0);
    }
    
    if(setsid()==-1){
        exit(1);
    }
    pid=fork();//防止重新获取终端
    if(pid>0)exit(0);
    umask(0);
    //LOG_SYSTEM_INFO("[PID] "<<getpid());
    
    
    if(chdir("/")==-1){
        //LOG_SYSTEM_ERROR("[chdir] "<<"false");
        exit(1);
    }

    int null_fd=open("/dev/null",O_WRONLY);//stdcout,stdcerr重定向输出文件设备，以防cout阻塞
    if(null_fd==-1){
        //LOG_SYSTEM_ERROR("[open] "<<"false");
    }
    
    if(dup2(null_fd,STDOUT_FILENO)==-1){
        //LOG_SYSTEM_ERROR("[dup2] "<<"false");
        close(null_fd);
    }
    if(dup2(null_fd,STDERR_FILENO)==-1){
        //LOG_SYSTEM_ERROR("[dup2] "<<"false");
        close(null_fd);       
    }
}

ssize_t read(int fd,void* buf,size_t len)
{
    return ::read(fd,buf,len);
}
ssize_t write(int fd,const void* buf,size_t len)
{
    return ::write(fd,buf,len);

}
ssize_t recv(int fd,void* buf,size_t len,int flags)
{
    return ::recv(fd,buf,len,flags);
}
ssize_t send(int fd,const void* buf,size_t len,int flags)
{
    return ::send(fd,buf,len,flags);
}

ssize_t recvfrom(int fd,void* buf,size_t len,int flags,struct sockaddr_storage& peerAddr)
{
    socklen_t size=sizeof(peerAddr);
    return ::recvfrom(fd,buf,len,flags,reinterpret_cast<struct sockaddr*>(&peerAddr),&size);

}
ssize_t sendto(int fd,const void* buf,size_t len,int flags,const Address& address)
{
    socklen_t address_len=address.len();
    assert(len);
    
    return ::sendto(fd, buf, len, flags,address.sockAddr(),address_len);

}

}   
}    
}