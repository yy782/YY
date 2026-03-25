#include "TcpServer.h"
#include <memory>
namespace yy
{
namespace net
{
TcpServer::TcpServer(const Address& addr,int threadnum,EventLoop* loop):
loop_(loop),
acceptor_(std::make_unique<Acceptor>(addr,loop_)),
threadpool_(threadnum,loop)
{
    LOG_SYSTEM_INFO(addr.sockaddrToString());

    acceptor_->setNewConnectCallBack(std::bind(&TcpServer::newConnection,this,_1,_2));
        
}   

void TcpServer::loop()
{
    acceptor_->listen();
    threadpool_.run();
} 
void TcpServer::stop()
{
    threadpool_.stop();
}
void TcpServer::newConnection(int fd,const Address& addr)
{

    

    EventLoop* loop=threadpool_.getEventLoop();
    TcpConnectionPtr conn=std::make_shared<TcpConnection>(fd,addr,loop,addr.sockaddrToString().c_str());
    connects_.insert(conn);
    
    
    
   
    conn->setMessageCallBack(SmessageCallback_);
    conn->setCloseCallBack([this](TcpConnectionPtr con){
        ScloseCallback_(con);
        removeConnection(con);
    });
    conn->setErrorCallBack(SerrorCallback_);
    conn->setConnectCallBack(SconnectCallback_);
    conn->ConnectSuccess();
}
    
void TcpServer::removeConnection(TcpConnectionPtr conn)
{
    conn->getHandler()->get_loop()->submit([this,&conn](){
        assert(connects_.find(conn)!=connects_.end());
        connects_.erase(conn);
    });

}
}    
}