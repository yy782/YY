#include "TcpConnection.h"
#include "sockets.h"
#include <vector>
#include "EventLoop.h"
namespace yy
{
namespace net
{
TcpConnection::TcpConnection(int fd,Address addr,EventLoop* loop):
addr_(addr),
handler_(EventHandler::create(fd,loop))
{

    handler_->setReadCallBack(std::bind(&TcpConnection::handleRead,this));
    handler_->setWriteCallBack(std::bind(&TcpConnection::handleWrite,this));
    handler_->setCloseCallBack(std::bind(&TcpConnection::handleClose,this));
    handler_->setErrorCallBack(std::bind(&TcpConnection::handleError,this));

    handler_->setReading();
    handler_->setExcept();

    
}    

TcpConnection::CharContainer TcpConnection::recv()
{
    auto n=sockets::recv(handler_->get_fd(),readBuffer_.append(),readBuffer_.get_writable_size(),0);
    assert(n>=0);

    readBuffer_.move_write_index(n);
    auto str=readBuffer_.retrieve();
    return str;    
}
void TcpConnection::send(const char* message,size_t len)
{
    auto n=sockets::send(handler_->get_fd(),message,len,0);
    if(n>=0&&n<static_cast<ssize_t>(len))
    {
        writeBuffer_.append(message+n,len-n);
        handler_->setWriting();
        auto loop=handler_->get_loop();
        assert(loop);
        loop->update_listen(handler_);
    }
    else if(n<0)
    {
        handleError();
    }
}
void TcpConnection::handleRead()
{
    assert(SmessageCallBack_);

    char buf[1];
    auto n=sockets::recv(handler_->get_fd(),buf,1,MSG_PEEK|MSG_DONTWAIT);
    if(n>0)
    {
        SmessageCallBack_(shared_from_this());
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
    auto msg=writeBuffer_.retrieve();
    auto len=static_cast<ssize_t>(msg.size());
    auto n=sockets::send(handler_->get_fd(),msg.data(),msg.size(),0);
    if(n==len)
    {
        if(writeBuffer_.get_readable_size()==0)
        {
            handler_->canecllWriting();
        }
    }
    else if(n>=0&&n<len)
    {
        if(!handler_->isWriting())handler_->setWriting();
        writeBuffer_.append(msg.data()+n,msg.size()-n);
    }
    else 
    {
        handleError();
    }
    if(SwriteCompleteCallBack_)SwriteCompleteCallBack_(shared_from_this());
}
void TcpConnection::handleClose()
{

    auto loop=handler_->get_loop();
    assert(loop);
    loop->submit(std::bind(&EventLoop::remove_listen,loop,handler_));

    if(ScloseCallBack_)ScloseCallBack_(shared_from_this());
}
void TcpConnection::handleError()   //FIXME 错误处理的不好
{
    assert(SerrorCallBack_);
    auto loop=handler_->get_loop();
    assert(loop);
    loop->submit(std::bind(&EventLoop::remove_listen,loop,handler_));
    SerrorCallBack_(shared_from_this());
    ScloseCallBack_(shared_from_this());
}

}
}