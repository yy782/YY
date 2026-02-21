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
   
int create_tcpsocket();
int create_epollfd(int flags);
int create_eventfd(size_t count,int flags);
int create_timerfd(clockid_t clock_id,int flags);

void timerfd_settime(int fd,int flags,const struct timespec* interval,const struct timespec* value);
void timerfd_settime(int fd,int flags,const struct itimerspec& new_ts);
void bind(int fd,const Address& addr);
void listen(int fd,int queue_size=SOMAXCONN);
void connect(int fd,const Address& addr);
int accept(int fd,Address& address);

// @brief 设置端口重用
void reuse_addr(int fd,bool on=true)
{
    int optval=on?1:0;
    auto ret=::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
            &optval, static_cast<socklen_t>(sizeof optval));
    if(ret<0)
    {
        LOG_PRINT_ERRNO(errno);
    }
}

// @brief 设置端口复用
void reuse_port(int fd,bool on=true)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT,
                            &optval, static_cast<socklen_t>(sizeof optval)); 
    if(ret<0)
    {
        LOG_PRINT_ERRNO(errno);
    }                               
}
    
void set_nonblock(int fd){
    int flags=fcntl(fd,F_GETFL,0);
    if(flags==-1){
        LOG_PRINT_ERRNO(errno);
    }
    if(fcntl(fd,F_SETFL,flags|O_NONBLOCK)==-1){
        LOG_PRINT_ERRNO(errno);
    }
}
void set_CloseOnExec(int fd)
{
    int flags=fcntl(fd,F_GETFL,0);
    if(flags==-1){
        LOG_PRINT_ERRNO(errno);
    }
    if(fcntl(fd,F_SETFL,flags|FD_CLOEXEC)==-1){
        LOG_PRINT_ERRNO(errno);
    }
    // @brief 避免在子进程继承文件描述符，比如监听套接字，导致异常        
}
ssize_t YYread(int fd,void* buf,size_t len)
{
    auto n=::read(fd,buf,len);
    if(n<0)
    {
        LOG_PRINT_ERRNO(errno);
    }
    return n;
}
ssize_t YYwrite(int fd,const void* buf,size_t len)
{
    ssize_t n=::write(fd,buf,len);
    if(n<0)
    {
        LOG_PRINT_ERRNO(errno);
    }
    return n;
}
ssize_t YYrecv(int fd,void* buf,size_t len,int flags)
{
    auto n=::recv(fd,buf,len,flags);
    if(n<0)
    {
        LOG_PRINT_ERRNO(errno);
    }
    return n;
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
        LOG_PRINT_ERRNO(errno);
        return -1;
    }
    assert(false&&"不可能执行到这");
    return -2;
}
 
}
}    
}

#endif