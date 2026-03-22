#include "Acceptor.h"
#include <vector>
namespace yy
{
namespace net
{
Acceptor::Acceptor(const Address& addr,EventLoop* loop):
addr_(addr),
loop_(loop),
handler_(sockets::createTcpSocketOrDie(addr.get_family()),loop_),
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
    handler_.setReadCallBack(std::bind(&Acceptor::accept,this));
    handler_.setReading();
    handler_.set_name("Acceptor");


}
Acceptor::~Acceptor()
{
  handler_.disableAll();
  handler_.removeListen();
}
void Acceptor::accept()
{
    Address addr;
    while(true)
    {
        int fd;
        if(addr_.get_family()==AF_INET6)
        {
            fd=sockets::acceptAutoOrDie(handler_.get_fd(),addr,true);
        }
        else
        {
            fd=sockets::acceptAutoOrDie(handler_.get_fd(),addr,false);
        }
        if(fd>0&&sockets::setNonBlocking(fd))
        {
            
            assert(callback_);
            callback_(fd,addr);
        }
        else 
        {
            if (errno == EMFILE)
            {
                close(idleFd_);
                idleFd_=sockets::acceptAutoOrDie(fd,addr,true);
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