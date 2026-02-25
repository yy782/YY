
#include "../net.h"
using namespace yy;
using namespace yy::net;
#include <algorithm>

//./EchoServer
// cd programs/yy/build/bin
class EchoServer
{
public:
    EchoServer(const Address& addr,int thread_num):
    server_(addr,thread_num)
    {
        server_.setConnectCallBack(std::bind(&EchoServer::onConnection,this,_1));
        server_.setMessageCallBack(std::bind(&EchoServer::onMessage,this,_1));
        server_.setCloseCallBack(std::bind(&EchoServer::onClose,this,_1));
        server_.getSignalHandler().addSign(SIGTERM,std::bind(&EchoServer::stop,this));
        server_.setRMessageBorder([](TcpServer::CharContainer msg)->TcpServer::CharContainer
        {
            auto it=std::find(msg.begin(),msg.end(),'\n');
            if(it!=msg.end())
            {
                return TcpServer::CharContainer(msg.begin(),it+1);
            }
            else 
            {
                return TcpServer::CharContainer();
            }
        });
        server_.setWMessageBorder([](TcpServer::CharContainer msg)->TcpServer::CharContainer
        {
            auto it=std::find(msg.begin(),msg.end(),'\n');
            if(it!=msg.end())
            {
                return TcpServer::CharContainer(msg.begin(),it+1);
            }
            else 
            {
                return TcpServer::CharContainer();
            }
        });
    }
    void start()
    {
        server_.loop();
    }
    void stop()
    {
        server_.stop();
        close_log_file();
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
    }
    void onMessage(TcpConnectionPtr conn)
    {
        auto msg=conn->recv();
        conn->send(msg.data(),msg.size());
        LOG_SYSTEM_INFO("recv msg: "<<msg.data());
    }
    void onClose(TcpConnectionPtr conn)
    {
        auto addr=conn->getAddr();
        LOG_SYSTEM_INFO("connection closed! "<<addr.sockaddrToString());
    }
    TcpServer server_;
};

int main(int argc,char* argv[])
{
    Address addr;
    if(argc<2)
    {
        addr=Address("127.0.0.1",8080);
    }
    else
    {
       addr=Address(std::stoi(argv[1]),true); 
    }
    
    init_log_file();
    set_log_global_level(LOG_LEVEL_DEBUG);
    enable_log_module(LogModule::SYSTEM); 
    enable_log_module(LogModule::WARN);
    enable_log_module(LogModule::CLIENT);
    //daemonize();
    LOG_SYSTEM_INFO("[PID] "<<getpid());   

    EchoServer server(addr,1);
    server.start();
}