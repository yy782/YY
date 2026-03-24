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
//./PingPongServer 4
//ss -tan | grep 8080

int main(int argc, char* argv[])
{
    int threadNums=1;
    if(argc>1)
    {
        threadNums=std::atoi(argv[1]);
    }
    // SyncLog::getInstance("../Log.log").getFilter() 
    //   .set_global_level(LOG_LEVEL_DEBUG) 
    //   .set_module_enabled("TCP")
    //   .set_module_enabled("SYSTEM")
 
    //   ;
    Address listenAddr("0.0.0.0",8080);
    EventLoop loop;
    TcpServer server(listenAddr,threadNums,&loop);

    server.setConnectCallBack([](TcpConnectionPtr con)mutable
    {
        con->setTcpNoDelay(true);
        //con->setEvent(EventType::ReadEvent|EventType::EV_ET);
        con->setReading();


        //auto loop_= con->getHandler()->get_loop();
        //LOG_SYSTEM_DEBUG("loopAddr:"<<loop_<<" push:"<<(++(loop_->num)));
    });
    // server.setMessageCallBack([](TcpConnectionPtr con)
    // {
    //     auto& buf=con->getRecvBuffer();
    //     while(true)
    //     {
    //         auto size=buf.get_readable_size();
    //         if(size==0)break;
    //         assert(size>0);
    //         con->send(buf.retrieve(size),size);
    //     }
    // });
    server.setMessageCallBack([](TcpConnectionPtr con)
    {
        auto& buf=con->getRecvBuffer();
        auto size=buf.get_readable_size();
        assert(size==4096);
        con->send(buf.retrieve(size),size);
        
    });
    server.setCloseCallBack([](TcpConnectionPtr)mutable
    {
        //auto& addr=con->getAddr();
        //auto loop_=con->getHandler()->get_loop();
        //LOG_SYSTEM_INFO("connection closed! "<<addr.sockaddrToString()<<" CloseNum:"<<(++loop_->num2));
    });
    server.setErrorCallBack([](TcpConnectionPtr con){
        LOG_SYSTEM_ERROR(con->getAddr().sockaddrToString()<<" errno:"<<errno);
    });
    server.loop();
    loop.loop();


  
}