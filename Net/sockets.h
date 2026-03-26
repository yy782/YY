#ifndef _YY_NET_SOCKETS_H_
#define _YY_NET_SOCKETS_H_

#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <assert.h>
#include <sys/timerfd.h>

#include "../Common/Types.h"
#include "Timer.h"
#include "../Common/LogFilter.h"
#include "InetAddress.h"
namespace yy
{
namespace  net
{


namespace sockets
{
   
int createTcpSocketOrDie(sa_family_t family); // 这些是服务端启动的步骤，要求不能出错
int createUdpSocketOrDie(sa_family_t family);
int createEpollFdOrDie(int flags);
int createEventFdOrDie(size_t count,int flags);
int createTimerFdOrDie(clockid_t clock_id,int flags);
int setSignalOrDie(int fd,sigset_t* sigset,int flags);
void bindOrDie(int fd,const Address& addr);
void listenOrDie(int fd,int queue_size=SOMAXCONN);
bool isSelfConnect(int sockfd);
int timerfd_settime(int fd,int flags,const struct timespec* interval,const struct timespec* value);
int timerfd_settime(int fd,int flags,const struct itimerspec& new_ts);
template<typename T>
int timerfd_settime(int fd,int flags,const Timer<T>& timer);

int connect(int fd,const Address& addr);

template<bool isIpv6=false>
int acceptAutoOrDie(int fd,Address& address);
void setKeepAlive(int fd,bool on,int idleSeconds=7200, 
                  int intervalSeconds=75,int maxProbes=9);
void setTcpNoDelay(int fd,bool on);//控制TCP的Nagle算法开关
void setSocketBufferSize(int fd,int recvBufSize,int sendBufSize);
// @brief 设置端口重用
void reuse_addr(int fd,bool on=true);
bool setNonBlocking(int fd);
// @brief 设置端口复用
void reuse_port(int fd,bool on=true);
//void set_nonblock(int fd);
void set_CloseOnExec(int fd); // 这些报错极为罕见，选择不检查
ssize_t read(int fd,void* buf,size_t len);
ssize_t readv(int fd, const iovec* iovec, int count);
ssize_t write(int fd,const void* buf,size_t len); //SIGPIPE信号

ssize_t recv(int fd,void* buf,size_t len,int flags);
ssize_t send(int fd,const void* buf,size_t len,int flags);
ssize_t recvfrom(int fd,void* buf,size_t len,int flags,struct sockaddr_storage& peerAddr);
ssize_t sendto(int fd,const void* buf,size_t len,int flags,const Address& address);
int sockfd_has_error(int fd);
void OnlyIpv6(int fd,bool ipv6_only=true);
void close(int fd);
void shutdown(int fd,int how);
void daemonize();

}
}    
}
template<bool isIpv6>
int yy::net::sockets::acceptAutoOrDie(int fd,Address& address)
{
    int connfd=-1;
    if constexpr(isIpv6)
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
        
        switch (errno)
        {
        case EINTR://被信号中断              
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
            LOG_ERRNO(errno);
            LOG_SYSFATAL("unexpected error of ::accept ");
            break;
        default:
            LOG_ERRNO(errno);
            LOG_SYSFATAL("unexpected error of ::accept ");
            break;
        }
    }
    // @brief 这里选择沿用muduo的设计，将错误控制在可判断的风险里 
    //      muduo库没有选择在信号中断时重新accept，有一部分原因是选择了epoll的水平触发模式
    //      accept的errno处理如此谨慎，一部分原因是accept是直接关联服务端的，与连接的所有客户有关  
      
    return connfd;
}
template<typename T>
int yy::net::sockets::timerfd_settime(int fd, int flags, const Timer<T>& timer)
{
    // 先确认单位
    auto duration = timer.getTimeInterval().getTimePeriod();
    
    // 转换为纳秒
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    
    struct itimerspec new_ts{};
    new_ts.it_value.tv_sec = ns / 1000000000;
    new_ts.it_value.tv_nsec = ns % 1000000000;
    
  
    
    return sockets::timerfd_settime(fd, flags, new_ts);
}

#endif