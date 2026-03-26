#include "Acceptor.h"
#include <vector>
namespace yy
{
namespace net
{
Acceptor::Acceptor(const Address& addr,EventLoop* loop):
addr_(addr),
loop_(loop),
handler_(sockets::createTcpSocketOrDie(addr.get_family()),loop_,"Acceptor"),
idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
    assert(idleFd_>=0);
    int fd=handler_.get_fd();
    sockets::set_CloseOnExec(fd);
    sockets::reuse_addr(fd);    
    sockets::reuse_port(fd); 
    if(addr.get_family()==AF_INET6)
    {
        sockets::OnlyIpv6(fd,true);
    }
    sockets::bindOrDie(fd,addr_);
    if(addr_.get_family()==AF_INET6)
    {
        handler_.setReadCallBack(std::bind(&Acceptor::accept<true>,this));
    }
    else 
    {
        handler_.setReadCallBack(std::bind(&Acceptor::accept<false>,this));
    }
    
}
Acceptor::~Acceptor() // 确保acceptor的生命周期比loop长
{
//   handler_.disableAll();
//   handler_.removeListen();
  sockets::close(handler_.get_fd());
}
   
}    
}