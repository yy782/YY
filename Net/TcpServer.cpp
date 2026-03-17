#include "TcpServer.h"

namespace yy
{
namespace net
{
TcpServer::TcpServer(const Address& addr,int threadnum,EventLoop* loop):
loop_(loop),
acceptor_(std::make_unique<Acceptor>(addr,loop_)),
threadpool_(threadnum)
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

    

    assert(connects_.find(conn)==connects_.end());
    connects_.insert(conn);
    conn->setMessageCallBack(SmessageCallback_);
    conn->setCloseCallBack(ScloseCallback_);
    conn->setErrorCallBack(SerrorCallback_);
  

    conn->setName(conn->getAddr().sockaddrToString().c_str());
    
    EventLoop* loop=threadpool_.getEventLoop();
    conn->getHandler()->init(conn->get_fd(),loop);
    conn->setReading(); // 启用读事件监听
    
    if(SconnectCallback_)SconnectCallback_(conn);

}
    
void TcpServer::removeConnection(TcpConnectionPtr conn)
{
    assert(connects_.find(conn)!=connects_.end());
    connects_.erase(conn);
    conn->disconnect();
}
}    
}