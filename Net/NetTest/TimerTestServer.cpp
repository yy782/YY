
#include "../TcpServer.h"
#include "../TimerWheel.h"
#include "../TimerQueue.h"
#include "../../Common/SyncLog.h"
using namespace yy;
using namespace yy::net;
#include <algorithm>

namespace TcpCon 
{
struct checkAlive
{
    using ReturnType=void;
};
struct createAliveTimer
{
    using ReturnType=void;
    TimerWheel& timerWheel_;
    createAliveTimer(TimerWheel& timerWheel):timerWheel_(timerWheel){}
}; 
}
using namespace TcpCon;
template<>
void TcpConnection::Extend<checkAlive>(checkAlive)
{
    auto& lastFulsh=context<LTimeStamp>();
    if(LTimeStamp::now()-lastFulsh>LTimeInterval(10s))
    {
        disconnect();
        LOG_TCP_DEBUG(addr().sockaddrToString()<<" Close!");
    } 
}
template<>
void TcpConnection::Extend<createAliveTimer>(createAliveTimer cc)
{
    auto& timerWheel=cc.timerWheel_;
    auto timer=std::make_shared<LTimer>( // 如果插入时间轮，则1min是不准确的，以时间轮的时间为准
    [weakConn=weak_from_this(),this]()
    {
        if(auto c=weakConn.lock()) 
        {   
            Extend<checkAlive>({});               
        }
    },
    30s,
    BaseTimer::FOREVER
    );
    timerWheel.insert(timer);
    context<LTimerPtr>()=timer;
}

//./TimerTestServer
// cd programs/yy/build/bin
class TimerServer
{
public:
    typedef TcpConnection::CharContainer CharContainer;
    TimerServer(const Address& addr,int thread_num):
    server_(addr,1,thread_num),
    timerWheel_(thread_.run(),2,5)
    {
        server_.setConnectCallBack([this](int Cfd,const Address& Caddr,EventLoop* Cloop)
        {
            auto conn=TcpConnection::accept(Cfd,Caddr,Cloop,Event(LogicEvent::Read));
            onConnection(conn);
            conn->setMessageCallBack([this](TcpConnectionPtr con){
                onMessage(con);
            });
            conn->setCloseCallBack([this](TcpConnectionPtr con){
                onClose(con);
            });
            return conn;
        });
    }
    void start()
    {
        server_.loop();
    }
    void stop()
    {
        server_.stop();
    }
    void wait()
    {
        server_.wait();
    }
    ~TimerServer()
    {
        LOG_SYSTEM_INFO("server stop!");
        
    }
private:
    void onConnection(TcpConnectionPtr conn)
    {
        auto addr=conn->addr(); 
        LOG_SYSTEM_INFO("new connection! "<<addr.sockaddrToString());
        conn->context<LTimeStamp>()=LTimeStamp::now();
        conn->Extend<createAliveTimer>({timerWheel_});
    }

    void onMessage(TcpConnectionPtr conn)
    {
        conn->context<LTimeStamp>()=LTimeStamp::now();
        LOG_SYSTEM_INFO("recv msg! from"<<conn->addr().sockaddrToString());
    }
    void onClose(TcpConnectionPtr conn)
    {
        auto addr=conn->addr(); 
        LOG_SYSTEM_INFO("connection closed! "<<addr.sockaddrToString());
        conn->context<LTimerPtr>()->cancel();
    }
    EventLoopThread thread_;
    TcpServer server_;
    TimerWheel timerWheel_;
};

int main()
{

    SyncLog::getInstance("../Log.log").getFilter() 
        .set_global_level(LOG_LEVEL_DEBUG) 
        .set_module_enabled("TCP")
        .set_module_enabled("SYSTEM")
        .set_module_enabled("TIME");
    EXCLUDE_BEFORE_COMPILATION(
        LOG_SYSTEM_INFO("[PID] "<<getpid());
    )        
    Address addr("127.0.0.1",8080);   
    TimerServer server(addr,4);
    server.start();
    server.wait();

}
