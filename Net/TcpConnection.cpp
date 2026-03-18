#include "TcpConnection.h"
#include "sockets.h"
#include <vector>

namespace yy
{
namespace net
{
TcpConnection::TcpConnection():
Connstatus_(ConnectStatus::Connecting)
{

}
void TcpConnection::init(int fd,const Address& addr,EventLoop* loop)
{
    handler_.init(fd,loop);
    addr_=addr;
    handler_.setReadCallBack(std::bind(&TcpConnection::handleRead,this));
    handler_.setExceptCallBack(std::bind(&TcpConnection::handleException,this));
    handler_.setWriteCallBack(std::bind(&TcpConnection::handleWrite,this));
    handler_.setCloseCallBack(std::bind(&TcpConnection::handleClose,this));
    handler_.setErrorCallBack(std::bind(&TcpConnection::handleError,this));
}
TcpConnection::TcpConnection(int fd,const Address& addr):
addr_(addr),
fd_(fd),
Connstatus_(ConnectStatus::Connecting),
handler_() // 监听的loop不确定，延迟初始化
{

    handler_.setReadCallBack(std::bind(&TcpConnection::handleRead,this));
    handler_.setExceptCallBack(std::bind(&TcpConnection::handleException,this));
    handler_.setWriteCallBack(std::bind(&TcpConnection::handleWrite,this));
    handler_.setCloseCallBack(std::bind(&TcpConnection::handleClose,this));
    handler_.setErrorCallBack(std::bind(&TcpConnection::handleError,this));
}
TcpConnection::~TcpConnection()
{
    if(RecvBuffer_.get_readable_size()!=0)
    {
        LOG_TCP_WARN("had data not read!");
    }
    
}
void TcpConnection::disconnect()
{
  

    if(SendBuffer_.get_readable_size()==0)
    {
        EventLoop* loop=handler_.get_loop();
        loop->submit([this](){
            disconnectInLoop();
        });
    }
}  
void TcpConnection::disconnectInLoop()
{
    if(Connstatus_==ConnectStatus::Connected)
    {
        Connstatus_=ConnectStatus::DisConnecting;
        sockets::shutdown(handler_.get_fd(),SHUT_WR);   
        if(handler_.isWriting())
        {
            handler_.cancelWriting();
        }      
    }
}
void TcpConnection::send(const char* message,size_t len)
{
    send(std::string(message,len));       
}
void TcpConnection::send(std::string&& message)
{
    if(Connstatus_!=ConnectStatus::Connected)return;
    auto loop=handler_.get_loop();
    loop->submit([this,msg=std::move(message)](){
        
        sendInLoop(msg.c_str(),msg.size());
    });     
}
void TcpConnection::sendOutput()
{
    if(!handler_.isWriting())
    {
        handler_.setWriting();
    }
}
void TcpConnection::sendInLoop(const char* message,size_t len)
{

    ssize_t n=sockets::sendAuto(handler_.get_fd(),message,len,0);
    if(n>=0&&n<static_cast<ssize_t>(len))
    {
        SendBuffer_.append(message+n,len-n);
        if(!handler_.isWriting())
        {
            handler_.setWriting();
            
        }
    }
    else if(n<0)
    {
        handleError();
    }         
    

}
void TcpConnection::handleRead()
{
    assert(SmessageCallBack_);
     
    auto n=sockets::recvAuto(handler_.get_fd(),RecvBuffer_.BeginWrite(),RecvBuffer_.get_writable_size(),0);
    if(n>0)
    {
        RecvBuffer_.hasWritten(n);
        updateWaterMark();
        SmessageCallBack_(shared_from_this());  

        handleBackpressureAfterRead();
    }  
    else if(n==0)
    {
        handleClose();
    }
    else
    {
        handleError();
    }    
}
void TcpConnection::handleWrite()
{

    ssize_t len=static_cast<ssize_t>(SendBuffer_.get_readable_size());
    auto fd=handler_.get_fd();
    if(len==0)return;
    ssize_t n=sockets::sendAuto(fd,SendBuffer_.peek(),len,0);
    if(n>0)
    {
        if(n==len)
        {
            if(Connstatus_==ConnectStatus::DisConnecting)
            {
                sockets::shutdown(fd,SHUT_WR);
            }
            if(handler_.isWriting())
            {
                handler_.cancelWriting();
            }
        }
        SendBuffer_.consume(n);
        updateWaterMark();
        handleBackpressureAfterSend();            
    }
    else 
    {
        handleError();
    }        
    


}
void TcpConnection::handleClose()
{

    assert(Connstatus_==ConnectStatus::Connected||Connstatus_==ConnectStatus::DisConnecting);
    auto loop=handler_.get_loop();
    assert(loop);
    
    handler_.disableAll();
    loop->remove_listen(&handler_);

    

    Connstatus_=ConnectStatus::DisConnected;

    if(ScloseCallBack_)ScloseCallBack_(shared_from_this());
}
void TcpConnection::handleError()   
{
    if(errno==EPIPE||errno==ECONNRESET)
    {
        handleClose();
    }
    else if (errno==ESHUTDOWN)
    {
        return;
    }
    else if(errno == EAGAIN || errno == EWOULDBLOCK)
    {
        return;
    } // sendOrrecv 函数的错误
    
    else if(SerrorCallBack_)SerrorCallBack_(shared_from_this()); //也许业务层需要errno
    
}
void TcpConnection::handleException()
{   
    auto n=sockets::recvAuto(handler_.get_fd(),RecvBuffer_.BeginWrite(),RecvBuffer_.get_writable_size(),MSG_OOB);
    if(n>0)
    {
        char oob_buf[1];
        if(SexceptCallBack_)SexceptCallBack_(shared_from_this(),oob_buf);          
    }  
    else if(n==0)
    {
        handleClose();
    }
    else
    {
        handleError();
    }  
}
void TcpConnection::handleBackpressureAfterSend()
{
    if(!BackpressureAfterSend_)return;
    else
        return BackpressureAfterSend_(shared_from_this());
}
void TcpConnection::handleBackpressureAfterRead()
{
    if(!BackpressureAfterRead_)return;
    else 
        return BackpressureAfterRead_(shared_from_this());
}

void TcpConnection::updateWaterMark()
{
    if(RecvBuffer_.get_readable_size()>BackpressureConfig::highWaterMark)
        RecvbpState_=BackpressureState::kHighWaterMark;
    else if(RecvBuffer_.get_readable_size()<BackpressureConfig::lowWaterMark)
        RecvbpState_=BackpressureState::kNormal;
        
    if(SendBuffer_.get_readable_size()>BackpressureConfig::highWaterMark)
        SendbpState_=BackpressureState::kHighWaterMark;
    else if(SendBuffer_.get_readable_size()<BackpressureConfig::lowWaterMark)
        SendbpState_=BackpressureState::kNormal;  

}
template<>
void handleAfterSend<BufferBackpressureStrategy::kDiscard>(TcpConnectionPtr conn)
{
    if(conn->getSendBufferState() == TcpConnection::BackpressureState::kHighWaterMark)
        return;
    else
        return;    
}

template<>
void handleAfterSend<BufferBackpressureStrategy::kCloseConnection>(TcpConnectionPtr conn)
{
    if(conn->getSendBufferState() == TcpConnection::BackpressureState::kHighWaterMark)
    {
        if(conn->isConnected())
            conn->disconnect();
        return;        
    }
    else 
        return;
}

template<>
void handleAfterSend<BufferBackpressureStrategy::kPass>(TcpConnectionPtr)
{
    return;
}

template<>
void handleAfterRecv<BufferBackpressureStrategy::kDiscard>(TcpConnectionPtr conn)
{
    if(conn->getRecvBufferState() == TcpConnection::BackpressureState::kHighWaterMark)
        return;
    else
        return;    
}

template<>
void handleAfterRecv<BufferBackpressureStrategy::kCloseConnection>(TcpConnectionPtr conn)
{
    if(conn->getRecvBufferState() == TcpConnection::BackpressureState::kHighWaterMark)
    {
        if(conn->isConnected())
            conn->disconnect();
        return;        
    }
    else 
        return;
}

template<>
void handleAfterRecv<BufferBackpressureStrategy::kPass>(TcpConnectionPtr)
{
    return;
}

template<>
void handleAfterRecv<BufferBackpressureStrategy::kBackoff>(TcpConnectionPtr conn)
{
    if(conn->getRecvBufferState() == TcpConnection::BackpressureState::kHighWaterMark)
    {
        if(conn->getHandler()->isReadingAndExcept())
        {
            conn->getHandler()->cancelReadingAndExcept();
        }
        return;        
    }
    else
    {
        if(!conn->getHandler()->isReadingAndExcept())
        {
            conn->getHandler()->setReadingAndExcept();
        }
        return;
    }    
}
}
}