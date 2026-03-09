#include "TcpConnection.h"
#include "sockets.h"
#include <vector>
#include "EventLoop.h"
namespace yy
{
namespace net
{

TcpConnection::TcpConnection(int fd,const Address& addr,EventLoop* loop):
addr_(addr),
handler_(fd,loop),
Connstatus_(ConnectStatus::DisConnected)
{

    handler_.setReadCallBack(std::bind(&TcpConnection::handleRead,this));
    handler_.setWriteCallBack(std::bind(&TcpConnection::handleWrite,this));
    handler_.setCloseCallBack(std::bind(&TcpConnection::handleClose,this));
    handler_.setErrorCallBack(std::bind(&TcpConnection::handleError,this));
}
TcpConnection::~TcpConnection()
{
    if(RecvBuffer_.get_readable_size()!=0)
    {
        LOG_CLIENT_WARN("had data not read!");
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
    if(Connstatus_!=ConnectStatus::Connected)return;
    auto loop=handler_.get_loop();

    loop->submit([this,message,len](){
        sendInLoop(message,len);
    });        
    

}
void TcpConnection::send()
{
    if(!handler_.isWriting())
    {
        handler_.setWriting();
    }
}
void TcpConnection::sendInLoop(const char* message,size_t len)
{
    if(!handleBackpressureBeforeSend())
        return;
    EventLoop* loop=handler_.get_loop();

    if(codec_)
    {
        codec_->encode(string_view(message,len),SendBuffer_);
        if(!handler_.isWriting())
        {
            handler_.setWriting();
            loop->update_listen(&handler_);
        }
    }
    else 
    {
        ssize_t n=sockets::sendET(handler_.get_fd(),message,len,MSG_NOSIGNAL);
        if(n>=0&&n<static_cast<ssize_t>(len))
        {
            SendBuffer_.append(message+n,len-n);
            if(!handler_.isWriting())
            {
                handler_.setWriting();
                loop->update_listen(&handler_);
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
    if(!handleBackpressureBeforeRead())
        return;
    auto n=sockets::recvET(handler_.get_fd(),RecvBuffer_.append(),RecvBuffer_.get_writable_size(),0);
    if(n>0)
    {
        RecvBuffer_.append(n);
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
    ssize_t n=sockets::send(fd,SendBuffer_.peek(),len,MSG_NOSIGNAL);
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
                handler_.caneclWriting();
                handler_.get_loop()->update_listen(&handler_);
            }
            
        }
        SendBuffer_.append(n);
        updateWaterMark();
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
    LOG_CLIENT_ERROR("have errno");
    if(SerrorCallBack_)SerrorCallBack_(shared_from_this());
}

bool TcpConnection::handleBackpressureBeforeSend()
{
    if(!BackpressureBeforeSend_)return true;
    else
        return BackpressureBeforeSend_(shared_from_this());
}
bool TcpConnection::handleBackpressureBeforeRead()
{
    if(!BackpressureBeforeRead_)return true;
    else 
        return BackpressureBeforeRead_(shared_from_this());
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
bool handleBeforeSend<BufferBackpressureStrategy::kDiscard>(TcpConnection::TcpConnectionPtr conn)
{
    if(conn->getSendBufferState() == TcpConnection::BackpressureState::kHighWaterMark)
        return false;
    else
        return true;    
}

template<>
bool handleBeforeSend<BufferBackpressureStrategy::kCloseConnection>(TcpConnection::TcpConnectionPtr conn)
{
    if(conn->getSendBufferState() == TcpConnection::BackpressureState::kHighWaterMark)
    {
        if(conn->isConnected())
            conn->disconnect();
        return false;        
    }
    else 
        return true;
}

template<>
bool handleBeforeSend<BufferBackpressureStrategy::kPass>(TcpConnection::TcpConnectionPtr)
{
    return true;
}

template<>
bool handleBeforeRecv<BufferBackpressureStrategy::kDiscard>(TcpConnection::TcpConnectionPtr conn)
{
    if(conn->getRecvBufferState() == TcpConnection::BackpressureState::kHighWaterMark)
        return false;
    else
        return true;    
}

template<>
bool handleBeforeRecv<BufferBackpressureStrategy::kCloseConnection>(TcpConnection::TcpConnectionPtr conn)
{
    if(conn->getRecvBufferState() == TcpConnection::BackpressureState::kHighWaterMark)
    {
        if(conn->isConnected())
            conn->disconnect();
        return false;        
    }
    else 
        return true;
}

template<>
bool handleBeforeRecv<BufferBackpressureStrategy::kPass>(TcpConnection::TcpConnectionPtr)
{
    return true;
}

template<>
bool handleBeforeRecv<BufferBackpressureStrategy::kBackoff>(TcpConnection::TcpConnectionPtr conn)
{
    if(conn->getRecvBufferState() == TcpConnection::BackpressureState::kHighWaterMark)
    {
        if(conn->getHandler()->isReadingAndExcept())
        {
            conn->getHandler()->cancelReadingAndExcept();
        }
        return false;        
    }
    else
    {
        if(!conn->getHandler()->isReadingAndExcept())
        {
            conn->getHandler()->setReadingAndExcept();
        }
        return true;
    }    
}
}
}