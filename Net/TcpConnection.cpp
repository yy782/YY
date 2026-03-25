#include "TcpConnection.h"
#include "sockets.h"
#include <vector>


#include <atomic>


namespace yy
{
namespace net
{


const int MaxReadNum=100;
TcpConnection::TcpConnection(int fd,const Address& addr,EventLoop* loop,const char* name):
fd_(fd),
addr_(addr),
loop_(loop),
Connstatus_(ConnectStatus::Connecting),
handler_(fd,loop,name) 
{

    handler_.setReadCallBack(std::bind(&TcpConnection::handleRead,this));
    handler_.setExceptCallBack(std::bind(&TcpConnection::handleException,this));
    handler_.setWriteCallBack(std::bind(&TcpConnection::handleWrite,this));
    handler_.setCloseCallBack(std::bind(&TcpConnection::handleClose,this));
    handler_.setErrorCallBack(std::bind(&TcpConnection::handleError,this));
    //handler_.tie(shared_from_this()); 对象没创建不能shared_from_this()
}
TcpConnection::~TcpConnection()
{
    if(RecvBuffer_.get_readable_size()!=0)
    {
        LOG_TCP_WARN("had data not read!");
    }
    sockets::close(fd_);
}

void TcpConnection::disconnect()
{
    if(SendBuffer_.get_readable_size()==0)
    {
        loop_->submit([c=weak_from_this()](){
            auto con=c.lock();
            if(con)
                con->disconnectInLoop();
        });
    }else 
    {
        if(!handler_.isWriting())
        {
            handler_.setWriting();
        }          
    }
}  
void TcpConnection::disconnectInLoop()
{
    assert(loop_->isInLoopThread());
    if(Connstatus_==ConnectStatus::Connected)
    {
        Connstatus_=ConnectStatus::DisConnecting;
        if(SendBuffer_.get_readable_size()==0)
        {
           sockets::shutdown(handler_.get_fd(),SHUT_WR);  
        }
        else 
        {
            if(!handler_.isWriting())
            {
                handler_.setWriting();
            }
        }    
    }
}
void TcpConnection::send(const char* message,size_t len)
{
    

    if(loop_->isInLoopThread())
    {
        sendInLoop(message,len);
    }
    else 
    {
        loop_->submit([c=weak_from_this(),s=std::string(message,len)]()
        {
            auto con=c.lock();
            if(con)
            {
                con->sendInLoop(s.c_str(),s.size());
            }
        });
    }      
}
// void TcpConnection::send(const std::string& message)
// {
//     send(std::string(message));
// }
// void TcpConnection::send(std::string&& message)
// {
//     if(Connstatus_!=ConnectStatus::Connected)return;
    
//     loop_->submit([c=weak_from_this(),msg=std::move(message)](){
//         auto con=c.lock();
//         if(con)
//             con->sendInLoop(msg.c_str(),msg.size());
//     });     
// }
void TcpConnection::send(const std::string& message)
{
    send(std::string(message));
}
void TcpConnection::send(std::string&& message)
{
    if(Connstatus_!=ConnectStatus::Connected)return;
    
    loop_->submit([c=weak_from_this(),msg=std::move(message)](){
        auto con=c.lock();
        if(con)
            con->sendInLoop(msg.c_str(),msg.size());
    });     
}
// void TcpConnection::send(const std::string_view& msg)
// {
//     if(loop_->isInLoopThread())
//     {
//         sendInLoop(msg.data(),msg.size());
//     }
//     else 
//     {
//         loop_->submit([c=weak_from_this(),s=std::string(msg)]()
//         {
//             auto con=c.lock();
//             if(con)
//             {
//                 con->sendInLoop(s.c_str(),s.size());
//             }
//         });
//     }
// }


void TcpConnection::sendOutput()
{
    loop_->submit([c=weak_from_this()](){
        auto con=c.lock();
        if(con)
        {
            if(!con->handler_.isWriting())
            {
                con->handler_.setWriting();
            }              
        }
    });
}
void TcpConnection::sendInLoop(const char* message,size_t len)
{
    // TimeStamp<HighPrecision> Now=TimeStamp<HighPrecision>::now();
    // LOG_SYSTEM_DEBUG("semdInLoop happen"<<" time:"<<Now.nowToString());    

    assert(loop_->isInLoopThread());
    ssize_t n=sockets::send(handler_.get_fd(),message,len,0);
    // LOG_TCP_DEBUG("sendInLoop: 尝试发送"<<len<<"字节，实际发送"<<n<<"字节");
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
        if(errno == EAGAIN)
        {
            SendBuffer_.append(message,len);
            if(!handler_.isWriting())
            {
                handler_.setWriting();
            }            
        }
        else 
        {
            handleError();
        }     
    }         
}
void TcpConnection::handleETRead(ServicesReadCallback cb)
{

    assert(loop_->isInLoopThread());
    int ReadNum=0;
    while(ReadNum<MaxReadNum)
    {
        auto n=RecvBuffer_.appendFormFd(handler_.get_fd());
        if(n>0)
        {
            updateWaterMark();
            cb();  
            ++ReadNum;
            handleBackpressureAfterRead();
        }  
        else if(n==0)
        {

            handleClose();
            return;
        }
        else
        {   if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return;
            }
            else if(errno==EINTR)
            {
                continue;
            }
            else
            {
                handleError();
                return;
            }  
        }            
    }
    handler_.get_loop()->submit([c=weak_from_this()](){
        auto con=c.lock();
        if(con)
        {
            con->handleRead();
        }
    });

}
void TcpConnection::handleRead()
{
    assert(loop_->isInLoopThread());
    assert(!(SreadCallback_&&SmessageCallBack_));
    if(!SreadCallback_)
    {
        assert(SmessageCallBack_);
        auto n=RecvBuffer_.appendFormFd(handler_.get_fd());
        if(n>0)
        {

            updateWaterMark();
            SmessageCallBack_(shared_from_this());
            handleBackpressureAfterRead();
            // LOG_TCP_DEBUG("读取了"<<n<<"字节，buffer可读:"<<RecvBuffer_.get_readable_size()<<"字节");
        }  
        else if(n==0)
        {
            handleClose();
            return;
        }
        else
        {   if(errno == EAGAIN || errno == EWOULDBLOCK||errno==EINTR)
            {
                handleError();
                return;                
            }
        }
    }
    else 
    {
        SreadCallback_();
    }
            
}
void TcpConnection::handleWrite()
{
    assert(loop_->isInLoopThread());
    ssize_t len=static_cast<ssize_t>(SendBuffer_.get_readable_size());
    auto fd=handler_.get_fd();
    ssize_t n=sockets::send(fd,SendBuffer_.peek(),len,0);
    if(n>0)
    {
        if(n==len)
        {
            if(Connstatus_==ConnectStatus::DisConnecting)
            {
                sockets::shutdown(fd,SHUT_WR);
                // ++shutDownNum;
                // LOG_SYSTEM_DEBUG("shutDonwNum:"<<shutDownNum);
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
    assert(loop_->isInLoopThread());
    assert(Connstatus_==ConnectStatus::Connected||Connstatus_==ConnectStatus::DisConnecting); 
    handler_.removeListen();
    Connstatus_=ConnectStatus::DisConnected;
    if(ScloseCallBack_)ScloseCallBack_(shared_from_this());
}
void TcpConnection::handleError()   
{
    LOG_TCP_DEBUG(addr_.sockaddrToString()<<" handleError");

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
    while(true)
    {
        auto n=sockets::recv(handler_.get_fd(),RecvBuffer_.BeginWrite(),RecvBuffer_.get_writable_size(),MSG_OOB);
        if(n>0)
        {
            char oob_buf[1];
            if(SexceptCallBack_)SexceptCallBack_(shared_from_this(),oob_buf);          
        }  
        else if(n==0)
        {
            handleClose();
            break;
        }
        else if(errno== EINTR)
        {
            continue;
        }
        else 
        {
            handleError();
            break;
        }        
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