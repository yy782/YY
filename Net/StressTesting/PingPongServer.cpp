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

using namespace yy;
using namespace yy::net;
//./PingPongServer 4 0
//ss -tan | grep 8080
// cd programs/yy/build/bin
int main(int argc, char* argv[])
{
    int threadNums=8;
    bool isET=true;
    if(argc>1)
    {
        threadNums=std::atoi(argv[1]);
        isET=std::atoi(argv[2])==1?true:false;
    }
    SyncLog::getInstance("../Log.log").getFilter() 
      .set_global_level(LOG_LEVEL_ERROR) 
      ;
    // SyncLog::getInstance("../Log.log").getFilter() 
    //     .set_global_level(LOG_LEVEL_DEBUG) 
    //     .set_module_enabled("TCP")
    //     .set_module_enabled("SYSTEM")
    //     ;
    Address listenAddr("0.0.0.0",8080);
    EventLoop loop;
    TcpServer server(listenAddr,threadNums,&loop);
    int ConNum=0;
    if(isET)
    {
        server.setConnectCallBack([&server,&ConNum](TcpConnectionPtr con)mutable
        { 
            if(!sockets::setNonBlocking(con->fd()))
            {
                LOG_ERRNO(errno);
            }
            con->setTcpNoDelay(true);
            con->setEvent(Event(LogicEvent::Read|LogicEvent::Edge));    
            
            LOG_TCP_DEBUG("连接数为:"<<(++ConNum));   
        });
    }
    else 
    {
        server.setConnectCallBack([&server,&ConNum](TcpConnectionPtr con)mutable
        { 
            con->setTcpNoDelay(true);
            con->setReading();     
            LOG_TCP_DEBUG("连接数为:"<<(++ConNum));   
        });
    }


    server.setMessageCallBack([](TcpConnectionPtr con)
    {
        auto& buf=con->recvBuffer();
        auto size=buf.readable_size();
        assert(size==4096);
        con->send(buf.retrieve(size),size);
        
    });
    server.setErrorCallBack([](TcpConnectionPtr con){
        LOG_SYSTEM_ERROR(con->addr().sockaddrToString()<<" errno:"<<errno);
    });

    std::atomic<int> CloseNum=0;
    server.setCloseCallBack([&CloseNum](TcpConnectionPtr){
        LOG_SYSTEM_DEBUG("CloseNum:"<<(++CloseNum));
    });

    server.loop();
    loop.loop();


  
}