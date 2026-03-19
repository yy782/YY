#ifndef YY_NET_ACCEPTOR_H_
#define YY_NET_ACCEPTOR_H_
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
    
    typedef std::function<void(int fd,const Address&)> NewConnectCallBack;
    Acceptor(const Address& addr,EventLoop* loop);
    ~Acceptor();
    int get_fd()const{return handler_.get_fd();}
    void setNewConnectCallBack(NewConnectCallBack cb){callback_=std::move(cb);}
    void listen()
    {
        handler_.setReading();
        loop_->submit([this](){
            sockets::listenOrDie(handler_.get_fd());
        });
    }
private:
    void accept();
    Address addr_;
    EventLoop* loop_;
    EventHandler handler_;
    NewConnectCallBack callback_;
    int idleFd_;
};

}  
}
#endif