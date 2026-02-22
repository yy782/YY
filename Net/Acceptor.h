#include "../Common/noncopyable.h"
#include "sockets.h"
#include "EventHandler.h"
#include "InetAddress.h"
#include "EventLoop.h"
#include <assert.h>
#include <functional>
namespace yy
{
namespace net
{

class Acceptor:noncopyable
{
public:
    typedef std::function<void(int fd,Address addr)> NewConnectCallBack;
    Acceptor(const Address& addr,EventLoop* loop):
    addr_(addr),
    loop_(loop),
    handler_(sockets::create_tcpsocket(addr.get_family()),loop_)
    {

        int fd=handler_.get_fd();
        sockets::set_CloseOnExec(fd);
        sockets::reuse_addr(fd);
        sockets::reuse_port(fd);        
        sockets::bind(fd,addr_);
        handler_.setReadCallBack(std::bind(&Acceptor::accept,this));

        loop_->submit(std::bind(&EventLoop::addListen,loop_,&handler_));
    }
    int get_fd()const{return handler_.get_fd();}
    void setNewConnectCallBack(NewConnectCallBack cb){callback_=std::move(cb);}
    void listen()
    {
        sockets::listen(handler_.get_fd());
        handler_.setReading();
    }
private:
    void accept()
    {
        Address addr;
        int fd;
        if(addr_.get_family()==AF_INET6)
        {
            fd=sockets::accept(handler_.get_fd(),addr,true);
        }
        else
        {
            fd=sockets::accept(handler_.get_fd(),addr,false);
        }
        callback_(fd,addr);
    }
    Address addr_;
    EventLoop* loop_;
    EventHandler handler_;
    NewConnectCallBack callback_;
};

}  
}