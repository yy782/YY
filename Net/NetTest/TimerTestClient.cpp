#include "../TcpClient.h"
#include <vector>
#include <iostream>
#include "../../Common/TimeStamp.h"
#include <atomic>
// ./TimerTestClient
using namespace yy;
using namespace yy::net;

class TimerClient;

const int N=20;

class TimerClient// stdout是线程不安全的
{
public:
    TimerClient(const Address& serverAddr,EventLoop* loop):
    client_(serverAddr,loop)
    {
        client_.setMessageCallBack([this](TcpConnectionPtr,string_view){
        
        });
        client_.setCloseCallBack([this](TcpConnectionPtr){
            std::cout<<"client close"<<std::endl;
        });
    }
    void send(const char* msg,size_t len)
    {
        client_.send(msg,len);
    }
    void connect()
    {
        client_.connect();
        //cout<<"connect success "<<client_.getPeerAddr().sockaddrToString()<<endl;
    }
    void disconnect()
    {
        
        client_.disconnect();
    }
    bool isConnected()
    {
        return client_.isConnected();
    }
private:
    TcpClient client_;
  
};
int main()
{ 
    std::vector<std::unique_ptr<TimerClient>> clients;
    Address serverAddr("127.0.0.1",8080);
    EventLoop loop;

    for(int i=0;i<N;i++)
    {
        clients.emplace_back(new TimerClient(serverAddr,&loop));
        
    }

    for(int i=0;i<N;++i)
    {
        clients[i]->connect();
    }
   
    for(int i=0;i<N/2;++i)
    {
        clients[i]->send("hello world",11);
    }
    std::thread t([&loop](){
        loop.loop();
    });     
    sleep(120); 
    for(int i=0;i<N;++i)
    {
        assert(!clients[i]->isConnected());
    }
    loop.quit();
    t.join();
    return 0;
}