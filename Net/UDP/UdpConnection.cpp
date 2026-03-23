
#include "UdpConnection.h"
#include "../sockets.h"
#include "../PollerType.h"
#include "../Timer.h"
namespace yy 
{
namespace net 
{
// 创建客户端UDP连接（connect方式，固定对端）
UdpConnection::UdpConnectionPtr UdpConnection::createConnection(
    EventLoop* loop, const char* host, unsigned short port) 
{
    
    Address addr(host, port);
    int fd=sockets::createUdpSocketOrDie(addr.get_family());
    sockets::set_CloseOnExec(fd);
    
    sockets::connect(fd, addr);

    UdpConnectionPtr conn(new UdpConnection(loop,fd,Address()));
    conn->peer_=addr;
    conn->isServer_=false;
    return conn;
}
UdpConnection::UdpConnectionPtr UdpConnection::createServer(
    EventLoop* loop, const  char* bindHost, unsigned short port) 
    {
    
    Address addr(bindHost, port);
    int fd=sockets::createUdpSocketOrDie(addr.get_family());

    sockets::bindOrDie(fd,addr);
    UdpConnectionPtr conn(new UdpConnection(loop,fd,addr));
    conn->isServer_=true;
    return conn;
}

UdpConnection::UdpConnection(EventLoop* loop, int fd, const Address& local):
    loop_(loop), 
    handler_(fd, loop,local.sockaddrToString().c_str()), 
    local_(local), 
    closed_(false), 
    isServer_(false) 
{
    
    handler_.setReadCallBack([this](){handleRead();});
    handler_.setErrorCallBack([this](){handleError();});
    handler_.setReading();
     
}

UdpConnection::~UdpConnection()
{
    close();
}

void UdpConnection::onMessage(UdpMessageCallBack cb) 
{
    messageCallback_=std::move(cb);
}

void UdpConnection::onError(UdpErrorCallBack cb) 
{
    errorCallback_=std::move(cb);
}

// void UdpConnection::send(const char* buf, size_t len) {
//     assert(!isServer_);
//     sendInLoop(buf,len,nullptr);
// }

// void UdpConnection::send(const std::string& s) {
//     send(s.data(), s.size());
// }

// void UdpConnection::send(const char* s) 
// {
//     send(s, strlen(s));
// }

void UdpConnection::sendTo(const char* buf, size_t len, const Address* dest) {

    sendTo(std::string(buf, len),dest);
}

void UdpConnection::sendTo(const std::string& s,const Address* dest) 
{
    if (closed_) 
    {
        LOG_WARN("UDP connection closed");
        return;
    }

    loop_->submit([this,buf=std::move(s),dest]() 
    {
        sendInLoop(buf.data(), buf.size(),dest);
    });    
}
void UdpConnection::startHeartbeat(LTimeInterval interval,LTimeInterval MaxidleTime)
{
    MaxidleTime_=MaxidleTime;
    checkUdpAlive_=true;
    LTimer timer([this](){
        sendTo("heartbeat");
        lastSendTimestamp_=LTimeStamp::now();
    },interval,BaseTimer::FOREVER);
    int timerfd=sockets::createTimerFdOrDie(CLOCK_MONOTONIC,TFD_CLOEXEC|TFD_NONBLOCK);
    sockets::timerfd_settime(timerfd,0,timer);
    timerHandler_.init(timerfd,loop_,"HeartBearHandler");
    timerHandler_.setReadCallBack([this](){
        lastRecvTimestamp_=LTimeStamp::now();
        if(lastRecvTimestamp_-lastSendTimestamp_>MaxidleTime_)
        {
            ispeerAlive_=false;
        }
    });
    timerHandler_.setReading();
     
}
void UdpConnection::close() 
{
    assert(!closed_);
    loop_->remove_listen(&handler_);
    closed_=true;
}

Address UdpConnection::getPeerAddr()const 
{
    assert(!isServer_);
    return peer_;
}

void UdpConnection::handleRead() {
    char buf[kUdpPacketSize];
  
    struct sockaddr_storage peerAddr;

    ssize_t n=sockets::recvfromAuto(handler_.get_fd(), buf, kUdpPacketSize, 0,peerAddr);


    messageCallback_(shared_from_this(),stringPiece(buf,n),peerAddr);
}

void UdpConnection::sendInLoop(const char* buf, size_t len, const Address* dest) {
    if (closed_) return;

    ssize_t n;
    if (dest) 
    {
        n=sockets::sendtoAuto(handler_.get_fd(), buf, len, 0,
                     *dest);
    }else 
    {
       
        n=sockets::sendtoAuto(handler_.get_fd(), buf, len, 0,
                     peer_);
    }
    if (n<0)
    {
        if(errorCallback_)errorCallback_(shared_from_this());
    }

}

void UdpConnection::handleError() 
{
    LOG_ERRNO(errno);
    if (errorCallback_) 
    {
        errorCallback_(shared_from_this());
    }
    
    // UDP错误通常不会致命，但可以根据情况决定是否关闭
    // 这里选择不自动关闭，让用户决定
}
}    
}