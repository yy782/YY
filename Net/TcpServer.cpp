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

    acceptor_->setNewConnectCallBack([this](int fd, const Address& address)
    {
        newConnection(fd, address);
    });
        
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

    
    assert(loop_->isInLoopThread());
    EventLoop* loop=threadpool_.getEventLoop();
    assert(SconnectionCallBack_);
    auto conn=SconnectionCallBack_(fd,addr,loop);
    conn->setDestructorCallBack([this](TcpConnectionPtr con)
    {
        removeConnection(con);
    });
    assert(!connects_.contains(conn));
    connects_.insert(conn);    
}
    
void TcpServer::removeConnection(TcpConnectionPtr conn)
{
    assert(conn->loop()->isInLoopThread()); 
    
    // loop_->submit([this,conn](){//////////////////connects_是公共数据结构 会导致死锁，accept线程向IO池提交连接，IO池向accept线程移除连接
    //     assert(connects_.find(conn)!=connects_.end());
    //     connects_.erase(conn);
    // });
    
    
    connects_.erase(conn);
}
}    
}