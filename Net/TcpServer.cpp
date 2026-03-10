#include "TcpServer.h"

namespace yy
{
namespace net
{
TcpServer::TcpServer(const Address& addr,int threadnum,EventLoop* loop):
loop_(loop),
acceptor_(std::make_unique<Acceptor>(addr,loop_)),
threadpool_(threadnum),
signalHandler_(loop_),
LTimerQueue_(loop_),
HTimerQueue_(loop_),
TimerWheel_(loop_)
{
    LOG_SYSTEM_INFO(addr.sockaddrToString());

    acceptor_->setNewConnectCallBack(std::bind(&TcpServer::newConnection,this,_1));
    acceptor_->listen();    
}   

void TcpServer::loop()
{
    threadpool_.run();
} 
void TcpServer::stop()
{
    threadpool_.stop();
}
void TcpServer::newConnection(TcpConnectionPtr conn)
{

    SconnectCallback_(conn);

    assert(connects_.find(conn)==connects_.end());
    connects_.insert(conn);
    conn->setMessageCallBack(SmessageCallback_);
    conn->setCloseCallBack(ScloseCallback_);
    conn->setErrorCallBack(SerrorCallback_);
  

    conn->setName(conn->getAddr().sockaddrToString().c_str());
    threadpool_.addHandler(conn->getHandler());
}
void TcpServer::addTime(TimerCallBack cb,int interval,int execute_count,bool is_persice,bool isisHighPrecision)
{
    if(is_persice)
        if(isisHighPrecision)
            HTimerQueue_.insert(std::move(cb),interval,execute_count);
        else
            LTimerQueue_.insert(std::move(cb),interval,execute_count);
    else
        TimerWheel_.insert(std::move(cb),interval,execute_count);
}
void TcpServer::removeConnection(TcpConnectionPtr conn)
{
    assert(connects_.find(conn)!=connects_.end());
    connects_.erase(conn);
    conn->disconnect();
}
}    
}