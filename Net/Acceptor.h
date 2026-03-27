#ifndef YY_NET_ACCEPTOR_H_
#define YY_NET_ACCEPTOR_H_
#include "../Common/noncopyable.h"

#include "sockets.h"
#include "EventHandler.h"
#include "InetAddress.h"
#include "EventLoop.h"
#include <assert.h>
#include <functional>
#include <memory>
namespace yy
{
namespace net
{

/**
 * @file Acceptor.h
 * @brief 接受器类的定义
 * 
 * 本文件定义了接受器类，用于接受新的TCP连接。
 */

class TcpConnection;

/**
 * @brief 接受器类
 * 
 * 用于接受新的TCP连接。
 */
class Acceptor:noncopyable
{
public:
    /**
     * @brief 新连接回调函数类型
     */
    typedef std::function<void(int fd,const Address&)> NewConnectCallBack;
    
    /**
     * @brief 构造函数
     * 
     * @param addr 地址
     * @param loop 事件循环
     */
    Acceptor(const Address& addr,EventLoop* loop);
    
    /**
     * @brief 析构函数
     */
    ~Acceptor();
    
    /**
     * @brief 获取文件描述符
     * 
     * @return int 文件描述符
     */
    int fd()const{return handler_.fd();}
    
    /**
     * @brief 设置新连接回调函数
     * 
     * @param cb 回调函数
     */
    void setNewConnectCallBack(NewConnectCallBack cb){callback_=std::move(cb);}
    
    /**
     * @brief 开始监听
     */
    void listen()
    {
        handler_.set_event(Event(LogicEvent::Read|LogicEvent::Edge));
       
        sockets::listenOrDie(handler_.fd());
        
    }
private:
    /**
     * @brief 接受连接
     * 
     * @tparam isIpv6 是否是IPv6
     */
    template<bool isIpv6=false>
    void accept();
    
    /**
     * @brief 地址
     */
    Address addr_;
    
    /**
     * @brief 事件循环
     */
    EventLoop* loop_;
    
    /**
     * @brief 事件处理器
     */
    EventHandler handler_;
    
    /**
     * @brief 新连接回调函数
     */
    NewConnectCallBack callback_;
    
    /**
     * @brief 空闲文件描述符
     */
    int idleFd_;
};
/**
 * @brief 接受连接
 * 
 * @tparam isIpv6 是否是IPv6
 */
template<bool isIpv6>
void Acceptor::accept()
{
    Address addr;
    while(true)
    {
        int fd=sockets::acceptAutoOrDie<isIpv6>(handler_.fd(),addr);
       
        if(fd>0)
        {
            
            assert(callback_);
            callback_(fd,addr);
        }
        else 
        {
            if (errno == EMFILE)
            {
                close(idleFd_);
                idleFd_=sockets::acceptAutoOrDie<isIpv6>(fd,addr);
                close(idleFd_);
                idleFd_= ::open("/dev/null",O_RDONLY|O_CLOEXEC);
            }
            if(errno==EAGAIN||errno == EWOULDBLOCK)
            {
                break;
            }
        }         
    }
} 
}  
}
#endif