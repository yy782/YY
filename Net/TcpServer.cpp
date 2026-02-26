#include "TcpServer.h"

namespace yy
{
namespace net
{
TcpServer::TcpServer(const Address& addr,int threadnum,int listenFdnum):
loop_(),
acceptors_(listenFdnum),
threadpool_(threadnum),
signalHandler_(&loop_)
{
    LOG_SYSTEM_INFO(addr.sockaddrToString());
    for(auto& acceptor:acceptors_)
    {
        acceptor=std::make_unique<Acceptor>(addr,&loop_);
        acceptor->setNewConnectCallBack(std::bind(&TcpServer::newConnection,this,_1));
        acceptor->listen();
    }
}   
void TcpServer::loop()
{
    threadpool_.run();
    loop_.loop();
} 
void TcpServer::stop()
{
    
    threadpool_.stop();
    loop_.quit();
}
void TcpServer::newConnection(TcpConnectionPtr conn)
{

    SconnectCallback_(conn);
    connects_.insert(conn);
    conn->setMessageCallBack(SmessageCallback_);
    conn->setCloseCallBack(ScloseCallback_);
    conn->setErrorCallBack(SerrorCallback_);
    conn->setRMessageBorder(SRmessageBorder_);
    conn->setWMessageBorder(SWmessageBorder_);
    conn->setName(conn->getAddr().sockaddrToString().c_str());
    threadpool_.addHandler(conn->getHandler());
}
}    
}