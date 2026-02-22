#include "TcpServer.h"

namespace yy
{
namespace net
{
TcpServer::TcpServer(const Address& addr,int threadnum):
loop_(),
acceptor_(addr,&loop_),
handler_(acceptor_.get_fd(),&loop_),
threadpool_(threadnum)
{
    auto addConnect=[this](int fd,Address addr_in)
    {
        auto newconnect=std::make_unique<TcpConnect>(fd,addr_in,&loop_);
        
        threadpool_.submit([this,Newconnect=newconnect.get()]()mutable
        {
            auto handler=Newconnect->getHandler();
            assert(handler);
            handler->setReadCallBack(messageCallback_);
            handler->setCloseCallBack(closeCallback_);
            connectCallback_(Newconnect->get_fd(),Newconnect->getAddr());
            threadpool_.submit(std::bind(&EventLoop::addListen, &loop_,handler));
        });
        connects_.insert(std::move(newconnect));
    };
    acceptor_.setNewConnectCallBack(addConnect);
}   
void TcpServer::loop()
{
    loop_.loop();
} 
void TcpServer::stop()
{
    loop_.quit();
}
}    
}