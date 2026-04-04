#ifndef YY_NET_ACCEPTOR_H_
#define YY_NET_ACCEPTOR_H_
#include "../Common/noncopyable.h"

#include "sockets.h"
#include "EventHandler.h"
#include "InetAddress.h"
#include "EventLoop.h"
#include "TcpConnection.h"
#include <assert.h>
#include <functional>
#include <memory>
#include <unordered_set>
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
/**
 * @brief 接受器类
 * 
 * 用于接受新的TCP连接。
 */
const int AcceptorinitialId=1000;
class TcpServer;

class Acceptor:noncopyable
{
public:
    /**
     * @brief 新连接回调函数类型
     */
    typedef std::function<TcpConnectionPtr(int fd,const Address& addr,EventLoop* loop)> ServicesConnectedCallBack;
    //typedef  boost::concurrent_flat_set<std::shared_ptr<TcpConnection>> ConnectMap;
    typedef std::unordered_set<TcpConnectionPtr> ConnectMap;
    /**
     * @brief 构造函数
     * 
     * @param addr 地址
     * @param loop 事件循环
     * @param Ser 所属的TcpServer
     * @param id 接受器ID
     */
    Acceptor(const Address& addr,EventLoop* loop,TcpServer* Ser);
    
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
    void setNewConnectCallBack(ServicesConnectedCallBack cb){SconnectCallBack_=std::move(cb);}
    
    /**
     * @brief 开始监听
     */
    void listen()
    {      
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
     * @brief 处理新连接
     * 
     * @param fd 文件描述符
     * @param addr 客户端地址
     */
    void NewConnection(int fd,const Address& addr);
    /**
     * @brief 移除连接
     * 
     * @param conn 要移除的连接
     */
    void removeConnection(TcpConnectionPtr conn);
    /**
     * @brief 地址
     */
    const Address& addr_;// inOne

    /**
     * @brief 事件循环
     */
    EventLoop* const loop_;
    
    /**
     * @brief 事件处理器
     */
    EventHandler handler_;

    /**
     * @brief 连接集合
     */
    ConnectMap connects_;// Not

    /**
     * @brief 新连接回调函数
     */
    ServicesConnectedCallBack SconnectCallBack_;// inOne

    
    
    /**
     * @brief 空闲文件描述符
     */
    int idleFd_;
    /**
     * @brief 所属的TcpServer
     */
    TcpServer* Ser_;
};

template<bool isIpv6>
void Acceptor::accept()
{
    Address addr;
    while(true)
    {
        int fd=sockets::acceptAutoOrDie<isIpv6>(handler_.fd(),addr);
       
        if(fd>0)
        {
            NewConnection(fd,addr);
        }
        else 
        {
            if (errno==EMFILE)
            {
                close(idleFd_);
                idleFd_=sockets::acceptAutoOrDie<isIpv6>(fd,addr);
                close(idleFd_);
                idleFd_= ::open("/dev/null",O_RDONLY|O_CLOEXEC);
            }
            if(errno==EAGAIN||errno==EWOULDBLOCK)
            {
                break;
            }
        }         
    }
} 

}  
}
#endif