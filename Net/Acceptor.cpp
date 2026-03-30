#include "Acceptor.h"
#include <vector>
#include "TcpServer.h"
namespace yy
{
namespace net
{
Acceptor::Acceptor(const Address& addr,EventLoop* loop,TcpServer* Ser,int id)://主线程构造
addr_(addr),
id_(id),
loop_(loop),
handler_(sockets::createTcpSocketOrDie(addr.family()),loop_,std::string("AcceptorID:"+std::to_string(id)+" AddListen"),Event(LogicEvent::Read|LogicEvent::Edge)),
idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)),
Ser_(Ser)
{
    assert(Ser_);
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
void Acceptor::NewConnection(int fd,const Address& addr)
{
    assert(loop_->isInLoopThread());
    auto loop=Ser_->NextLoop();
    assert(SconnectCallBack_);
    assert(loop);
    auto conn=SconnectCallBack_(fd,addr,loop);
    conn->setDestructorCallBack([this](TcpConnectionPtr con)
    {
        removeConnection(con);
    });
    assert(!connects_.contains(conn));
    connects_.insert(conn);     
}
void Acceptor::removeConnection(TcpConnectionPtr conn)
{
    assert(conn->loop()->isInLoopThread()); 
    // loop_->submit([this,conn](){//////////////////connects_是公共数据结构 会导致死锁，accept线程向IO池提交连接，IO池向accept线程移除连接
    //     assert(connects_.find(conn)!=connects_.end());
    //     connects_.erase(conn);
    // });
    connects_.erase(conn);
}
Acceptor::~Acceptor() // 确保acceptor的生命周期比loop长
{
//   handler_.disableAll();
//   handler_.removeListen();
  sockets::close(handler_.fd());
}
   
}    
}