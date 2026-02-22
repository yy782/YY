#ifndef _YY_NET_TCPSERVER_H_
#define _YY_NET_TCPSERVER_H_

#include "../Common/noncopyable.h"
#include "Acceptor.h"
#include "EventHandler.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "TcpConnect.h"
#include <set>
#include <memory>
namespace yy
{
namespace net
{



class TcpServer:public noncopyable
{
public:
    typedef std::set<std::unique_ptr<TcpConnect>> ConnectMap;
    typedef Acceptor::NewConnectCallBack ConnectCallBack;
    typedef EventHandler::ReadCallBack MessageCallBack;
    typedef EventHandler::CloseCallBack CloseCallBack;

    TcpServer(const Address& addr,int threadnum);
    void setConnectCallBack(ConnectCallBack cb){connectCallback_=std::move(cb);}
    void setMessageCallBack(MessageCallBack cb){messageCallback_=std::move(cb);}
    void setCloseCallBack(CloseCallBack cb){closeCallback_=std::move(cb);}
    
    void loop();
    void stop();
private:
    EventLoop loop_;
    Acceptor acceptor_;
    EventHandler handler_;
    EventLoopThreadPool threadpool_;
    ConnectMap connects_;

    ConnectCallBack connectCallback_;
    MessageCallBack messageCallback_;
    CloseCallBack closeCallback_;
}; 

}    
}


#endif