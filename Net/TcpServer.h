#ifndef _YY_NET_TCPSERVER_H_
#define _YY_NET_TCPSERVER_H_

#include "config.h"

#include "../Common/noncopyable.h"
#include "../Common/Log.h"
#include "Acceptor.h"
#include "EventHandler.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "TcpConnection.h"
#include "SignalHandler.h"
#include <set>
#include <memory>
#include <vector>
namespace yy
{
namespace net
{

typedef Acceptor::TcpConnectionPtr TcpConnectionPtr;

class TcpServer:public noncopyable
{
public:
    
    typedef std::set<TcpConnectionPtr> ConnectMap;
    typedef std::vector<std::unique_ptr<Acceptor>> AcceptorList;
    typedef Acceptor::NewConnectCallBack ServicesConnectCallBack;
    typedef TcpConnection::ServicesCloseCallBack ServicesMessageCallBack;
    typedef TcpConnection::ServicesCloseCallBack ServicesCloseCallBack;
    typedef TcpConnection::ServicesErrorCallBack ServicesErrorCallBack;
    typedef TcpConnection::FindCompleteMessageFunc FindCompleteMessageFunc;
    typedef TcpConnection::CharContainer CharContainer;

    TcpServer(const Address& addr,int threadnum,int listenFdnum=1);
    void setConnectCallBack(ServicesConnectCallBack cb){SconnectCallback_=std::move(cb);}
    void setMessageCallBack(ServicesMessageCallBack cb){SmessageCallback_=std::move(cb);}
    void setCloseCallBack(ServicesCloseCallBack cb){ScloseCallback_=std::move(cb);}
    void setErrorCallBack(ServicesErrorCallBack cb){SerrorCallback_=std::move(cb);}

    void setRMessageBorder(FindCompleteMessageFunc cb){SRmessageBorder_=std::move(cb);}
    void setWMessageBorder(FindCompleteMessageFunc cb){SWmessageBorder_=std::move(cb);}
    
    SignalHandler& getSignalHandler(){return signalHandler_;}
    
    void loop();
    void stop();
private:
    void newConnection(TcpConnectionPtr conn);
    EventLoop loop_;
    AcceptorList acceptors_;
    EventLoopThreadPool threadpool_;
    ConnectMap connects_;
    SignalHandler signalHandler_;

    ServicesConnectCallBack SconnectCallback_;
    ServicesMessageCallBack SmessageCallback_;
    ServicesCloseCallBack ScloseCallback_;
    ServicesErrorCallBack SerrorCallback_;
    FindCompleteMessageFunc SRmessageBorder_;
    FindCompleteMessageFunc SWmessageBorder_;
}; 

}    
}


#endif