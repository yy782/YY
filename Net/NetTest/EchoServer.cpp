//./EchoServer
// cd programs/yy/build/bin
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <algorithm>
#include <thread>
#include <vector>
#include "../TcpServer.h"
#include "../Codec.h"
#include "../../Common/SyncLog.h"
#include "../ConfigCenter.h"
#include "../../Common/AsyncLog.h"
#include "../../Common/SyncLog.h"
using namespace yy;
using namespace yy::net;
using namespace yy::net::sockets;

//pidof CodecTestServer


class EchoServer
{
public:

    typedef TcpConnection::CharContainer CharContainer;
    EchoServer(const Address& addr,int thread_num,EventLoop* loop):
    server_(addr,thread_num,loop)
    
    {
        server_.setConnectCallBack(std::bind(&EchoServer::onConnection,this,_1));
        server_.setMessageCallBack(std::bind(&EchoServer::onMessage,this,_1));
        server_.setCloseCallBack(std::bind(&EchoServer::onClose,this,_1));
        
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
    }
    void onMessage(TcpConnectionPtr conn)
    {
        TcpBuffer& buffer=conn->getRecvBuffer();
        string_view msg;


        const char* last=buffer.findBorder("\n");
        if(last == buffer.peek() + buffer.get_readable_size()) // 没有找到\n
        {
            return;
        }
        msg=string_view(buffer.peek(),last);
        
 
   
        if(msg=="bye") // @note 
        {
            return;
        }

        conn->send(msg.data(),msg.size());
        LOG_SYSTEM_INFO("recv msg: "<<msg);
        // 消费掉缓冲区中的数据
        buffer.consume(msg.size()+ 1); // +1 是为了包含\n
    }
    void onClose(TcpConnectionPtr conn)
    {
        auto addr=conn->getAddr();
        LOG_SYSTEM_INFO("connection closed! "<<addr.sockaddrToString());
    }
    TcpServer server_;

};

int main() 
{

   
    Conf config;
    int parse_result=config.parse("../../Net/NetTest/config.conf");
    if(parse_result!=0)
    {
        std::cerr<<"Failed to parse config.conf at line "<<parse_result<<std::endl;
        return 1;
    }        

    
    std::string host=config.get("server","host");
    int port=static_cast<int>(config.getInteger("server","port"));
    int thread_nums=static_cast<int>(config.getInteger("server","threadnums"));

    bool isAsync=config.getBoolean("log","isAsync");
    std::string logLevel=config.get("log","logLevel");
    std::string logPath=config.get("log","logPath");
    std::list<std::string> modules=config.getStrings("log","modules");
    
    if(isAsync)
    {
        auto async_flush_interval=config.getDuration("AsyncLog","flush_interval");
        size_t async_buffer_size=static_cast<size_t>(config.getInteger("AsyncLog","BufferSize"));        
        auto& instance=AsyncLog::getInstance(logPath.c_str(),async_flush_interval,async_buffer_size);
        instance.getFilter().set_global_level(logLevel)
                                .set_module_enabled(modules);
    }
    else
    {
        auto sync_flush_interval=config.getDuration("SyncLog","flush_interval");
        auto& instance=SyncLog::getInstance(logPath.c_str(),sync_flush_interval);
        instance.getFilter().set_global_level(logLevel)
                                .set_module_enabled(modules);
    }
    //daemonize(); // 目录被换了
  
    EXCLUDE_BEFORE_COMPILATION(
        LOG_SYSTEM_INFO("[PID] "<<getpid());
    )
    Address serverAddr(host.c_str(),port);
    
    EventLoop loop;
    EchoServer server(serverAddr,thread_nums,&loop);
    server.start();
    auto& signal_handler=SignalHandler::getInstance(&loop);
    signal_handler.addSign(SIGTERM,[&server,&loop](){
        loop.quit();
        server.stop();
    });
    loop.loop();
}
