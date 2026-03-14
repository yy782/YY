#include "InetAddress.h"
#include "sockets.h"
#include "TcpConnection.h"
#include "EventLoop.h"
#include <functional>
namespace yy
{
namespace net
{
class TcpClient:noncopyable
{
public:
    typedef TcpConnection::ServicesMessageCallBack ServicesMessageCallBack;
    typedef TcpConnection::ServicesCloseCallBack ServicesCloseCallBack;
    typedef TcpConnection::ServicesErrorCallBack ServicesErrorCallBack;
    typedef TcpConnection::ConnectStatus Status;
    typedef std::function<void(TcpConnectionPtr)> ServicesConnectCallBack;

    TcpClient(const Address& serverAddr,EventLoop* loop):
    serverAddr_(serverAddr),
    loop_(loop),
    connection_(std::make_shared<TcpConnection>(sockets::createTcpSocketOrDie(serverAddr.get_family()),serverAddr,loop))
    {
        
    }
    ~TcpClient()
    {
        connection_->disconnect();
    }
    void connect()
    {
        connection_->connect();
        EventHandler* handler_ = connection_->getHandler();
        handler_->setReadingAndExcept();
        

    }
    void disconnect()
    {


        connection_->disconnect();
        
    }
    void send(const char* msg,size_t len)
    {
        connection_->send(msg,len);
    }
    void sendOutput()
    {
        connection_->sendOutput();
    }
    
    bool isConnected(){return connection_->isConnected();}
    void setMessageCallBack(ServicesMessageCallBack callback){ connection_->setMessageCallBack(std::move(callback));}
    void setCloseCallBack(ServicesCloseCallBack callback){ connection_->setCloseCallBack(std::move(callback));}
    void setErrorCallBack(ServicesErrorCallBack callback){ connection_->setErrorCallBack(std::move(callback));}
    TcpBuffer& getSendBuffer(){return connection_->getSendBuffer();}
    TcpBuffer& getRecvBuffer(){return connection_->getRecvBuffer();}

    Address getPeerAddr(){return serverAddr_;}
private:
    Address serverAddr_;
    EventLoop* loop_;
    TcpConnectionPtr connection_; // @brief 由于回调要TcpConnectionPtr,创建栈对象shared_from_this()会报错

};
}
}