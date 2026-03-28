
#include "../TcpServer.h"
#include "../TimerWheel.h"
#include "../TimerQueue.h"
#include "../../Common/SyncLog.h"
using namespace yy;
using namespace yy::net;
#include <algorithm>
const int N=20;
std::atomic<int> MsgCount=0;
std::atomic<int> ConnNum=0;





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
            onConnection(conn,Cloop);
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
    void onConnection(TcpConnectionPtr conn,EventLoop* loop)
    {
        ++ConnNum;
        LOG_SYSTEM_INFO("连接数:"<<ConnNum);
        auto addr=conn->addr(); 
        LOG_SYSTEM_INFO("new connection! "<<addr.sockaddrToString());
        conn->context<LTimeStamp>()=LTimeStamp::now();
        std::weak_ptr<TcpConnection> weakConn=conn;
        auto timer=std::make_shared<LTimer>( // 如果插入时间轮，则1min是不准确的，以时间轮的时间为准
        [this,weakConn]()
        {
            if(auto c=weakConn.lock()) 
            {   
                checkAlive(c);                
            }
        },
        30s,
        BaseTimer::FOREVER
        );

        timerWheel_.insert(timer);
        auto Ltimer=std::make_shared<LTimer>(
        [this,weakConn]()
        {
            if(auto c=weakConn.lock())
            {
                c->send("You had connected ten s",26);
                LOG_SYSTEM_DEBUG(c->addr().sockaddrToString()<<" had connected two min");   
                ++MsgCount;
                LOG_SYSTEM_DEBUG("MsgCount=="<<MsgCount);
            }
        },
        10s,
        1    
        );
        loop->runTimer<LowPrecision>(Ltimer);
        conn->context<LTimerPtr>()=timer;
    }
    void checkAlive(TcpConnectionPtr conn)
    {
        auto& lastFulsh=conn->context<LTimeStamp>();
        if(LTimeStamp::now()-lastFulsh>LTimeInterval(10s))
        {
            
            conn->disconnect();
            LOG_TCP_DEBUG(conn->addr().sockaddrToString()<<" Close!");
        }            
    }
    void onMessage(TcpConnectionPtr conn)
    {
        conn->context<LTimeStamp>()=LTimeStamp::now();

        LOG_SYSTEM_INFO("recv msg! from"<<conn->addr().sockaddrToString());
    }
    void onClose(TcpConnectionPtr conn)
    {
        --ConnNum;
        LOG_SYSTEM_INFO("连接数:"<<ConnNum);
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
