#ifndef _YY_NET_TCPSERVER_H_
#define _YY_NET_TCPSERVER_H_

#include "../Common/noncopyable.h"
#include "../Common/SyncLog.h"
#include "Acceptor.h"
#include "EventHandler.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "TcpConnection.h"
#include "SignalHandler.h"
#include "TimerQueue.h"
#include "TimerWheel.h"
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
    //typedef std::vector<std::unique_ptr<Acceptor>> AcceptorList;
    typedef std::unique_ptr<Acceptor> AcceptorPtr;
    typedef Acceptor::NewConnectCallBack ServicesConnectCallBack;
    typedef TcpConnection::ServicesMessageCallBack ServicesMessageCallBack;
    typedef TcpConnection::ServicesCloseCallBack ServicesCloseCallBack;
    typedef TcpConnection::ServicesErrorCallBack ServicesErrorCallBack;
    typedef TcpConnection::CharContainer CharContainer;

    TcpServer(const Address& addr,int threadnum,EventLoop* loop);
    ~TcpServer()=default;
    void setConnectCallBack(ServicesConnectCallBack cb){SconnectCallback_=std::move(cb);}
    void setMessageCallBack(ServicesMessageCallBack cb){SmessageCallback_=std::move(cb);}
    void setCloseCallBack(ServicesCloseCallBack cb){ScloseCallback_=std::move(cb);}
    void setErrorCallBack(ServicesErrorCallBack cb){SerrorCallback_=std::move(cb);}




    void removeConnection(TcpConnectionPtr conn);
    
    
    void loop();
    void stop();
private:
    void newConnection(TcpConnectionPtr conn);
    EventLoop* loop_;
    AcceptorPtr acceptor_;
    EventLoopThreadPool threadpool_;
    ConnectMap connects_;

    // TimerQueue<LowPrecision> LTimerQueue_;
    // TimerQueue<HighPrecision> HTimerQueue_;
    // TimerWheel TimerWheel_;


    ServicesConnectCallBack SconnectCallback_;
    ServicesMessageCallBack SmessageCallback_;
    ServicesCloseCallBack ScloseCallback_;
    ServicesErrorCallBack SerrorCallback_;
}; 

}    
}


#endif