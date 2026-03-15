#include "TcpConnection.h"
#include "sockets.h"
#include <vector>

namespace yy
{
namespace net
{

TcpConnection::TcpConnection(int fd,const Address& addr,EventLoop* loop):
addr_(addr),
fd_(fd),

Connstatus_(ConnectStatus::DisConnected),
handler_(fd,loop)
{

    handler_.setReadCallBack(std::bind(&TcpConnection::handleRead,this));
    handler_.setExceptCallBack(std::bind(&TcpConnection::handleException,this));
    handler_.setWriteCallBack(std::bind(&TcpConnection::handleWrite,this));
    handler_.setCloseCallBack(std::bind(&TcpConnection::handleClose,this));
    handler_.setErrorCallBack(std::bind(&TcpConnection::handleError,this));
}
TcpConnection::TcpConnection(int fd,const Address& addr):
addr_(addr),
fd_(fd),
Connstatus_(ConnectStatus::DisConnected),
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
    if(Connstatus_!=ConnectStatus::DisConnected)
    {
        disconnect();
    }
}
void TcpConnection::disconnect()
{
    Connstatus_=ConnectStatus::DisConnected;
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
    sockets::shutdown(handler_.get_fd(),SHUT_WR);   
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

    if(codec_)
    {
        codec_->encode(string_view(message,len),SendBuffer_);
        if(!handler_.isWriting())
        {
            handler_.setWriting();
            
        }
    }
    else 
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

}
void TcpConnection::handleRead()
{
    assert(SmessageCallBack_);
     
    auto n=sockets::recvAuto(handler_.get_fd(),RecvBuffer_.BeginWrite(),RecvBuffer_.get_writable_size(),0);
    if(n>0)
    {
        RecvBuffer_.hasWritten(n);
        updateWaterMark();
        string_view data(string_view(RecvBuffer_.peek(),RecvBuffer_.get_readable_size()));
        string_view msg;
        if(codec_)
        {
            int ret=codec_->tryDecode(data,msg);
            if(ret>0)
            {
                SmessageCallBack_(shared_from_this(),msg);
            }
            else if(ret<0)
            {
                handleError();
            }
        }
        else 
        {
          SmessageCallBack_(shared_from_this(),data);  
        }
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
    ssize_t n=sockets::sendAuto(fd,SendBuffer_.peek(),len,0);
    if(n>0)
    {
        if(n==len)
        {
            if(Connstatus_==ConnectStatus::DisConnected)
            {
                sockets::shutdown(fd,SHUT_WR);
            }
        }
        else 
        {
            if(handler_.isWriting())
            {
                handler_.cancelWriting();
                
            }
            
        }
        SendBuffer_.hasWritten(n);
        updateWaterMark();
        handleBackpressureAfterSend();
    }
    else 
    {
        handleError();
    }
    if(SwriteCompleteCallBack_)SwriteCompleteCallBack_(shared_from_this());
}
void TcpConnection::handleClose()
{

    auto loop=handler_.get_loop();
    assert(loop);
    loop->remove_listen(&handler_);

    if(ScloseCallBack_)ScloseCallBack_(shared_from_this());
    assert(Connstatus_==ConnectStatus::Connected);
    Connstatus_=ConnectStatus::DisConnected;
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