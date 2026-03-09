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
   
int create_tcpsocket(sa_family_t family);
int create_udpsocket(sa_family_t family);

int create_epollfd(int flags);
int create_eventfd(size_t count,int flags);
int create_timerfd(clockid_t clock_id,int flags);
int set_signalfd(int fd,sigset_t* sigset,int flags);

void timerfd_settime(int fd,int flags,const struct timespec* interval,const struct timespec* value);
void timerfd_settime(int fd,int flags,const struct itimerspec& new_ts);
template<typename T>
void timerfd_settime(int fd,int flags,const Timer<T>& timer);
void bind(int fd,const Address& addr);
void listen(int fd,int queue_size=SOMAXCONN);
void connect(int fd,const Address& addr);
int accept(int fd,Address& address,bool is_ipv6);
void setKeepAlive(int fd,bool on,int idleSeconds=7200, 
                  int intervalSeconds=75,int maxProbes=9);
// @brief 设置端口重用
void reuse_addr(int fd,bool on=true);

// @brief 设置端口复用
void reuse_port(int fd,bool on=true);
void set_nonblock(int fd);
void set_CloseOnExec(int fd);
ssize_t readAuto(int fd,void* buf,size_t len);
ssize_t writeAuto(int fd,const void* buf,size_t len);
ssize_t recvAuto(int fd,void* buf,size_t len,int flags);
ssize_t sendAuto(int fd,const void* buf,size_t len,int flags);
ssize_t recvfromAuto(int fd,void* buf,size_t len,int flags,struct sockaddr_storage& peerAddr);
ssize_t sendtoAuto(int fd,const void* buf,size_t len,int flags,const Address& address);
//ssize_t sendtoET(int fd,const void* buf,size_t len,int flags,const Address& address);
//ssize_t readET(int fd,void* buf,size_t len);
//ssize_t writeET(int fd,const void* buf,size_t len);
//ssize_t recvET(int fd,void* buf,size_t len,int flags);
//ssize_t sendET(int fd,const void* buf,size_t len,int flags);
//ssize_t recvfromET(int fd,void* buf,size_t len,int flags,struct sockaddr_storage& peerAddr);
int sockfd_has_error(int fd);
void OnlyIpv6(int fd,bool ipv6_only=true);
void close(int fd);
void shutdown(int fd,int how);
void daemonize();

}
}    
}

template<typename T>
void yy::net::sockets::timerfd_settime(int fd,int flags,const Timer<T>& timer)
{
    auto diff_us=timer->getTimeInterval().getTimePeriod().count();
    struct itimerspec new_ts{};
    new_ts.it_value.tv_sec=diff_us/1000000;            
    new_ts.it_value.tv_nsec=(diff_us%1000000)*1000; 
    sockets::timerfd_settime(fd,0,new_ts); 
}

#endif