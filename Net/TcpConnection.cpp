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
status_(Status::DisConnected)
{

    handler_.setReadCallBack(std::bind(&TcpConnection::handleRead,this));
    handler_.setWriteCallBack(std::bind(&TcpConnection::handleWrite,this));
    handler_.setCloseCallBack(std::bind(&TcpConnection::handleClose,this));
    handler_.setErrorCallBack(std::bind(&TcpConnection::handleError,this));
}
TcpConnection::~TcpConnection()
{
    if(readBuffer_.get_readable_size()!=0)
    {
        LOG_CLIENT_WARN("had data not read!");
    }
}
void TcpConnection::disconnect()
{
    status_=Status::DisConnected;
    if(writeBuffer_.get_readable_size()==0)
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
// TcpConnection::CharContainer TcpConnection::recv()
// {
//     Recvlocker_.lock();
//     auto n=sockets::recv(handler_.get_fd(),readBuffer_.append(),readBuffer_.get_writable_size(),0);
//     assert(n>=0);

//     readBuffer_.move_write_index(n);
//     auto str=readBuffer_.retrieve();

//     Recvlocker_.unlock();
//     return str;    
// }
void TcpConnection::send(const char* message,size_t len)
{
    auto loop=handler_.get_loop();
    if(!loop->isInLoopThread())
    {
        loop->submit([this,message,len](){
            sendInLoop(message,len);
        });
    }

}
void TcpConnection::sendInLoop(const char* message,size_t len)
{
    EventLoop* loop=handler_.get_loop();
    auto n=sockets::send(handler_.get_fd(),message,len,MSG_NOSIGNAL);
    if(n>=0&&n<static_cast<ssize_t>(len))
    {
        writeBuffer_.append(message+n,len-n);
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

    auto n=sockets::recv(handler_.get_fd(),readBuffer_.append(),readBuffer_.get_writable_size(),0);
    if(n>0)
    {
        readBuffer_.append(n);
        SmessageCallBack_(shared_from_this(),&readBuffer_);
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
    ssize_t len=static_cast<ssize_t>(writeBuffer_.get_readable_size());
    auto fd=handler_.get_fd();
    ssize_t n=sockets::send(fd,writeBuffer_.peek(),len,MSG_NOSIGNAL);
    if(n==len)
    {
        if(status_==Status::DisConnected)
        {
            sockets::shutdown(fd,SHUT_WR);
        }
        else 
        {
            handler_.canecllWriting();
        }
        writeBuffer_.retrieve(n);
    }
    else if(n>=0&&n<len)
    {
        if(!handler_.isWriting())handler_.setWriting();
        writeBuffer_.retrieve(n);
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
    assert(status_==Status::Connected);
    status_=Status::DisConnected;
}
void TcpConnection::handleError()   
{
    LOG_CLIENT_ERROR("have errno");
    if(SerrorCallBack_)SerrorCallBack_(shared_from_this());

    
}

}
}