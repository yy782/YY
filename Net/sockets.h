#ifndef _YY_NET_SOCKETS_H_
#define _YY_NET_SOCKETS_H_

#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <assert.h>
#include <sys/timerfd.h>
#include "../Common/Errno.h"
#include "../Common/Types.h"
namespace yy
{
namespace  net
{
class Address;
namespace sockets
{
   
int create_tcpsocket(sa_family_t family);
int create_epollfd(int flags);
int create_eventfd(size_t count,int flags);
int create_timerfd(clockid_t clock_id,int flags);
int set_signalfd(int fd,sigset_t* sigset,int flags);

void timerfd_settime(int fd,int flags,const struct timespec* interval,const struct timespec* value);
void timerfd_settime(int fd,int flags,const struct itimerspec& new_ts);
void bind(int fd,const Address& addr);
void listen(int fd,int queue_size=SOMAXCONN);
void connect(int fd,const Address& addr);
int accept(int fd,Address& address,bool is_ipv6);

// @brief 设置端口重用
void reuse_addr(int fd,bool on=true);

// @brief 设置端口复用
void reuse_port(int fd,bool on=true);
void set_nonblock(int fd);
void set_CloseOnExec(int fd);
ssize_t read(int fd,void* buf,size_t len);
ssize_t write(int fd,const void* buf,size_t len);
ssize_t recv(int fd,void* buf,size_t len,int flags);
int sockfd_has_error(int fd);
 
}
}    
}

#endif