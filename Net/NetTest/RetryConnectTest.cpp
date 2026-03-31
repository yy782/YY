#include "../include/TcpClient.h"
#include "../include/EventLoopThread.h"
#include "../include/TimerQueue.h"
using namespace yy;
using namespace yy::net;
//./RetryConnectTest
int main()
{
    SyncLog::getInstance("../CliLog.log").getFilter() 
        .set_global_level(LOG_LEVEL_DEBUG) 
        .set_module_enabled("TCP")
        .set_module_enabled("SYSTEM")
        .set_module_enabled("TIME")
        .set_module_enabled("CLIENT")
        ;
    Address addr(8080,true);
    EventLoopThread  thread;
    auto loop=thread.run();
    TcpClient cli(addr,loop);
    cli.enableRetry();
    cli.setConnectionCallback([](int Cfd,const Address& Caddr,EventLoop* Cloop){
        auto conn=TcpConnection::accept(Cfd,Caddr,Cloop,Event(LogicEvent::Read));
        LOG_CLIENT_DEBUG("连接成功!");
        return conn;
    });
    cli.connect();
    int fd=0;
    loop->runTimer<LowPrecision>([&addr,&fd](){
        fd=sockets::createTcpSocketOrDie(AF_INET);
        sockets::bindOrDie(fd,addr);
        sockets::listenOrDie(fd);
    },10s,1);
    while(!cli.isConnected())
    {
        sleep(1);
    }
    thread.stop();
    sleep(1);
    sockets::close(fd);
    return 0;
}
