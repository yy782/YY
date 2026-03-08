
#include "../net.h"
using namespace yy;
using namespace yy::net;
using namespace yy::net::sockets;
#include <algorithm>

//./EchoServer
// cd programs/yy/build/bin
class EchoServer
{
public:

    typedef TcpConnection::CharContainer CharContainer;
    EchoServer(const Address& addr,int thread_num):
    server_(addr,thread_num)
    {
        server_.setConnectCallBack(std::bind(&EchoServer::onConnection,this,_1));
        server_.setMessageCallBack(std::bind(&EchoServer::onMessage,this,_1));
        server_.setCloseCallBack(std::bind(&EchoServer::onClose,this,_1));
        server_.getSignalHandler().addSign(SIGTERM,std::bind(&EchoServer::stop,this));


        
    }
    void start()
    {
        server_.loop();
    }
    void stop()
    {
        server_.stop();
    }
    ~EchoServer()
    {
        LOG_SYSTEM_INFO("server stop!");
    }
private:
    void onConnection(TcpConnectionPtr conn)
    {
        auto addr=conn->getAddr();
        LOG_SYSTEM_INFO("new connection! "<<addr.sockaddrToString());
        conn->setReading();// @note 对方连接是否决定监听由业务层决定
        auto& data=conn->getData();
        data.push_back(LTimeStamp::now());

        std::weak_ptr<TcpConnection> weakConn=conn;
        auto timer=std::make_shared<LTimer>(
        [this,weakConn]()
        {
            if(auto c=weakConn.lock()) 
            {   
                checkAlive(c);                
            }
        },
        60*5,
        FOREVER
        );
        server_.addTime(timer,false);
        auto Htimer=std::make_shared<HTimer>(
        [this,weakConn]()
        {
            if(auto c=weakConn.lock())
            {
                c->send("You had connected two min",26);
                LOG_SYSTEM_DEBUG(c->getAddr().sockaddrToString()<<" had connected two min");
            }
        },
        120*(1e6),
        1    
        );
        server_.addTime(Htimer,true);
        data.push_back(timer);
    }
    void checkAlive(TcpConnectionPtr conn)
    {
        
        auto& data=conn->getData();
        LTimeStamp lastFulsh=std::any_cast<LTimeStamp>(data[0]);
        if((LTimeStamp::now()-lastFulsh)>LTimeInterval(60*5))
        {
            conn->send("Close!",7);
            conn->disconnect();
            auto timer=std::any_cast<LTimerPtr>(data[1]);
            timer->cancel();
        }            
        

    }
    void onMessage(TcpConnectionPtr conn)
    {
        TcpBuffer& buffer=conn->getRecvBuffer();
        auto& data=conn->getData();
        data[0]=LTimeStamp::now();

        char* last=buffer.findBorder("\n");
        std::string msg(buffer.peek(),last);
        if(msg=="bye\n") // @note 
        {
            return;
        }

        conn->send(msg.data(),msg.size());
        LOG_SYSTEM_INFO("recv msg: "<<msg);
    }
    void onClose(TcpConnectionPtr conn)
    {
        auto addr=conn->getAddr();
        LOG_SYSTEM_INFO("connection closed! "<<addr.sockaddrToString());
        auto& data=conn->getData();
        auto timer=std::any_cast<LTimerPtr>(data[1]);
        timer->cancel();
    }
    TcpServer server_;

};

int main(int argc,char* argv[])
{
    auto& instance=SyncLog::getInstance("../../build/Log.log",10);
    instance.getFilter()->set_global_level(LOG_LEVEL_DEBUG);
    instance.getFilter()->set_module_enabled(LogModule::SYSTEM,true);
    instance.getFilter()->set_module_enabled(LogModule::SIGNAL,true);
    instance.getFilter()->set_module_enabled(LogModule::TIME,true);
    instance.getFilter()->set_module_enabled(LogModule::CLIENT,true);    
    
    daemonize();
    Address addr;
    if(argc<2)
    {
        addr=Address("127.0.0.1",8080);
    }
    else
    {
       addr=Address(std::stoi(argv[1]),true); 
    }
    


    LOG_SYSTEM_INFO("[PID] "<<getpid());   

    EchoServer server(addr,1);
    server.start();
}