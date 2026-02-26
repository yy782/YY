#include "../Common/noncopyable.h"

#include "sockets.h"
#include "EventHandler.h"
#include "InetAddress.h"
#include "EventLoop.h"
#include <assert.h>
#include <functional>
#include <memory>
namespace yy
{
namespace net
{
class TcpConnection;
class Acceptor:noncopyable
{
public:
    typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
    typedef std::function<void(TcpConnectionPtr)> NewConnectCallBack;
    Acceptor(const Address& addr,EventLoop* loop);
    int get_fd()const{return handler_->get_fd();}
    void setNewConnectCallBack(NewConnectCallBack cb){callback_=std::move(cb);}
    void listen()
    {
        sockets::listen(handler_->get_fd());
        handler_->setReading();
    }
private:
    void accept();
    Address addr_;
    EventLoop* loop_;
    EventHandlerPtr handler_;
    NewConnectCallBack callback_;
};

}  
}