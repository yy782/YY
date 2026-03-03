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
    LOG_CLIENT_DEBUG("Delete "<<handler_.printName());
}
void TcpConnection::disconnect()
{
    status_=Status::DisConnected;
}  
TcpConnection::CharContainer TcpConnection::recv()
{
    Recvlocker_.lock();
    auto n=sockets::recv(handler_.get_fd(),readBuffer_.append(),readBuffer_.get_writable_size(),0);
    assert(n>=0);

    readBuffer_.move_write_index(n);
    auto str=readBuffer_.retrieve();

    Recvlocker_.unlock();
    return str;    
}
void TcpConnection::send(const char* message,size_t len)
{
    auto loop=handler_.get_loop();
    if(!loop->isInLoopThread())
    {
        auto self=shared_from_this();
        if(len<=64) 
        {
            std::array<char, 64> data;
            memcpy(data.data(), message, len);
            loop->submit([self,data,len](){
                self->send(data.data(),len);
            });
        } 
        else 
        {
            loop->submit([self,data=std::vector<char>(message,message+len)]() 
            {
                self->send(data.data(),data.size());
            });
        }
    }
    else
    {
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

}
void TcpConnection::handleRead()
{
    assert(SmessageCallBack_);

    char buf[1];
    auto n=sockets::recv(handler_.get_fd(),buf,1,MSG_PEEK|MSG_DONTWAIT);
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
    auto fd=handler_.get_fd();
    auto n=sockets::send(fd,msg.data(),msg.size(),MSG_NOSIGNAL);
    if(n==len)
    {
        if(writeBuffer_.get_readable_size()==0)
        {
            if(status_==Status::DisConnected)
            {
                sockets::shutdown(fd,SHUT_WR);
            }
            else 
            {
                handler_.canecllWriting();
            }
        }
    }
    else if(n>=0&&n<len)
    {
        if(!handler_.isWriting())handler_.setWriting();
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

    auto loop=handler_.get_loop();
    assert(loop);
    loop->remove_listen(&handler_);

    if(ScloseCallBack_)ScloseCallBack_(shared_from_this());
    assert(status_==Status::Connected);
    status_=Status::DisConnected;
}
void TcpConnection::handleError()   //FIXME 错误处理的不好
{
    //assert(SerrorCallBack_);
    auto loop=handler_.get_loop();
    assert(loop);
    loop->remove_listen(&handler_);
    if(SerrorCallBack_)SerrorCallBack_(shared_from_this());
    if(ScloseCallBack_)ScloseCallBack_(shared_from_this());
}

}
}