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
void TcpServer::newConnection(int fd,const Address& addr)
{

    

    EventLoop* loop=threadpool_.getEventLoop();
    TcpConnectionPtr conn=std::make_shared<TcpConnection>(fd,addr,loop);
    connects_.insert(conn);
    conn->setName(conn->getAddr().sockaddrToString().c_str());
    
    
   
    conn->setMessageCallBack(SmessageCallback_);
    conn->setCloseCallBack([this,loop](TcpConnectionPtr con){
        ScloseCallback_(con);
        removeConnection(con,loop);
    });
    conn->setErrorCallBack(SerrorCallback_);
    conn->setConnectCallBack(SconnectCallback_);
    conn->ConnectSuccess();
}
    
void TcpServer::removeConnection(TcpConnectionPtr conn,EventLoop* loop)
{
    loop->submit([this,&conn,loop](){
        assert(connects_.find(conn)!=connects_.end());
        connects_.erase(conn);
    });

}
}    
}