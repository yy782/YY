#ifndef _YY_NET_TCPCONNECT_H_
#define _YY_NET_TCPCONNECT_H_
#include "../Common/noncopyable.h"
#include "InetAddress.h"
#include "EventHandler.h"
namespace yy
{
namespace net
{

    
class TcpConnect:noncopyable
{
public:
    TcpConnect(int fd,Address addr,EventLoop* loop):
    addr_(addr),
    handler_(fd,loop)
    {}
    //TcpConnect(TcpConnect&& other);
    EventHandler* getHandler(){return &handler_;}    
    int get_fd()const{return handler_.get_fd();}
    Address getAddr()const{return addr_;}
    
private:
    Address addr_;
    EventHandler handler_;
};
}    
}


#endif