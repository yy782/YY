#ifndef _YY_NET_TCPSERVER_H_
#define _YY_NET_TCPSERVER_H_

#include "../Common/noncopyable.h"
#include "../Common/SyncLog.h"
#include "AcceptorPool.h"
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
     * @brief 接收器指针类型
     */
    /**
     * @brief 连接回调函数类型
     */
    typedef Acceptor::ServicesConnectCallBack ServicesConnectCallBack;
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
    TcpServer(const Address& addr,int AcceptorNum,int WorkThreadnum,EventLoop* loop);
    /**
     * @brief 析构函数
     */
    ~TcpServer()=default;
    /**
     * @brief 启动服务器
     * 
     * 启动服务器，开始接受客户端连接。
     */
    void setConnectCallBack(ServicesConnectCallBack cb)
    {
        AcceptorPool_.setNewConnectCallBack(std::move(cb)); 
    }
    void loop();
    void wait(); 
    /**
     * @brief 停止服务器
     * 
     * 停止服务器，不再接受新的客户端连接。
     */
    void stop();


private:
    friend class Acceptor;
    EventLoop* NextLoop();
    
    /**
     * @brief 移除连接
     * 
     * @param conn 连接对象
     * 
     * 当连接关闭时调用，清理连接资源。
     */
    
    /**
     * @brief 事件循环
     */
    EventLoop* loop_;
    
    /**
     * @brief 接收器
     */
    AcceptorPool AcceptorPool_;
    /**
     * @brief 事件循环线程池
     */
    EventLoopThreadPool WorkThreadPool_;
    
    /**
     * @brief 连接映射
     */

}; 

}    
}


#endif