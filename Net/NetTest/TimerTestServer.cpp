
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
    TimerServer(const Address& addr,int thread_num,EventLoop* loop):
    server_(addr,thread_num,loop),
    timerWheel_(loop,2,5),
    LtimerQueue_(loop)
    {
        server_.setConnectCallBack(std::bind(&TimerServer::onConnection,this,_1));
        server_.setMessageCallBack(std::bind(&TimerServer::onMessage,this,_1));
        server_.setCloseCallBack(std::bind(&TimerServer::onClose,this,_1));
    }
    void start()
    {
        server_.loop();
    }
    void stop()
    {
        server_.stop();
    }
    ~TimerServer()
    {
        LOG_SYSTEM_INFO("server stop!");
        
    }
private:
    void onConnection(TcpConnectionPtr conn)
    {
        ++ConnNum;
        LOG_SYSTEM_INFO("连接数:"<<ConnNum);
        auto addr=conn->addr(); 
        LOG_SYSTEM_INFO("new connection! "<<addr.sockaddrToString());
        conn->setEvent(Event(LogicEvent::Read|LogicEvent::Edge));
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
        LtimerQueue_.insert(Ltimer);
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
    TcpServer server_;
    TimerWheel timerWheel_;
    TimerQueue<LowPrecision> LtimerQueue_;
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
    EventLoop loop;
    TimerServer server(addr,4,&loop);
    
    Signal::signal(SIGTERM,[&server,&loop](){
        LOG_SYSTEM_DEBUG("Siganal handle exit");
        loop.quit();
        server.stop();
        
        sleep(2);
    });
    server.start();
    loop.runTimer<LowPrecision>([](){
        LOG_TIME_INFO("server run 5s");
    },5s,1);
    loop.loop();

}
