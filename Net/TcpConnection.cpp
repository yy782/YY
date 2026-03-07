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
void TcpConnection::send(const char* message,size_t len) // message是栈变量?
{
    if(Connstatus_!=ConnectStatus::Connected)return;
    auto loop=handler_.get_loop();

    loop->submit([this,message,len](){
        sendInLoop(message,len);
    });
    

}
void TcpConnection::sendInLoop(const char* message,size_t len)
{
    if(!handleBackpressureBeforeSend(len))  
    {
        return;  // 这里数据丢失 
    }

    EventLoop* loop=handler_.get_loop();
    auto n=sockets::send(handler_.get_fd(),message,len,MSG_NOSIGNAL);
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
void TcpConnection::handleRead()
{
    assert(SmessageCallBack_);

    if(!handleBackpressureBeforeRead())
    {
        return;
    }
    auto n=sockets::recv(handler_.get_fd(),RecvBuffer_.append(),RecvBuffer_.get_writable_size(),0);
    if(n>0)
    {
        RecvBuffer_.append(n);
        SmessageCallBack_(shared_from_this(),&RecvBuffer_);
    }
    else if(n==0)
    {
        handleClose();
    }
    else
    {
        handleError();
    }    
    if(n>0)handleBackpressureAfterRead(n);
}
void TcpConnection::handleWrite()
{
    ssize_t len=static_cast<ssize_t>(SendBuffer_.get_readable_size());
    auto fd=handler_.get_fd();
    ssize_t n=sockets::send(fd,SendBuffer_.peek(),len,MSG_NOSIGNAL);
    if(n==len)
    {
        if(Connstatus_==ConnectStatus::DisConnected)
        {
            sockets::shutdown(fd,SHUT_WR);
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
    }
    else if(n>=0&&n<len)
    {
        if(!handler_.isWriting())
        {
            handler_.setWriting();
            handler_.get_loop()->update_listen(&handler_);
        }
        SendBuffer_.append(n);
    }
    else 
    {
        handleError();
    }
    if(n>0)
    {
        handleBackpressureAfterWrite();
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

bool TcpConnection::handleBackpressureBeforeSend(size_t len)
{
    size_t currentSize=SendBuffer_.get_readable_size();
    size_t newSize=currentSize+len;
    
    if(newSize>=BackpressureConfig::highWaterMark)
            SendbpState_=BackpressureState::kHighWater;        

    if(SendbpState_==BackpressureState::kHighWater)
    {
        switch(SendbpStrategy_)
        {
        case BufferBackpressureStrategy::kDiscard: //丢弃新数据
            LOG_CLIENT_WARN("Discarding "<<len<<" bytes due to backpressure");
            return false;
            
            break;
            
        case BufferBackpressureStrategy::kCloseConnection: //断开连接
        
            LOG_CLIENT_WARN("Closing connection due to high watermark");
            disconnect();
            return false;
            
            break;
        case BufferBackpressureStrategy::kThrottle: //限速发送
            // TODO
            
            break;
        case BufferBackpressureStrategy::kPass:   //不处理
        default:
            break;
        }   
        return true;     
    }

    
    return true;
}
void TcpConnection::handleBackpressureAfterWrite()
{
    if(SendBuffer_.get_readable_size()<=BackpressureConfig::lowWaterMark)
        SendbpState_=BackpressureState::kNormal;
}

bool TcpConnection::handleBackpressureBeforeRead()
{
    if(RecvbpState_==BackpressureState::kHighWater)
    {
        switch(SendbpStrategy_)
        {
        case BufferBackpressureStrategy::kDiscard: //丢弃新数据
            LOG_CLIENT_WARN("Discarding "<<" bytes due to backpressure");
            return false;
            
            break;
            
        case BufferBackpressureStrategy::kCloseConnection: //断开连接
        
            LOG_CLIENT_WARN("Closing connection due to high watermark");
            disconnect();
            return false;
            
            break;
        case BufferBackpressureStrategy::kBackoff: 
            // TODO
            
            break;
        case BufferBackpressureStrategy::kPass:   //不处理
        default:
            break;
        }   
        return true;     
    }

    
    return true;
}
void TcpConnection::handleBackpressureAfterRead(size_t len)
{
    if(RecvBuffer_.get_readable_size()>BackpressureConfig::lowWaterMark)
        RecvbpState_=BackpressureState::kNormal;
    else if(RecvBuffer_.get_readable_size()>BackpressureConfig::highWaterMark)
        RecvbpState_=BackpressureState::kHighWater;    

}
}
}