#include "Acceptor.h"
#include <vector>
namespace yy
{
namespace net
{
Acceptor::Acceptor(const Address& addr,EventLoop* loop):
addr_(addr),
loop_(loop),
handler_(sockets::createTcpSocketOrDie(addr.family()),loop_,"Acceptor"),
idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
    assert(idleFd_>=0);
    int fd=handler_.fd();
    sockets::set_CloseOnExec(fd);
    sockets::reuseAddrOrDie(fd);    
    sockets::reusePortOrDie(fd); 
    if(addr.family()==AF_INET6)
    {
        sockets::OnlyIpv6OrDie(fd,true);
    }
    sockets::bindOrDie(fd,addr_);
    if(addr_.family()==AF_INET6)    
    {
        handler_.setReadCallBack([this]()
        {
            accept<true>();
        });
    }
    else 
    {
        handler_.setReadCallBack([this]()
        {
            accept<false>();
        });
    }
    
}
Acceptor::~Acceptor() // 确保acceptor的生命周期比loop长
{
//   handler_.disableAll();
//   handler_.removeListen();
  sockets::close(handler_.fd());
}
   
}    
}