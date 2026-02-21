#include "sockets.h"
#include "../Common/Types.h"
#include "InetAddress.h"
namespace yy
{
namespace net
{
namespace sockets
{

int create_tcpsocket()
{
    auto listenfd=socket(AF_INET,SOCK_STREAM,0);
    if(listenfd<0)
    {
        LOG_PRINT_ERRNO(errno);
    }
    return listenfd;
}
int create_epollfd(int flags)
{
    int epfd=::epoll_create1(flags);
    if(epfd<0)
    {
        LOG_PRINT_ERRNO(errno);
    }
    return epfd;
}
int create_eventfd(size_t count,int flags)
{
    int fd=::eventfd(safe_static_cast<unsigned int>(count),flags);
    if(fd<0)
    {
        LOG_PRINT_ERRNO(errno);
    }
    return fd;
}
int create_timerfd(clockid_t clock_id,int flags)
{
    int fd=::timerfd_create(clock_id,flags);
    if(fd<0)
    {
        LOG_PRINT_ERRNO(errno);
    }
    return fd;
}

void timerfd_settime(int fd,int flags,const struct timespec* interval,const struct timespec* value)
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
    

    timerfd_settime(fd,flags,NewValue);
}
void timerfd_settime(int fd,int flags,const struct itimerspec& new_ts)
{
    int ret=::timerfd_settime(fd,flags,&new_ts,NULL); 
    if(ret<0)
    {
        LOG_PRINT_ERRNO(errno);
    }    
}

void YYbind(int fd,const Address& addr)
{
    if(::bind(fd,addr.getSockAddr(),addr.get_len())==-1)
    {
        LOG_PRINT_ERRNO(errno);
    }
// @brief 这里bind传入的长度是sockaddr_in6的长度，也就是最大的长度，兼容ipv4和ipv6,因为bind的第三个参数要求接收多少字节数据，但是解释多少数据
//          由传入addr的family属性决定
}
void YYlisten(int fd,int queue_size)
{
    auto ret=::listen(fd,queue_size);
    if(ret<0)
    {
        LOG_PRINT_ERRNO(errno);
    }
}
void YYconnect(int fd,const Address& addr)
{
    auto ret=::connect(fd,addr.getSockAddr(),addr.get_len());
    if(ret==-1)
    {
        LOG_PRINT_ERRNO(errno);
    }
}
int YYaccept(int fd,Address& address)
{
    struct sockaddr_in6 addr6_;
    memZero(&addr6_,sizeof addr6_);
    address.set_ipv6_addr(addr6_);
    socklen_t addrlen = static_cast<socklen_t>(sizeof(addr6_));
    int connfd=::accept(fd,address.getSockAddr(),&addrlen);
    
    // @brief muduo库选择accept4或accept来实现更好的兼容，我这里选择accept来向下兼容
    if(connfd==-1)
    {
        LOG_PRINT_ERRNO(errno);
        switch (errno)
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
    return connfd;
}
}    
}    
}