#include "Acceptor.h"
#include "TcpConnection.h"
namespace yy
{
namespace net
{
Acceptor::Acceptor(const Address& addr,EventLoop* loop):
addr_(addr),
loop_(loop),
handler_(sockets::createTcpSocketOrDie(addr.get_family()),loop_)
{

    int fd=handler_.get_fd();
    sockets::set_CloseOnExec(fd);
    sockets::reuse_addr(fd);
    sockets::reuse_port(fd); 
    if(addr.get_family()==AF_INET6)
    {
        sockets::OnlyIpv6(fd,true);
    }
    sockets::bindOrDie(fd,addr_);
    handler_.setReadCallBack(std::bind(&Acceptor::accept,this));
    handler_.setReading();
    handler_.set_name("Acceptor");

    loop_->addListen(&handler_);
}
Acceptor::~Acceptor()
{
    
}
void Acceptor::accept()
{
    Address addr;
    int fd;
    if(addr_.get_family()==AF_INET6)
    {
        fd=sockets::acceptAutoOrDie(handler_.get_fd(),addr,true);
    }
    else
    {
        fd=sockets::acceptAutoOrDie(handler_.get_fd(),addr,false);
    }
    auto conn=TcpConnection::accept(fd,addr);

    conn->getHandler()->set_name(addr.sockaddrToString());
    
    callback_(conn);
}    
}    
}