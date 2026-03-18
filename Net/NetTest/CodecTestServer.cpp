//./CodecTestServer
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

// 注意端口复用，导致调试有问题
//  ./CodecTestServer

class CodecTestServer
{
public:

    typedef TcpConnection::CharContainer CharContainer;
    CodecTestServer(const Address& addr,int thread_num,EventLoop* loop):
    server_(addr,thread_num,loop),
    loop_(loop)
    
    {
        server_.setConnectCallBack(std::bind(&CodecTestServer::onConnection,this,_1));
        server_.setMessageCallBack(std::bind(&CodecTestServer::onMessage,this,_1));
        server_.setCloseCallBack(std::bind(&CodecTestServer::onClose,this,_1));
        
    }
    void start()
    {
        server_.loop();
    }
    void stop()
    {
        server_.stop();
    }
    ~CodecTestServer()
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
        
        int p=1;
        while(p>0)
        {
            p=LineCodec::tryDecode(buffer.getReadView(),msg);
            if(p<=0)return;


            // 使用LineCodec::encode来正确编码消息
            LineCodec::encode(msg, conn->getSendBuffer());
            // 检查连接是否仍然是连接状态
            if(conn->isConnected())
            {
                LOG_SYSTEM_INFO("send msg: "<<msg);
                conn->sendOutput();
            }
            else
            {
                LOG_SYSTEM_INFO("connection closed, skip send msg: "<<msg);
            }
            LOG_SYSTEM_INFO("recv msg: "<<msg);
            // 消费掉缓冲区中的数据
            buffer.consume(p); // 使用tryDecode返回的完整长度，包含\r\n    
        }

    }
    void onClose(TcpConnectionPtr conn)
    {
        auto addr=conn->getAddr();
        LOG_SYSTEM_INFO("connection closed! "<<addr.sockaddrToString());
        loop_->quit();
        server_.stop();
        exit(0);
    }
    TcpServer server_;
    EventLoop* loop_;
};

int main() 
{

   
    EXCLUDE_BEFORE_COMPILATION(
        LOG_SYSTEM_INFO("[PID] "<<getpid());
    )
    SyncLog::getInstance("../Log.log").getFilter() 
        .set_global_level(LOG_LEVEL_DEBUG) 
        .set_module_enabled("TCP")
        .set_module_enabled("SYSTEM")
        .set_module_enabled("TIME");
    //daemonize(); // 目录被换了
  
    EXCLUDE_BEFORE_COMPILATION(
        LOG_SYSTEM_INFO("[PID] "<<getpid());
    )
    Address serverAddr(8080,true);
    
    EventLoop loop;
    CodecTestServer server(serverAddr,1,&loop);
    server.start();
    loop.loop();
}