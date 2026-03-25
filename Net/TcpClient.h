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
class TcpClient : noncopyable
                     {
public:
    typedef TcpConnection::ServicesConnectionCallBack ServicesConnectionCallBack;
    typedef std::function<void()> ServicesConnectFailCallback;
    typedef TcpConnection::ServicesMessageCallBack ServicesMessageCallBack;
    typedef TcpConnection::CloseCallBack ServicesCloseCallback;
    typedef TcpConnection::ServicesErrorCallBack ServicesErrorCallBack;
    typedef TcpConnection::Buffer Buffer;

    TcpClient(const Address& serverAddr,EventLoop* loop);
    ~TcpClient();
    // void setEvent(Event e)
    // {
    //     connection_->setEvent(e);
    // }
    // 连接控制
    void connect();
    void disconnect();
    void stop();  // 完全停止，不再重连

    // 重连控制
    void enableRetry() { retry_ = true; }
    void disableRetry() { retry_ = false; }
    bool retry() const { return retry_; }

    // 状态查询
    bool isConnected() const;
    bool isConnecting() const;

    // 回调设置
    void setConnectionCallback(ServicesConnectionCallBack cb) { SconnectionCallback_ = std::move(cb); }
    void setConnectFailCallback(ServicesConnectFailCallback cb) { SconnectFailCallback_ = std::move(cb); }
    void setMessageCallBack(ServicesMessageCallBack cb) { SmessageCallback_ = std::move(cb); }
    void setCloseCallBack(ServicesCloseCallback cb) { ScloseCallback_ = std::move(cb); }
    void setErrorCallback(ServicesErrorCallBack cb) {SerrorCallback_ = std::move(cb); }

    // 发送数据
    // void send(std::string&& message);
    // void send(const char* data, size_t len);
    // void sendOutput();

    TcpConnectionPtr getConnection() const { return connection_; }
    EventLoop* getLoop() const { return loop_; }
    const Address& getPeerAddr() const { return serverAddr_; }
    // Buffer& getSendBuffer() { return connection_->getSendBuffer(); }
    // Buffer& getRecvBuffer() { return connection_->getRecvBuffer(); }

private:
    // Pimpl: 隐藏所有连接细节
    struct Connector;
    

    EventLoop* loop_;
    Address addr_;
    Address serverAddr_;
    bool retry_;

    std::shared_ptr<Connector> connector_;
    TcpConnectionPtr connection_;


    // 回调函数
    ServicesConnectionCallBack SconnectionCallback_;
    ServicesConnectFailCallback SconnectFailCallback_;
    ServicesMessageCallBack SmessageCallback_;
    ServicesCloseCallback ScloseCallback_;
    ServicesErrorCallBack SerrorCallback_;

    void newConnection(int sockfd);
    void removeConnection();
    void connectFail();
};
}
}
#endif 