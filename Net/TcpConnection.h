#ifndef _YY_NET_TCPCONNECTION_H_
#define _YY_NET_TCPCONNECTION_H_
#include "../Common/noncopyable.h"
#include "InetAddress.h"
#include "EventHandler.h"
#include "../Common/Buffer.h"
#include <memory>
namespace yy
{
namespace net
{

    
class TcpConnection:noncopyable,
                    public std::enable_shared_from_this<TcpConnection>
{
public:
    typedef Buffer::CharContainer CharContainer;
    typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
    typedef std::function<void(TcpConnectionPtr)> ServicesMessageCallBack;
    typedef std::function<void(TcpConnectionPtr)> ServicesWriteCompleteCallBack;
    typedef std::function<void(TcpConnectionPtr)> ServicesCloseCallBack;
    typedef std::function<void(TcpConnectionPtr)> ServicesErrorCallBack;
    typedef Buffer::FindCompleteMessageFunc FindCompleteMessageFunc;

    TcpConnection(int fd,Address addr,EventLoop* loop);
    ~TcpConnection(){}// 构析函数不能触发回调了，TcpConnectionPtr不允许
    //TcpConnect(TcpConnect&& other);
    void connect()
    {
        sockets::connect(handler_->get_fd(),addr_);
    }
    void disconnect()
    {
        if(ScloseCallBack_)ScloseCallBack_(shared_from_this());
        handler_->removeListen();
    }
    EventHandlerPtr getHandler(){return handler_;}    
    int get_fd()const{return handler_->get_fd();}
    Address getAddr()const{return addr_;}
    void setReading()
    {
        handler_->setReading();
        handler_->setExcept();
    }
/**
 * 设置服务消息回调函数
 * @param cb 回调函数对象，通过移动语义传递以提高效率
 */
    void setMessageCallBack(ServicesMessageCallBack cb){SmessageCallBack_=std::move(cb);}
    void setWriteCompleteCallBack(ServicesWriteCompleteCallBack cb){SwriteCompleteCallBack_=std::move(cb);}
    void setCloseCallBack(ServicesCloseCallBack cb){ScloseCallBack_=std::move(cb);} // @brief 这是对端关闭的回调
    void setErrorCallBack(ServicesErrorCallBack cb){SerrorCallBack_=std::move(cb);}

    void setRMessageBorder(FindCompleteMessageFunc cb){readBuffer_.set_find_complete_message_func(std::move(cb));};
    void setWMessageBorder(FindCompleteMessageFunc cb){writeBuffer_.set_find_complete_message_func(std::move(cb));};
    void setTcpAlive(bool on,int idleSeconds=7200, 
                  int intervalSeconds=75,int maxProbes=9)
    {
        sockets::setKeepAlive(handler_->get_fd(),on,idleSeconds,intervalSeconds,maxProbes);
    }
    void setName(const char* name){handler_->set_name(name);}

    void send(const char* message,size_t len);
    CharContainer recv();
private:
    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();
    Address addr_; // @prief 对端的地址
    EventHandlerPtr handler_;
    Buffer readBuffer_;
    Buffer writeBuffer_;

    ServicesMessageCallBack SmessageCallBack_;
    ServicesWriteCompleteCallBack SwriteCompleteCallBack_;
    ServicesCloseCallBack ScloseCallBack_;
    ServicesErrorCallBack SerrorCallBack_;
    
};
}    
}


#endif