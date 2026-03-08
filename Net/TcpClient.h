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

    TcpClient(const Address& serverAddr):
    serverAddr_(serverAddr),
    loopThread_(),
    connection_(std::make_shared<TcpConnection>(sockets::create_tcpsocket(serverAddr.get_family()),serverAddr,loopThread_.getEventLoop()))
    {
        loopThread_.getEventLoop()->addListen(connection_->getHandler());
    }
    ~TcpClient()
    {
        connection_->disconnect();
        loopThread_.stop();
    }
    void connect()
    {
        connection_->connect();
        auto handler_=connection_->getHandler();
        handler_->setReading();
        handler_->setExcept();
        loopThread_.run();

    }
    void disconnect()
    {


        connection_->disconnect();
        
    }
    void send(const char* msg,size_t len)
    {
        connection_->send(msg,len);
    }
    bool isConnected(){return connection_->isConnected();}
    void setMessageCallBack(ServicesMessageCallBack callback){ connection_->setMessageCallBack(std::move(callback));}
    void setCloseCallBack(ServicesCloseCallBack callback){ connection_->setCloseCallBack(std::move(callback));}
    void setErrorCallBack(ServicesErrorCallBack callback){ connection_->setErrorCallBack(std::move(callback));}
    

    Address getPeerAddr(){return serverAddr_;}
private:
    Address serverAddr_;
    EventLoopThread loopThread_;
    TcpConnectionPtr connection_; // @brief 由于回调要TcpConnectionPtr,创建栈对象shared_from_this()会报错

};
}
}