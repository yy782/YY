#include <iostream>
#include <atomic>
#include <chrono>
#include <thread>
#include <iomanip>

#include "../EventLoop.h"
#include "../TcpServer.h"
#include "../TcpConnection.h"
#include "../InetAddress.h"
#include "../SignalHandler.h"
#include "../ConfigCenter.h"
//#include "../../Common/AsyncLog.h"
#include "../../Common/SyncLog.h"
#include "../../Common/TimeStamp.h"
#if 0
/usr/local/bin/valgrind --leak-check=full \
         --track-origins=yes \
         --show-leak-kinds=all \
         --num-callers=30 \
         --log-file=server_check.log \
         ./PingPongServer 4 0 
#endif
// tail -f server_check.log
using namespace yy;
using namespace yy::net;
//./PingPongServer 4 0
//ss -tan | grep 8080
// cd programs/yy/build/bin
int main(int argc, char* argv[])
{
    Signal::signal(SIGPIPE,[](){});
    int threadNums=1;
    bool isET=true;
    if(argc>1)
    {
        threadNums=std::atoi(argv[1]);
        isET=std::atoi(argv[2])==1?true:false;
    }
    SyncLog::getInstance("../Log.log").getFilter() 
    .set_global_level(LOG_LEVEL_WARN) 
    ;

    // SyncLog::getInstance("../Log.log").getFilter() 
    //     .set_global_level(LOG_LEVEL_DEBUG) 
    //     .set_module_enabled("TCP")
    //     .set_module_enabled("SYSTEM")
    //     ;
    Address listenAddr("0.0.0.0",8080);
    TcpServer server(listenAddr,1,threadNums);    

    int ConNum=0;
    if(isET)
    {
        server.setConnectCallBack([&server,&ConNum](int Cfd,const Address& Caddr,EventLoop* Cloop)mutable
        { 
            if(!sockets::setNonBlocking(Cfd))
            {
                LOG_ERRNO(errno);
                sockets::close(Cfd);
            }
            auto conn=TcpConnection::accept(Cfd,Caddr,Cloop,Event(LogicEvent::Read|LogicEvent::Edge));
            conn->setTcpNoDelay(true);   
            
            conn->setMessageCallBack([](TcpConnectionPtr con){
                auto& buf=con->recvBuffer();
                auto size=buf.readable_size();
                assert(size==4096);
                con->send(buf.retrieve(size),size);
                
            });
            conn->setErrorCallBack([](TcpConnectionPtr con){
                LOG_SYSTEM_ERROR(con->addr().sockaddrToString()<<" errno:"<<errno);
            });
            
            std::atomic<int> CloseNum=0;
            conn->setCloseCallBack([&CloseNum](TcpConnectionPtr){
                LOG_SYSTEM_DEBUG("CloseNum:"<<(++CloseNum));
            });
            
            LOG_TCP_DEBUG("连接数为:"<<(++ConNum));   
            return conn;
        });
    }
    else 
    {
        server.setConnectCallBack([&server,&ConNum](int Cfd,const Address& Caddr,EventLoop* Cloop)mutable
        { 
            auto conn=TcpConnection::accept(Cfd,Caddr,Cloop,Event(LogicEvent::Read));
            conn->setTcpNoDelay(true);             
            conn->setMessageCallBack([](TcpConnectionPtr con){
                auto& buf=con->recvBuffer();
                auto size=buf.readable_size();
                assert(size==4096);
                con->send(buf.retrieve(size),size);
                
            });
            conn->setErrorCallBack([](TcpConnectionPtr con){
                LOG_SYSTEM_ERROR(con->addr().sockaddrToString()<<" errno:"<<errno);
            });
            
            // std::atomic<int> CloseNum=0;
            // con->setCloseCallBack([&CloseNum](TcpConnectionPtr){
            //     LOG_SYSTEM_DEBUG("CloseNum:"<<(++CloseNum));
            // });
            conn->setCloseCallBack([](TcpConnectionPtr){

            });            
            LOG_TCP_DEBUG("连接数为:"<<(++ConNum));   
            return conn;
        });
    }

    server.loop();
    server.wait();        

    return 0;
}