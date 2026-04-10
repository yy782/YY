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

/**
 * @file sockets.h
 * @brief 套接字操作函数的定义
 * 
 * 本文件定义了一系列套接字操作函数，包括创建套接字、绑定、监听、连接、读写等操作。
 */

namespace sockets
{
   
/**
 * @brief 创建TCP套接字
 * 
 * 这些是服务端启动的步骤，要求不能出错
 * 
 * @param family 地址族
 * @return int 套接字文件描述符
 */
int createTcpSocketOrDie(sa_family_t family);

/**
 * @brief 创建UDP套接字
 * 
 * @param family 地址族
 * @return int 套接字文件描述符
 */
int createUdpSocketOrDie(sa_family_t family);

/**
 * @brief 创建epoll文件描述符
 * 
 * @param flags 标志
 * @return int epoll文件描述符
 */
int createEpollFdOrDie(int flags);

/**
 * @brief 创建eventfd文件描述符
 * 
 * @param count 计数
 * @param flags 标志
 * @return int eventfd文件描述符
 */
int createEventFdOrDie(size_t count,int flags);

/**
 * @brief 创建timerfd文件描述符
 * 
 * @param clock_id 时钟ID
 * @param flags 标志
 * @return int timerfd文件描述符
 */
int createTimerFdOrDie(clockid_t clock_id,int flags);

/**
 * @brief 设置信号
 * 
 * @param fd 文件描述符
 * @param sigset 信号集
 * @param flags 标志
 * @return int 结果
 */
int setSignalOrDie(int fd,sigset_t* sigset,int flags);

/**
 * @brief 绑定地址
 * 
 * @param fd 文件描述符
 * @param addr 地址
 */
void bindOrDie(int fd,const Address& addr);

/**
 * @brief 监听连接
 * 
 * @param fd 文件描述符
 * @param queue_size 队列大小
 */
void listenOrDie(int fd,int queue_size=SOMAXCONN);

/**
 * @brief 检查是否是自连接
 * 
 * @param sockfd 套接字文件描述符
 * @return bool 是否是自连接
 */
bool isSelfConnect(int sockfd);

/**
 * @brief 设置timerfd的时间
 * 
 * @param fd 文件描述符
 * @param flags 标志
 * @param interval 时间间隔
 * @param value 值
 * @return int 结果
 */
int timerfd_settime(int fd,int flags,const struct timespec* interval,const struct timespec* value);

/**
 * @brief 设置timerfd的时间
 * 
 * @param fd 文件描述符
 * @param flags 标志
 * @param new_ts 新的时间
 * @return int 结果
 */
int timerfd_settime(int fd,int flags,const struct itimerspec& new_ts);

/**
 * @brief 设置timerfd的时间
 * 
 * @tparam T 定时器类型
 * @param fd 文件描述符
 * @param flags 标志
 * @param timer 定时器
 * @return int 结果
 */
template<typename T>
int timerfd_settime(int fd,int flags,const Timer<T>& timer);

/**
 * @brief 连接到服务器
 * 
 * @param fd 文件描述符
 * @param addr 地址
 * @return int 结果
 */
int connect(int fd,const Address& addr);

/**
 * @brief 接受连接
 * 
 * @tparam isIpv6 是否是IPv6
 * @param fd 文件描述符
 * @param address 地址
 * @return int 连接文件描述符
 */
template<bool isIpv6>
int acceptAutoOrDie(int fd,Address& address);

/**
 * @brief 设置TCP保活
 * 
 * @param fd 文件描述符
 * @param on 是否启用
 * @param idleSeconds 空闲时间
 * @param intervalSeconds 间隔时间
 * @param maxProbes 最大探测次数
 */
void setKeepAlive(int fd,bool on,int idleSeconds=7200, 
                  int intervalSeconds=75,int maxProbes=9);

/**
 * @brief 设置TCP_NODELAY
 * 
 * 控制TCP的Nagle算法开关
 * 
 * @param fd 文件描述符
 * @param on 是否启用
 */
void setTcpNoDelay(int fd,bool on);

/**
 * @brief 设置套接字缓冲区大小
 * 
 * @param fd 文件描述符
 * @param BufSize 缓冲区大小
 * @return bool 是否成功
 */
bool setSocketBufferSize(int fd,size_t BufSize);

/**
 * @brief 设置端口重用
 * 
 * @param fd 文件描述符
 * @param on 是否启用
 */
void reuseAddrOrDie(int fd,bool on=true);

bool isReleasePort(unsigned short usPort);

bool getReleasePort(short &port);

/**
 * @brief 设置非阻塞
 * 
 * @param fd 文件描述符
 * @return bool 是否成功
 */
bool setNonBlocking(int fd);

/**
 * @brief 设置端口复用
 * 
 * @param fd 文件描述符
 * @param on 是否启用
 */
bool isNonBlocking(int fd);
void reusePortOrDie(int fd,bool on=true);

//void set_nonblock(int fd);

/**
 * @brief 设置CloseOnExec
 * 
 * 这些报错极为罕见，选择不检查
 * 
 * @param fd 文件描述符
 */
void set_CloseOnExec(int fd);

/**
 * @brief 读取数据
 * 
 * @param fd 文件描述符
 * @param buf 缓冲区
 * @param len 长度
 * @return ssize_t 读取的字节数
 */
ssize_t read(int fd,void* buf,size_t len);

/**
 * @brief 读取数据（分散读）
 * 
 * @param fd 文件描述符
 * @param iovec iov数组
 * @param count 数组长度
 * @return ssize_t 读取的字节数
 */
ssize_t readv(int fd, const iovec* iovec, int count);

/**
 * @brief 写入数据
 * 
 * SIGPIPE信号
 * 
 * @param fd 文件描述符
 * @param buf 缓冲区
 * @param len 长度
 * @return ssize_t 写入的字节数
 */
ssize_t write(int fd,const void* buf,size_t len);

/**
 * @brief 接收数据
 * 
 * @param fd 文件描述符
 * @param buf 缓冲区
 * @param len 长度
 * @param flags 标志
 * @return ssize_t 接收的字节数
 */
ssize_t recv(int fd,void* buf,size_t len,int flags);

/**
 * @brief 发送数据
 * 
 * @param fd 文件描述符
 * @param buf 缓冲区
 * @param len 长度
 * @param flags 标志
 * @return ssize_t 发送的字节数
 */
ssize_t send(int fd,const void* buf,size_t len,int flags);

/**
 * @brief 接收数据（带地址）
 * 
 * @param fd 文件描述符
 * @param buf 缓冲区
 * @param len 长度
 * @param flags 标志
 * @param peerAddr 对端地址
 * @return ssize_t 接收的字节数
 */
ssize_t recvfrom(int fd,void* buf,size_t len,int flags,struct sockaddr_storage& peerAddr);

/**
 * @brief 发送数据（带地址）
 * 
 * @param fd 文件描述符
 * @param buf 缓冲区
 * @param len 长度
 * @param flags 标志
 * @param address 地址
 * @return ssize_t 发送的字节数
 */
ssize_t sendto(int fd,const void* buf,size_t len,int flags,const Address& address);

/**
 * @brief 检查套接字是否有错误
 * 
 * @param fd 文件描述符
 * @return int 错误码
 */
int sockfd_has_error(int fd);

/**
 * @brief 设置只使用IPv6
 * 
 * @param fd 文件描述符
 * @param ipv6_only 是否只使用IPv6
 */
void OnlyIpv6OrDie(int fd,bool ipv6_only=true);

/**
 * @brief 关闭文件描述符
 * 
 * @param fd 文件描述符
 */
void close(int fd);

/**
 * @brief 关闭连接
 * 
 * @param fd 文件描述符
 * @param how 关闭方式
 */
void shutdown(int fd,int how);

/**
 * @brief 守护进程化
 */
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
        connfd=::accept(fd,address.sockAddr(),&addrlen);        
    }
    else
    {
        struct sockaddr_in addr4_;
        memZero(&addr4_,sizeof addr4_);
        address.set_ipv4_addr(addr4_);
        socklen_t addrlen = static_cast<socklen_t>(sizeof(addr4_));
        connfd=::accept(fd,address.sockAddr(),&addrlen);    
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