#include "TcpServer.h"
#include <memory>
namespace yy
{
namespace net
{   
TcpServer::TcpServer(const Address& addr,int AcceptorNum,int WorkThreadnum):
AcceptorPool_(addr,this,AcceptorNum),
WorkThreadPool_(WorkThreadnum)
{
    LOG_SYSTEM_INFO(addr.sockaddrToString());       
}  
void TcpServer::loop()
{
    AcceptorPool_.run();
    WorkThreadPool_.run();
} 
void TcpServer::stop()
{
    AcceptorPool_.stop();
    WorkThreadPool_.stop();
}
void TcpServer::wait() 
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    
    int sig;
    sigwait(&mask, &sig);  // 阻塞直到收到信号
}
EventLoop* TcpServer::NextLoop()
{
    return WorkThreadPool_.getEventLoop();
}
    
}    
}