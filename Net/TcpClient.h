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
    typedef TcpConnection::ServicesCloseCallBack ServicesMessageCallBack;
    typedef TcpConnection::ServicesCloseCallBack ServicesCloseCallBack;
    typedef TcpConnection::ServicesErrorCallBack ServicesErrorCallBack;
    typedef TcpConnection::FindCompleteMessageFunc FindCompleteMessageFunc;
    typedef std::function<void(TcpConnectionPtr)> ServicesConnectCallBack;

    TcpClient(const Address& serverAddr):
    serverAddr_(serverAddr),
    loopThread_(),
    connection_(std::make_shared<TcpConnection>(sockets::create_tcpsocket(serverAddr.get_family()),serverAddr,loopThread_.getEventLoop())),
    state_(State::Init)
    {
        loopThread_.sumbit(std::bind(&EventLoop::addListen,loopThread_.getEventLoop(),connection_->getHandler()));
    }
    ~TcpClient()
    {
        if(state_==State::Connected)connection_->disconnect();
        loopThread_.stop();
    }
    void connect()
    {
        connection_->connect();
        state_=State::Connected;
        auto handler_=connection_->getHandler();
        handler_->setReading();
        handler_->setExcept();
        loopThread_.run();
        if(Sconnect_callback_)Sconnect_callback_(connection_);
    }
    void disconnect()
    {
        assert(state_==State::Connected);
        if(Sdisconnect_callback_)Sdisconnect_callback_(connection_);
        connection_->disconnect();
        state_=State::Disconnected;
    }
    void send(const char* msg,size_t len)
    {
        connection_->send(msg,len);
    }
    void setMessageCallBack(ServicesMessageCallBack callback){ connection_->setMessageCallBack(std::move(callback));}
    void setConnectCallBack(ServicesConnectCallBack callback){ Sconnect_callback_=std::move(callback);}
    void setCloseCallBack(ServicesCloseCallBack callback){ Sdisconnect_callback_=std::move(callback);}
    void setErrorCallBack(ServicesErrorCallBack callback){ connection_->setErrorCallBack(std::move(callback));}
    void setRMessageBorder(FindCompleteMessageFunc cb){connection_->setRMessageBorder(std::move(cb));}
    void setWMessageBorder(FindCompleteMessageFunc cb){connection_->setWMessageBorder(std::move(cb));}

    Address getPeerAddr(){return serverAddr_;}
private:
    Address serverAddr_;
    EventLoopThread loopThread_;
    TcpConnectionPtr connection_; // @brief 由于回调要TcpConnectionPtr,创建栈对象shared_from_this()会报错
    ServicesConnectCallBack Sconnect_callback_; // @这里是客户端主动关闭时的回调
    ServicesCloseCallBack Sdisconnect_callback_;
    enum class State
    {
        Init=1,
        Connected,
        Disconnected
    };
    State state_;

};
}
}