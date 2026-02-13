#ifndef _YY_NET_SERVER_
#define _YY_NET_SERVER_


#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#include "../Common/noncopyable.h"
#include "../Common/Log.h"
#include "../Common/Types.h"
#include "../Common/Errno.h"
#include "Socket.h"
#include "InetAddress.h"

namespace yy
{
namespace net
{  
namespace server
{
    Socket create_tcpsocket()
    {
        auto listenfd=socket(AF_INET,SOCK_STREAM,0);
        if(listenfd<0)
        {
            LOG_PRINT_ERRNO;
        }
        return Socket(listenfd);
    }
    void bind(const Socket& k,const Address& addr)
    {
        if(::bind(k.get_fd(),addr.getSockAddr(),addr.get_len())==-1)
        {
            LOG_PRINT_ERRNO;
        }
    // @brief 这里bind传入的长度是sockaddr_in6的长度，也就是最大的长度，兼容ipv4和ipv6,因为bind的第三个参数要求接收多少字节数据，但是解释多少数据
    //          由传入addr的family属性决定
    }
    void listen(const Socket& k,size_t queue_size=SOMAXCONN)
    {
        auto ret=::listen(k.get_fd(),queue_size);
        if(ret<0)
        {
            LOG_PRINT_ERRNO;
        }
    }
    Socket accept(Socket& k,Address& addr)
    {
        struct sockaddr_in6 addr6_;
        memZero(&addr6_,sizeof addr6_);
        socklen_t addrlen = static_cast<socklen_t>(sizeof(addr6_));
        int connfd=::accept(k.get_fd(),safe_static_cast<sockaddr*>(&addr6_),&addrlen);
        // @brief muduo库选择accept4或accept来实现更好的兼容，我这里选择accept来向下兼容
        if(connfd==-1)
        {
            int savedErrno = errno;
            LOG_PRINT_ERRNO;
            switch (savedErrno)
            {
            case EAGAIN://连接队列为空
            case ECONNABORTED://连接被客户端终止
            case EINTR://被信号中断
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
                LOG_NULL_WARN("unexpected error of ::accept ");
                break;
            default:
                LOG_NULL_WARN("unknown error of ::accept ");
                break;
            }
        }
        // @brief 这里选择沿用muduo的设计，将错误控制在可判断的风险里 
        //      muduo库没有选择在信号中断时重新accept，有一部分原因是选择了epoll的水平触发模式
        //      accept的errno处理如此谨慎，一部分原因是accept是直接关联服务端的，与连接的所有客户有关    
        return Socket(connfd);
    }

    // @brief 设置端口重用
    void reuse_addr(Socket& k,bool on=true)
    {
        int optval=on?1:0;
        auto ret=::setsockopt(k.get_fd(), SOL_SOCKET, SO_REUSEADDR,
               &optval, static_cast<socklen_t>(sizeof optval));
        if(ret<0)
        {
            LOG_PRINT_ERRNO;
        }
    }

    // @brief 设置端口复用
    void reuse_port(Socket& k,bool on=true)
    {
        int optval = on ? 1 : 0;
        int ret = ::setsockopt(k.get_fd(), SOL_SOCKET, SO_REUSEPORT,
                                &optval, static_cast<socklen_t>(sizeof optval)); 
        if(ret<0)
        {
            LOG_PRINT_ERRNO;
        }                               
    }
}
}    
}
#endif