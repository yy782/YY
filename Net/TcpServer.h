#ifndef _YY_NET_TCPSERVER_H_
#define _YY_NET_TCPSERVER_H_

#include "../Common/noncopyable.h"
#include "../Common/SyncLog.h"
#include "Acceptor.h"
#include "EventHandler.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "TcpConnection.h"
#include "SignalHandler.h"
#include "TimerQueue.h"
#include "TimerWheel.h"
#include <set>
#include <memory>
#include <vector>
#include <boost/unordered/concurrent_flat_set.hpp>
namespace yy
{
namespace net
{



/**
 * @brief TCP服务器类，负责接受客户端连接
 * 
 * TcpServer封装了TCP服务器的功能，包括接受客户端连接、管理连接、处理连接事件等。
 * 它使用Acceptor来接受客户端连接，并使用EventLoopThreadPool来处理连接的IO事件。
 */
class TcpServer:public noncopyable
{
public:
    /**
     * @brief 连接映射类型
     */
    typedef  boost::concurrent_flat_set<std::shared_ptr<TcpConnection>> ConnectMap;
    //typedef std::vector<std::unique_ptr<Acceptor>> AcceptorList;
    /**
     * @brief 接收器指针类型
     */
    typedef std::unique_ptr<Acceptor> AcceptorPtr;
    /**
     * @brief 连接回调函数类型
     */
    typedef std::function<TcpConnectionPtr(int fd,const Address& addr,EventLoop* loop)> ServicesConnectionCallBack;
    /**
     * @brief 字符容器类型
     */
    typedef TcpConnection::CharContainer CharContainer;

    /**
     * @brief 构造函数
     * 
     * @param addr 服务器地址
     * @param threadnum 线程池大小
     * @param loop 事件循环
     */
    TcpServer(const Address& addr,int threadnum,EventLoop* loop);
    
    /**
     * @brief 析构函数
     */
    ~TcpServer()=default;
    /**
     * @brief 启动服务器
     * 
     * 启动服务器，开始接受客户端连接。
     */
    void setConnectCallBack(ServicesConnectionCallBack cb)
    {
        SconnectionCallBack_=std::move(cb);
    }
    void loop();
    
    /**
     * @brief 停止服务器
     * 
     * 停止服务器，不再接受新的客户端连接。
     */
    void stop();
private:
    /**
     * @brief 新连接回调
     * 
     * @param fd 套接字文件描述符
     * @param addr 客户端地址
     * 
     * 当有新的客户端连接时调用，创建TcpConnection对象。
     */
    void newConnection(int fd,const Address& addr);
    
    /**
     * @brief 移除连接
     * 
     * @param conn 连接对象
     * 
     * 当连接关闭时调用，清理连接资源。
     */
    void removeConnection(TcpConnectionPtr conn);
    
    /**
     * @brief 事件循环
     */
    EventLoop* loop_;
    
    /**
     * @brief 接收器
     */
    AcceptorPtr acceptor_;
    
    /**
     * @brief 事件循环线程池
     */
    EventLoopThreadPool threadpool_;
    
    /**
     * @brief 连接映射
     */
    ConnectMap connects_;

    ServicesConnectionCallBack SconnectionCallBack_;
}; 

}    
}


#endif