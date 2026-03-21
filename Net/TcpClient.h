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
// class TcpClient:noncopyable
// {
// public:
//     typedef TcpConnection::ServicesMessageCallBack ServicesMessageCallBack;
//     typedef TcpConnection::ServicesCloseCallBack ServicesCloseCallBack;
//     typedef TcpConnection::ServicesErrorCallBack ServicesErrorCallBack;
//     typedef TcpConnection::ConnectStatus Status;
//     typedef std::function<void(TcpConnectionPtr)> ServicesConnectCallBack;

//     TcpClient(const Address& serverAddr,EventLoop* loop):
//     serverAddr_(serverAddr),
//     loop_(loop),
//     conn_(std::make_shared<TcpConnection>()),
//     ConnectHandler_(sockets::createTcpSocketOrDie(serverAddr.get_family()),loop)
//     {
//         ConnectHandler_.setWriteCallBack(std::bind(&TcpClient::connectEstablished,this));
//     }
//     ~TcpClient()
//     {
//         conn_->disconnect();
//     }
//     void connect()
//     {
//         conn_->connect();
//         EventHandler* handler_ = conn_->getHandler();
//         handler_->setReadingAndExcept();
//     }
//     void disconnect()
//     {


//         conn_->disconnect();
        
//     }
//     void send(const char* msg,size_t len)
//     {
//         conn_->send(msg,len);
//     }
//     void sendOutput()
//     {
//         conn_->sendOutput();
//     }
    
//     bool isConnected(){return conn_->isConnected();}
//     void setMessageCallBack(ServicesMessageCallBack callback){ conn_->setMessageCallBack(std::move(callback));}
//     void setCloseCallBack(ServicesCloseCallBack callback){ conn_->setCloseCallBack(std::move(callback));}
//     void setErrorCallBack(ServicesErrorCallBack callback){ conn_->setErrorCallBack(std::move(callback));}
//     TcpBuffer& getSendBuffer(){return conn_->getSendBuffer();}
//     TcpBuffer& getRecvBuffer(){return conn_->getRecvBuffer();}

//     Address getPeerAddr(){return serverAddr_;}
// private:
//     void connectEstablished();
//     Address serverAddr_;
//     EventLoop* loop_;
//     TcpConnectionPtr conn_; // @brief 由于回调要TcpConnectionPtr,创建栈对象shared_from_this()会报错

//     EventHandler ConnectHandler_;

// };
class TcpClient : noncopyable {
public:
    typedef TcpConnection::ServicesConnectionCallBack ServicesConnectionCallBack;
    typedef std::function<void(TcpClient*)> ServicesConnectFailCallback;
    typedef TcpConnection::ServicesMessageCallBack ServicesMessageCallBack;
    typedef TcpConnection::CloseCallBack ServicesCloseCallback;
    typedef TcpConnection::ServicesErrorCallBack ServicesErrorCallBack;
    typedef TcpConnection::Buffer Buffer;

    TcpClient(const Address& serverAddr,EventLoop* loop);
    ~TcpClient();

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
    void send(std::string&& message);
    void send(const char* data, size_t len);
    void sendOutput();

    TcpConnectionPtr connection() const { return connection_; }
    EventLoop* getLoop() const { return loop_; }
    const Address& getPeerAddr() const { return serverAddr_; }
    Buffer& getSendBuffer() { return connection_->getSendBuffer(); }
    Buffer& getRecvBuffer() { return connection_->getRecvBuffer(); }

private:
    // Pimpl: 隐藏所有连接细节
    struct Connector;
    

    EventLoop* loop_;
    Address serverAddr_;
    std::atomic<bool> retry_;
    std::unique_ptr<Connector> connector_;
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