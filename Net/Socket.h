#ifndef _YY_NET_SOCKET_
#define _YY_NET_SOCKET_

#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#include "../Common/noncopyable.h"
#include "../Common/Log.h"
#include "../Common/Errno.h"
#include "InetAddress.h"

namespace yy
{
namespace net
{    
class Socket:public noncopyable
{
public:
    Socket():
    fd_(-1)
    {}    
    explicit Socket(int fd):
    fd_(fd)
    {}

    ~Socket()
    {
        if(::close(fd_)<0)
        {
            LOG_PRINT_ERRNO;
        }
    }

    void set_nonblock(){
        int flags=fcntl(fd_,F_GETFL,0);
        if(flags==-1){
            LOG_PRINT_ERRNO;
        }
        if(fcntl(fd_,F_SETFL,flags|O_NONBLOCK)==-1){
            LOG_PRINT_ERRNO;
        }
    }
    void set_CloseOnExec()
    {
        int flags=fcntl(fd_,F_GETFL,0);
        if(flags==-1){
            LOG_PRINT_ERRNO;
        }
        if(fcntl(fd_,F_SETFL,flags|FD_CLOEXEC)==-1){
            LOG_PRINT_ERRNO;
        }
        // @brief 避免在子进程继承文件描述符，比如监听套接字，导致异常        
    }
    int get_fd()const{return fd_;}
private:
    int fd_;
};
}
}
#endif