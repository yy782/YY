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
ssize_t sendtoET(int fd,const void* buf,size_t len,int flags,const Address& address);
ssize_t readET(int fd,void* buf,size_t len);
ssize_t writeET(int fd,const void* buf,size_t len);
ssize_t recvET(int fd,void* buf,size_t len,int flags);
ssize_t sendET(int fd,const void* buf,size_t len,int flags);
ssize_t recvfromET(int fd,void* buf,size_t len,int flags,struct sockaddr_storage& peerAddr);
ssize_t sendto(int fd,const void* buf,size_t len,int flags,const Address& address);
ssize_t read(int fd,void* buf,size_t len);
ssize_t write(int fd,const void* buf,size_t len);
ssize_t recv(int fd,void* buf,size_t len,int flags);
ssize_t send(int fd,const void* buf,size_t len,int flags);
ssize_t recvfrom(int fd,void* buf,size_t len,int flags,struct sockaddr_storage& peerAddr);


int createTcpSocketOrDie(sa_family_t family)
{
    auto listenfd=::socket(family,SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if(listenfd<0)
    {
        LOG_SYSFATAL("sockets::createNonblockingOrDie");
    }
    return listenfd;
}
int createUdpSocketOrDie(sa_family_t family)
{
    int fd=::socket(family,SOCK_DGRAM| SOCK_NONBLOCK | SOCK_CLOEXEC,IPPROTO_UDP);
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
    int ret=::timerfd_settime(fd,flags,&new_ts,NULL); 
    if(ret<0)
    {
        LOG_ERRNO(errno);
    }  
    return ret;  
}

void bindOrDie(int fd,const Address& addr)
{
    if(::bind(fd,addr.getSockAddr(),addr.get_len())==-1)
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
    auto ret=::connect(fd,addr.getSockAddr(),addr.get_len());
    if(ret==-1)
    {
        LOG_ERRNO(errno);
    }
    return ret;
}
int acceptAutoOrDie(int fd,Address& address,bool is_ipv6)
{
    int connfd=-1;
    if(is_ipv6)
    {
        struct sockaddr_in6 addr6_;
        memZero(&addr6_,sizeof addr6_);
        address.set_ipv6_addr(addr6_);
        socklen_t addrlen = static_cast<socklen_t>(sizeof(addr6_));
        connfd=::accept(fd,address.getSockAddr(),&addrlen);        
    }
    else
    {
        struct sockaddr_in addr4_;
        memZero(&addr4_,sizeof addr4_);
        address.set_ipv4_addr(addr4_);
        socklen_t addrlen = static_cast<socklen_t>(sizeof(addr4_));
        connfd=::accept(fd,address.getSockAddr(),&addrlen);
    }

    
    // @brief muduo库选择accept4或accept来实现更好的兼容，我这里选择accept来向下兼容
    if(connfd==-1)
    {
        LOG_ERRNO(errno);
        switch (errno)
        {
        case EINTR://被信号中断  
            if constexpr (std::is_same_v<PollerType,Epoll>)
                return acceptAutoOrDie(fd,address,is_ipv6);            
            
        case EAGAIN://连接队列为空
        case ECONNABORTED://连接被客户端终止
        case EPROTO://协议错误
        case EPERM://权限错误
        case EMFILE: //进程打开的FD达到上限
            break;
        case EBADF://非法的文件描述符
        case EFAULT://地址空间非法
        case EINVAL://参数无效
        case ENFILE://操作系统的FD总数用完了
        case ENOBUFS://
        case ENOMEM://内存不足
        case ENOTSOCK://FD不是套接字
        case EOPNOTSUPP://操作不被支持
            LOG_SYSFATAL("unexpected error of ::accept ");
            break;
        default:
            LOG_SYSFATAL("unexpected error of ::accept ");
            break;
        }
    }
    // @brief 这里选择沿用muduo的设计，将错误控制在可判断的风险里 
    //      muduo库没有选择在信号中断时重新accept，有一部分原因是选择了epoll的水平触发模式
    //      accept的errno处理如此谨慎，一部分原因是accept是直接关联服务端的，与连接的所有客户有关    
    return connfd;
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
void reuse_addr(int fd,bool on)
{
    // 根据on参数设置optval的值，1表示启用，0表示禁用
    int optval=on?1:0;
    auto ret=::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
            &optval, static_cast<socklen_t>(sizeof optval));
    if(ret<0)
    {
        LOG_ERRNO(errno);
    }
}
void reuse_port(int fd,bool on)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT,
                            &optval, static_cast<socklen_t>(sizeof optval)); 
    if(ret<0)
    {
        LOG_ERRNO(errno);
    }                               
}
void set_nonblock(int fd){
    int flags=fcntl(fd,F_GETFL,0);
    if(flags==-1){
        LOG_ERRNO(errno);
    }
    if(fcntl(fd,F_SETFL,flags|O_NONBLOCK)==-1){
        LOG_ERRNO(errno);
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

ssize_t readvAuto(int fd, const iovec* iovec, int count)
{
    int ret=::readv(fd,iovec,count);
    if(ret<0)
    {
        LOG_ERRNO(errno);
    }
    return ret;
}
ssize_t readAuto(int fd,void* buf,size_t len)
{
    if constexpr (std::is_same_v<PollerType,Epoll>)
    {
        return readET(fd,buf,len);
    }
    else
    {
        return read(fd,buf,len);
    }
}
ssize_t writeAuto(int fd,const void* buf,size_t len)
{
    if constexpr (std::is_same_v<PollerType,Epoll>)
    {
        return writeET(fd,buf,len);
    }
    else
    {
        return write(fd,buf,len);
    }
}
ssize_t recvAuto(int fd,void* buf,size_t len,int flags)
{
    if constexpr (std::is_same_v<PollerType,Epoll>)
    {
        return recvET(fd,buf,len,flags|MSG_DONTWAIT);
    }
    else
    {
        return recv(fd,buf,len,flags);
    }
}
ssize_t sendAuto(int fd,const void* buf,size_t len,int flags)
{
    if constexpr (std::is_same_v<PollerType,Epoll>)
    {
        return sendET(fd,buf,len,flags|MSG_DONTWAIT|MSG_NOSIGNAL|MSG_MORE);
    }
    else
    {
        return send(fd,buf,len,flags|MSG_NOSIGNAL|MSG_MORE);
    }
}
ssize_t recvfromAuto(int fd,void* buf,size_t len,int flags,struct sockaddr_storage& peerAddr)
{
    if constexpr (std::is_same_v<PollerType,Epoll>)
    {
        return recvfromET(fd,buf,len,flags,peerAddr);
    }
    else
    {
        return recvfrom(fd,buf,len,flags,peerAddr);
    }
}
ssize_t sendtoAuto(int fd,const void* buf,size_t len,int flags,const Address& address)
{
    if constexpr (std::is_same_v<PollerType,Epoll>)
    {
        return sendtoET(fd,buf,len,flags,address);
    }
    else
    {
        return sendto(fd,buf,len,flags,address.getSockAddr(),address.get_len());
    }
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
void OnlyIpv6(int fd,bool ipv6_only)
{
    int optval=ipv6_only?1:0;
    auto ret=::setsockopt(fd,SOL_IPV6,IPV6_V6ONLY,&optval,sizeof(optval));
    if(ret<0)
    {
        LOG_ERRNO(errno);
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
    auto n=::read(fd,buf,len);
    if(n<0)
    {
        LOG_ERRNO(errno);
    }
    return n;
}
ssize_t write(int fd,const void* buf,size_t len)
{
    ssize_t n=::write(fd,buf,len);
    if(n<0)
    {
        LOG_ERRNO(errno);
    }
    return n;
}
ssize_t recv(int fd,void* buf,size_t len,int flags)
{
    auto n=::recv(fd,buf,len,flags);
    if(n<0)
    {
        LOG_ERRNO(errno);
    }
    return n;
}
ssize_t send(int fd,const void* buf,size_t len,int flags)
{
    auto n=::send(fd,buf,len,flags);
    if(n<0)
    {
        LOG_ERRNO(errno);
    }
    return n;
}

/**
 * ET模式下的read函数 - recvn风格
 * 特点：内部循环，尽可能读取数据，遇到EAGAIN返回已读取字节数
 * @param fd 文件描述符
 * @param buf 缓冲区
 * @param len 期望读取的长度
 * @return >0 实际读取的字节数，0 对端关闭，-1 错误
 */
ssize_t readET(int fd, void* buf, size_t len)
{
    assert(len);
    
    char* ptr = static_cast<char*>(buf);
    size_t left = len;
    
    while (left > 0) {
        ssize_t n = ::read(fd, ptr, left);
        
        if (n == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 没有数据可读了，返回已经读取的字节数
                return len - left;
            } else if (errno == EINTR) {
                // 被信号中断，继续读取
                continue;
            } else {
                LOG_ERRNO(errno);
                // 如果已经读取了一些数据，返回已读取的字节数；否则返回-1
                return (len - left) > 0 ? (len - left) : -1;
            }
        } else if (n == 0) {
            // 对端关闭连接，返回已读取的字节数
            return len - left;
        } else {
            // 成功读取n字节
            ptr += n;
            left -= n;
            // 注意：不判断left是否为0，继续循环直到left==0或出错
        }
    }
    
    return len;  // 完全读取了len字节
}

/**
 * ET模式下的write函数 - recvn风格
 * 特点：内部循环，尽可能写入数据，遇到EAGAIN返回已写入字节数
 */
ssize_t writeET(int fd, const void* buf, size_t len)
{
    assert(len);
    
    const char* ptr = static_cast<const char*>(buf);
    size_t left = len;
    
    while (left > 0) {
        ssize_t n = ::write(fd, ptr, left);
        
        if (n == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 缓冲区满，返回已经写入的字节数
                return len - left;
            } else if (errno == EINTR) {
                // 被信号中断，继续写入
                continue;
            } else {
                LOG_ERRNO(errno);
                return (len - left) > 0 ? (len - left) : -1;
            }
        } else {
            // 成功写入n字节
            ptr += n;
            left -= n;
        }
    }
    
    return len;  // 完全写入了len字节
}

/**
 * ET模式下的recv函数 - recvn风格
 */
ssize_t recvET(int fd, void* buf, size_t len, int flags)
{
    assert(len);
    
    char* ptr = static_cast<char*>(buf);
    size_t left = len;
    while (left > 0) {
        ssize_t n = ::recv(fd, ptr, left, flags);
        
        if (n == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if(len-left!=0)
                    return len - left;
                else 
                    return -1; 
            } else if (errno == EINTR) {
                continue;
            } else {
                LOG_ERRNO(errno);
                return (len - left) > 0 ? (len - left) : -1;
            }
        } else if (n == 0) {
            // 对端关闭
            return len - left;
        } else {
            ptr += n;
            left -= n;
        }
    }
    
    return len;
}

/**
 * ET模式下的send函数 - recvn风格
 */
ssize_t sendET(int fd, const void* buf, size_t len, int flags)
{
    assert(len);
    
    const char* ptr = static_cast<const char*>(buf);
    size_t left = len;
    
    while (left > 0) {
        ssize_t n = ::send(fd, ptr, left, flags); 
        if (n == -1) 
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) 
            {
                return len - left;
            } else if (errno == EINTR) 
            {
                continue;
            } else {
                LOG_ERRNO(errno);
                return (len - left) > 0 ? (len - left) : -1;
            }
        } else {
            ptr += n;
            left -= n;
        }
    }
    
    return len;
}
ssize_t recvfrom(int fd,void* buf,size_t len,int flags,struct sockaddr_storage& peerAddr)
{
    socklen_t size=sizeof(peerAddr);
    ssize_t n=::recvfrom(fd,buf,len,flags,reinterpret_cast<struct sockaddr*>(&peerAddr),&size);
    if(n<0)
    {
        LOG_ERRNO(errno);
    }
    return n;
}
ssize_t recvfromET(int fd,void* buf,size_t len,int flags,struct sockaddr_storage& peerAddr)
{
    socklen_t size=sizeof(peerAddr);
    assert(len);
    
    char* ptr = static_cast<char*>(buf);
    size_t left = len;
    
    while (left > 0) 
    {
        ssize_t n =::recvfrom(fd,ptr,left,flags,reinterpret_cast<struct sockaddr*>(&peerAddr),&size);
        if (n == -1) 
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return len - left;
            } else if (errno == EINTR) {
                continue;
            } else {
                LOG_ERRNO(errno);
                return (len - left) > 0 ? (len - left) : -1;
            }
        } else if (n == 0) 
        {
            assert(false);
            return len-left;
        } else {
            ptr += n;
            left -= n;
        }
    }

    return len;
}
ssize_t sendto(int fd,const void* buf,size_t len,int flags,const Address& address)
{
    socklen_t address_len=address.get_len();
    assert(len);
    
    ssize_t n=::sendto(fd, buf, len, flags,address.getSockAddr(),address_len);
    if (n == -1) 
    {
        LOG_ERRNO(errno); 
    }
    return n;
}
ssize_t sendtoET(int fd,const void* buf,size_t len,int flags,const Address& address)
{
    socklen_t address_len=address.get_len();
    assert(len);
    
    const char* ptr = static_cast<const char*>(buf);
    size_t left = len;
    
    while (left > 0) {
        ssize_t n=::sendto(fd, ptr, left, flags,address.getSockAddr(),address_len);
        if (n == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) 
            {
                return len - left;
            } else if (errno == EINTR) 
            {
                continue;
            } else 
            {
                LOG_ERRNO(errno);
                return (len - left) > 0 ? (len - left) : -1;
            }
        } else {
            ptr += n;
            left -= n;
        }
    }
    
    return len;
}

}   
}    
}