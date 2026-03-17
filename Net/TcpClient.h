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
//     connection_(std::make_shared<TcpConnection>()),
//     ConnectHandler_(sockets::createTcpSocketOrDie(serverAddr.get_family()),loop)
//     {
//         ConnectHandler_.setWriteCallBack(std::bind(&TcpClient::connectEstablished,this));
//     }
//     ~TcpClient()
//     {
//         connection_->disconnect();
//     }
//     void connect()
//     {
//         connection_->connect();
//         EventHandler* handler_ = connection_->getHandler();
//         handler_->setReadingAndExcept();
//     }
//     void disconnect()
//     {


//         connection_->disconnect();
        
//     }
//     void send(const char* msg,size_t len)
//     {
//         connection_->send(msg,len);
//     }
//     void sendOutput()
//     {
//         connection_->sendOutput();
//     }
    
//     bool isConnected(){return connection_->isConnected();}
//     void setMessageCallBack(ServicesMessageCallBack callback){ connection_->setMessageCallBack(std::move(callback));}
//     void setCloseCallBack(ServicesCloseCallBack callback){ connection_->setCloseCallBack(std::move(callback));}
//     void setErrorCallBack(ServicesErrorCallBack callback){ connection_->setErrorCallBack(std::move(callback));}
//     TcpBuffer& getSendBuffer(){return connection_->getSendBuffer();}
//     TcpBuffer& getRecvBuffer(){return connection_->getRecvBuffer();}

//     Address getPeerAddr(){return serverAddr_;}
// private:
//     void connectEstablished();
//     Address serverAddr_;
//     EventLoop* loop_;
//     TcpConnectionPtr connection_; // @brief 由于回调要TcpConnectionPtr,创建栈对象shared_from_this()会报错

//     EventHandler ConnectHandler_;

// };
class TcpClient : noncopyable
{
 public:
    typedef std::function<void(TcpConnectionPtr)> ServicesConnectedCallback;
    typedef std::function<void(TcpClient*)> ServicesConnectFailCallback;
    typedef TcpConnection::ServicesMessageCallBack ServicesMessageCallBack;
    typedef TcpConnection::ServicesCloseCallBack ServicesCloseCallBack;
    typedef TcpConnection::ServicesErrorCallBack ServicesErrorCallBack;
    typedef TcpConnection::ConnectStatus Status;
    typedef TcpConnection::Buffer Buffer;
    typedef std::function<void(TcpConnectionPtr)> ServicesConnectCallBack;
    TcpClient(const Address& serverAddr,EventLoop* loop);
    ~TcpClient();  
    void connect();
    void disconnect();

    EventLoop* getLoop() const { return loop_; }
    bool retry() const { return retry_; }

    bool isConnecting(){return connection_->isConnecting();}
    bool isConnected(){return connection_->isConnected();}
    void enableRetry() { retry_ = true; }
    void setConnectedCallback(ServicesConnectedCallback callback){ connectedCallback_=std::move(callback);}
    void setConnectFailCallback(ServicesConnectFailCallback callback){ connectFailCallback_=std::move(callback);}
    void setMessageCallBack(ServicesMessageCallBack callback){ connection_->setMessageCallBack(std::move(callback));}
    void setCloseCallBack(ServicesCloseCallBack callback){ connection_->setCloseCallBack(std::move(callback));}
    void setErrorCallBack(ServicesErrorCallBack callback){ connection_->setErrorCallBack(std::move(callback));}
    Buffer& getSendBuffer(){return connection_->getSendBuffer();}
    Buffer& getRecvBuffer(){return connection_->getRecvBuffer();}
    Address getPeerAddr(){return addr_;}
    void send(const char* message,size_t len)
    {
        send(std::string(message,len));        
    }
    void send(std::string&& message)
    {
        if(isConnecting())
        {
            connection_->getSendBuffer().append(message.c_str(),message.size());
           
        }
        else 
        {
            connection_->send(std::forward<std::string>(message));
        }
        
    }
    void sendOutput()
    {connection_->sendOutput();}    

    private:
  
    void newConnection();


    const int fd_;
    const Address& addr_;
    EventLoop* loop_;
    TcpConnectionPtr connection_;
    EventHandler ConnectHandler_;
    std::atomic<bool> retry_;   // atomic
  

   ServicesConnectedCallback connectedCallback_;
   ServicesConnectFailCallback connectFailCallback_;
    // always in loop thread
  
    
};
}
}