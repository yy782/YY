#include "TcpConnection.h"
#include "sockets.h"
#include <vector>


#include <atomic>
#include "../Common/Assert.h"

namespace yy
{
namespace net
{


const int MaxReadNum=100;
TcpConnection::TcpConnection(int fd,const Address& addr,EventLoop* loop,Event events):
fd_(fd),
addr_(addr),
loop_(loop),
Connstatus_(ConnectStatus::Connected),
handler_(fd,loop,std::string(addr.sockaddrToString()+" AddListen"),events),
isET(events.has(LogicEvent::Edge)?true:false)
{
    if(isET)
    {
        assert(sockets::isNonBlocking(fd));
        handler_.setReadCallBack([this]()
        {
            handleETRead();
        });         
    }
    else 
    {
        handler_.setReadCallBack([this]()
        {
            handleRead();
        });        
    }

    handler_.setExceptCallBack([this]()
    {
        handleException();
    });
    handler_.setWriteCallBack([this]()
    {
        handleWrite();
    });
    handler_.setCloseCallBack([this]()
    {
        handleClose();
    });
    handler_.setErrorCallBack([this]()
    {
        handleError();
    });
    // loop_->DelayedExecution<false>([this](){
    //     handler_.tie(shared_from_this()); //对象没创建不能shared_from_this()/////////////////////////////////////////
    // });
}

TcpConnection::~TcpConnection()
{
    if(RecvBuffer_.readable_size()!=0)
    {
        LOG_TCP_WARN("had data not read!");
    }
    if(fd_!=-1)
        sockets::close(fd_);
}
TcpConnection::TcpConnection(Event events):
Connstatus_(ConnectStatus::Connecting),
handler_(events),
isET(events.has(LogicEvent::Edge)?true:false)
{
    if(isET)
    {
        handler_.setReadCallBack([this]()
        {
            handleETRead();
        });         
    }
    else 
    {
        handler_.setReadCallBack([this]()
        {
            handleRead();
        });        
    }

    handler_.setExceptCallBack([this]()
    {
        handleException();
    });
    handler_.setWriteCallBack([this]()
    {
        handleWrite();
    });
    handler_.setCloseCallBack([this]()
    {
        handleClose();
    });
    handler_.setErrorCallBack([this]()
    {
        handleError();
    });    
}
void TcpConnection::init(int fd,const Address& addr,EventLoop* loop)
{
    assert(!handler_.event().has(LogicEvent::Edge)|| 
        !sockets::isNonBlocking(fd));
    assert(fd_==-1);
    assert(loop_==nullptr);
    assert(Connstatus_==ConnectStatus::Connecting);
    Connstatus_=ConnectStatus::Connected;
    fd_=fd;
    loop_=loop;
    addr_=addr;
    handler_.init(fd,loop,addr.sockaddrToString());
}
void TcpConnection::reset()///////////////////////////////////////////
{
    sockets::close(fd_);
    fd_=-1;
    loop_=nullptr;
    //address不重置，保留最后一次连接的地址信息
    //     if(ScloseCallBack_)ScloseCallBack_(shared_from_this());
    // assert(destructCallBack_);
    // loop_->DelayedExecution<true>([this](){
    //    destructCallBack_(shared_from_this()); 
    // },std::string("TcpCon::handleClose "+addr_.sockaddrToString()+" destructCallBack_"));

    RecvBuffer_.reset();
    SendBuffer_.reset();
    Connstatus_=ConnectStatus::Connecting;
}
void TcpConnection::setEvent(Event e)
{   
    handler_.set_event(e);  
    if(e.has(LogicEvent::Edge)&&!isET)
    {
        handler_.setReadCallBack([this]()
        {
            handleETRead();
        });
        isET=true;
    }
    else if(!e.has(LogicEvent::Edge)&&isET)
    {
        handler_.setReadCallBack([this]()
        {
            handleRead();
        });
        isET=false;            
    }
}
// void TcpConnection::ConnectSuccess()
// {
//     // add();
//     assert(Connstatus_==ConnectStatus::Connecting);
//     Connstatus_=ConnectStatus::Connected;
//     handler_.tie(shared_from_this());
//     if(SconnectCallback_)
//     {
//         SconnectCallback_(shared_from_this());
//     }
// }
void TcpConnection::disconnect()
{
    if(SendBuffer_.readable_size()==0)
    {
        loop_->submit([c=weak_from_this()]()
        {
            auto con=c.lock();
            if(con)
                con->disconnectInLoop();
        },std::string("TcpCon::disconnect:"+addr_.sockaddrToString()));
    }
    else 
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
        },std::string("TcpCon::disconnect handlerSetWriting "+addr_.sockaddrToString()));
        
    }
}  
void TcpConnection::disconnectInLoop()
{
    assert(loop_->isInLoopThread());
    if(Connstatus_==ConnectStatus::Connected)
    {
        Connstatus_=ConnectStatus::DisConnecting;
        if(SendBuffer_.readable_size()==0)
        {
            sockets::shutdown(fd_,SHUT_WR);  
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
        },std::string("TcpConn::send"+addr_.sockaddrToString()));
    }      
}
void TcpConnection::send(std::string&& message)
{
    if(Connstatus_!=ConnectStatus::Connected)return;
    
    loop_->submit([c=weak_from_this(),msg=std::move(message)]()
    {
        auto con=c.lock();
        if(con)
            con->sendInLoop(msg.c_str(),msg.size());
    },std::string("TcpConn::send"+addr_.sockaddrToString()));     /////////////////////////////////////////////////////////////////////////////
}
void TcpConnection::sendOutput()
{
    loop_->submit([c=weak_from_this()]()
    {
        auto con=c.lock();
        if(con)
        {
            if(!con->handler_.isWriting())
            {
                con->handler_.setWriting();
            }              
        }
    },std::string("TcpConn::sendOutput()"+addr_.sockaddrToString()));
}
void TcpConnection::sendInLoop(const char* message,size_t len)
{
    // TimeStamp<HighPrecision> Now=TimeStamp<HighPrecision>::now();
    // LOG_SYSTEM_DEBUG("semdInLoop happen"<<" time:"<<Now.nowToString());    

    assert(loop_->isInLoopThread());
    ssize_t n=sockets::send(handler_.fd(),message,len,0);
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
void TcpConnection::handleETRead()
{

    assert(loop_->isInLoopThread());
    int ReadNum=0;
    while(ReadNum<MaxReadNum)
    {
        auto n=RecvBuffer_.appendFormFd(handler_.fd());
        if(n>0)
        {
            //updateWaterMark();
            SmessageCallBack_(shared_from_this());
            ++ReadNum;
//             handleBackpressureAfterRead();
        }  
        else if(n==0)
        {
            // ++CloseNum;
            // assert(CloseNum<2);
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
    loop_->DelayedExecution<true>([c=weak_from_this()]()
    {
        auto con=c.lock();
        if(con)
        {
            con->handleETRead();
        }
    },std::string("TcpConn::handleETRead()"+addr_.sockaddrToString()));

}
void TcpConnection::handleRead()
{
    assert(loop_->isInLoopThread());
    assert(SmessageCallBack_);
    auto n=RecvBuffer_.appendFormFd(handler_.fd());
    if(n>0)
    {
        //updateWaterMark();
        SmessageCallBack_(shared_from_this());
        // handleBackpressureAfterRead();
        // LOG_TCP_DEBUG("读取了"<<n<<"字节，buffer可读:"<<RecvBuffer_.readable_size()<<"字节");
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
void TcpConnection::handleWrite()
{
    assert(loop_->isInLoopThread());
    ssize_t len=static_cast<ssize_t>(SendBuffer_.readable_size());
    auto fd=handler_.fd();
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
        //updateWaterMark();
//         handleBackpressureAfterSend();            
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
    handler_.removeListen(std::string("TcpCon::handleClose "+addr_.sockaddrToString()));
    Connstatus_=ConnectStatus::DisConnected;
    if(ScloseCallBack_)ScloseCallBack_(shared_from_this());
    assert(destructCallBack_);
    loop_->DelayedExecution<true>([this](){
       destructCallBack_(shared_from_this()); ///////////////////////一定要放到最后  ///////////////////////////////////////
    },std::string("TcpCon::handleClose "+addr_.sockaddrToString()+" destructCallBack_"));
}
void TcpConnection::handleError()   
{
    LOG_TCP_DEBUG(addr_.sockaddrToString()<<" handleError");

    if(errno==EPIPE||errno==ECONNRESET)
    {
        return;
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
        auto n=sockets::recv(handler_.fd(),RecvBuffer_.BeginWrite(),RecvBuffer_.writable_size(),MSG_OOB);   
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
// void TcpConnection::handleBackpressureAfterSend()
// {
//     if(!BackpressureAfterSend_)return;
//     else
//         return BackpressureAfterSend_(shared_from_this());
// }        
// void TcpConnection::handleBackpressureAfterRead()
// {
//     if(!BackpressureAfterRead_)return;
//     else 
//         return BackpressureAfterRead_(shared_from_this());
// }

// void TcpConnection::updateWaterMark()
// {
//     if(RecvBuffer_.readable_size()>BackpressureConfig::highWaterMark)
//         RecvbpState_=BackpressureState::kHighWaterMark;
//     else if(RecvBuffer_.readable_size()<BackpressureConfig::lowWaterMark)
//         RecvbpState_=BackpressureState::kNormal;
        
//     if(SendBuffer_.readable_size()>BackpressureConfig::highWaterMark)
//         SendbpState_=BackpressureState::kHighWaterMark;
//     else if(SendBuffer_.readable_size()<BackpressureConfig::lowWaterMark)
//         SendbpState_=BackpressureState::kNormal;  

// }
// template<>
// void handleAfterSend<BufferBackpressureStrategy::kDiscard>(TcpConnectionPtr conn)
// {
//     if(conn->getSendBufferState() == TcpConnection::BackpressureState::kHighWaterMark)
//         return;
//     else
//         return;    
// }

// template<>
// void handleAfterSend<BufferBackpressureStrategy::kCloseConnection>(TcpConnectionPtr conn)
// {
//     if(conn->getSendBufferState() == TcpConnection::BackpressureState::kHighWaterMark)
//     {
//         if(conn->isConnected())
//             conn->disconnect();
//         return;        
//     }
//     else 
//         return;
// }

// template<>
// void handleAfterSend<BufferBackpressureStrategy::kPass>(TcpConnectionPtr)
// {
//     return;
// }

// template<>
// void handleAfterRecv<BufferBackpressureStrategy::kDiscard>(TcpConnectionPtr conn)
// {
//     if(conn->getRecvBufferState() == TcpConnection::BackpressureState::kHighWaterMark)
//         return;
//     else
//         return;    
// }

// template<>
// void handleAfterRecv<BufferBackpressureStrategy::kCloseConnection>(TcpConnectionPtr conn)
// {
//     if(conn->getRecvBufferState() == TcpConnection::BackpressureState::kHighWaterMark)
//     {
//         if(conn->isConnected())
//             conn->disconnect();
//         return;        
//     }
//     else 
//         return;
// }

// template<>
// void handleAfterRecv<BufferBackpressureStrategy::kPass>(TcpConnectionPtr)
// {
//     return;
// }

// template<>
// void handleAfterRecv<BufferBackpressureStrategy::kBackoff>(TcpConnectionPtr conn)
// {
//     if(conn->getRecvBufferState() == TcpConnection::BackpressureState::kHighWaterMark)
//     {
//         if(conn->isReadingAndExcept())
//         {
//             conn->cancelReadingAndExcept();
//         }
//         return;        
//     }
//     else
//     {
//         if(!conn->isReadingAndExcept())
//         {
//             conn->setReadingAndExcept();
//         }
//         return;
//     }    
// }
}
}