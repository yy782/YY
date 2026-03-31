/**
 * @file TcpServer.h
 * @brief TCP服务器类的定义
 * 
 * 本文件定义了TCP服务器类，负责接受客户端连接、管理连接、处理连接事件等。
 * 它使用AcceptorPool来接受客户端连接，并使用EventLoopThreadPool来处理连接的IO事件。
 */

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
 * @brief TCP服务器类
 * 
 * TcpServer封装了TCP服务器的功能，包括接受客户端连接、管理连接、处理连接事件等。
 * 它使用AcceptorPool来接受客户端连接，并使用EventLoopThreadPool来处理连接的IO事件。
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
    typedef Acceptor::ServicesConnectedCallBack ServicesConnectedCallBack;
    /**
     * @brief 字符容器类型
     */
    typedef TcpConnection::CharContainer CharContainer;

    /**
     * @brief 构造函数
     * 
     * @param addr 服务器地址
     * @param AcceptorNum 接收器数量
     * @param WorkThreadnum 工作线程数量
     */
    TcpServer(const Address& addr,int AcceptorNum,int WorkThreadnum);
    /**
     * @brief 析构函数
     */
    ~TcpServer()=default;
    /**
     * @brief 设置连接回调函数
     * 
     * @param cb 连接回调函数
     */
    void setConnectCallBack(ServicesConnectedCallBack cb)
    {
        AcceptorPool_.setNewConnectCallBack(std::move(cb)); 
    }
    /**
     * @brief 启动服务器
     * 
     * 启动服务器，开始接受客户端连接。
     */
    void loop();
    /**
     * @brief 等待服务器停止
     * 
     * 等待服务器停止，阻塞直到服务器完全停止。
     */
    void wait(); 
    /**
     * @brief 停止服务器
     * 
     * 停止服务器，不再接受新的客户端连接。
     */
    void stop();


private:
    friend class Acceptor;
    /**
     * @brief 获取下一个事件循环
     * 
     * @return EventLoop* 事件循环
     */
    EventLoop* NextLoop();
    
    
    /**
     * @brief 接收器池
     */
    AcceptorPool AcceptorPool_;// InOne
    /**
     * @brief 事件循环线程池
     */
    EventLoopThreadPool WorkThreadPool_;
    

}; 

}    
}


#endif