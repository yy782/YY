#ifndef _YY_NET_CLIENT_H_
#define _YY_NET_CLIENT_H_
#include "InetAddress.h"
#include "sockets.h"
#include "TcpConnection.h"
#include "EventLoop.h"
#include <functional>
#include <atomic>
namespace yy
{
namespace net
{
/**
 * @brief TCP客户端类，负责与服务器建立连接
 * 
 * TcpClient封装了TCP客户端的连接管理，包括连接建立、断开、重连等功能。
 * 它通过Connector来处理连接的建立过程，并在连接建立后创建TcpConnection对象来管理连接。
 */
class TcpClient : noncopyable// 需要强行保证生命周期长,要求loop处理完所有connector的任务后再构析，否则内存泄漏
                     {
public:
    typedef std::function<TcpConnectionPtr(int fd,const Address& addr,EventLoop* loop)> ServicesConnectedCallBack;
    /**
     * @brief 连接失败回调函数类型
     */
    typedef std::function<void()> ServicesConnectFailCallBack;
    /**
     * @brief 缓冲区类型
     */
    typedef TcpConnection::Buffer Buffer;

    /**
     * @brief 构造函数
     * 
     * @param serverAddr 服务器地址
     * @param loop 事件循环
     * 
     * 创建一个TcpClient实例，初始化连接参数。
     */
    TcpClient(const Address& serverAddr,EventLoop* loop);
    
    /**
     * @brief 析构函数
     * 
     * 析构TcpClient实例，清理资源。
     */
    ~TcpClient();
    
    // void setEvent(Event e)
    // {
    //     connection_->setEvent(e);
    // }
    
    /**
     * @brief 连接控制
     * 
     * 建立与服务器的连接。
     */
    void connect();
    
    /**
     * @brief 断开连接
     * 
     * 断开与服务器的连接。
     */
    void disconnect();
    
    /**
     * @brief 完全停止
     * 
     * 完全停止客户端，不再重连。
     */
    void stop();  // 完全停止，不再重连

    /**
     * @brief 重连控制
     * 
     * 启用重连功能。
     */
    void enableRetry() { retry_ = true; }
    
    /**
     * @brief 禁用重连
     * 
     * 禁用重连功能。
     */
    void disableRetry() { retry_ = false; }
    
    /**
     * @brief 检查是否启用重连
     * 
     * @return bool 是否启用重连
     */
    bool retry() const noexcept { return retry_; }

    /**
     * @brief 状态查询
     * 
     * @return bool 是否已连接
     */
    bool isConnected() const noexcept;
    
    /**
     * @brief 检查是否正在连接
     * 
     * @return bool 是否正在连接
     */
    bool isConnecting() const noexcept;

    /**
     * @brief 回调设置
     * 
     * @param cb 连接回调函数
     */
    void setConnectionCallback(ServicesConnectedCallBack cb) { SconnectionCallback_ = std::move(cb); }
    
    /**
     * @brief 设置连接失败回调函数
     * 
     * @param cb 连接失败回调函数
     */
    void setConnectFailCallback(ServicesConnectFailCallBack cb) { SconnectFailCallback_ = std::move(cb); }

    /**
     * @brief 获取连接对象
     * 
     * @return TcpConnectionPtr 连接对象
     */
    TcpConnectionPtr getConnection() const noexcept { return connection_; }
    
    /**
     * @brief 获取事件循环
     * 
     * @return EventLoop* 事件循环
     */
    EventLoop* getLoop() const noexcept { return loop_; }
    
    /**
     * @brief 获取服务器地址
     * 
     * @return const Address& 服务器地址
     */
    const Address& getPeerAddr() const noexcept { return serverAddr_; }
    
    // Buffer& getSendBuffer() { return connection_->getSendBuffer(); }
    // Buffer& getRecvBuffer() { return connection_->getRecvBuffer(); }

private:
    /**
     * @brief 连接器结构体，负责处理连接的建立过程
     * 
     * Connector封装了连接建立的细节，包括连接尝试、重连等功能。
     */
    struct Connector;
    
    /**
     * @brief 事件循环
     */
    EventLoop* loop_;//InOne
    
    /**
     * @brief 服务器地址
     */
    const Address& serverAddr_;
    
    /**
     * @brief 是否启用重连
     */
    bool retry_;// 需要外部强行保证

    /**
     * @brief 连接器
     */
    std::shared_ptr<Connector> connector_;//InLoop
    
    /**
     * @brief 连接对象
     */
    TcpConnectionPtr connection_;

    /**you
     * @brief 回调函数
     */
    ServicesConnectedCallBack SconnectionCallback_;
    /**
     * @brief 连接失败回调函数
     */
    ServicesConnectFailCallBack SconnectFailCallback_;

    /**
     * @brief 新连接回调
     * 
     * @param sockfd 套接字文件描述符
     * 
     * 当连接建立成功时调用，创建TcpConnection对象。
     */
    void newConnection(int sockfd);
    
    /**
     * @brief 移除连接
     * 
     * 当连接关闭时调用，清理连接资源。
     */
    void removeConnection();
    
    /**
     * @brief 连接失败回调
     * 
     * 当连接建立失败时调用。
     */
    void connectFail();
};
}
}
#endif 