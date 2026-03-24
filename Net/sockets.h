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
namespace yy
{
namespace  net
{
class Address;

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
int acceptAutoOrDie(int fd,Address& address,bool is_ipv6);
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