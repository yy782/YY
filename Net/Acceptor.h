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
    int fd()const{return handler_.fd();}
    void setNewConnectCallBack(NewConnectCallBack cb){callback_=std::move(cb);}
    void listen()
    {
        handler_.set_event(Event(LogicEvent::Read|LogicEvent::Edge));
       
        sockets::listenOrDie(handler_.fd());
        
    }
private:
    template<bool isIpv6=false>
    void accept();
    Address addr_;
    EventLoop* loop_;
    EventHandler handler_;
    NewConnectCallBack callback_;
    int idleFd_;
};
template<bool isIpv6>
void Acceptor::accept()
{
    Address addr;
    while(true)
    {
        int fd=sockets::acceptAutoOrDie<isIpv6>(handler_.fd(),addr);
       
        if(fd>0)
        {
            
            assert(callback_);
            callback_(fd,addr);
        }
        else 
        {
            if (errno == EMFILE)
            {
                close(idleFd_);
                idleFd_=sockets::acceptAutoOrDie<isIpv6>(fd,addr);
                close(idleFd_);
                idleFd_= ::open("/dev/null",O_RDONLY|O_CLOEXEC);
            }
            if(errno==EAGAIN||errno == EWOULDBLOCK)
            {
                break;
            }
        }         
    }
} 
}  
}
#endif