#include "../TcpClient.h"
#include <vector>
#include <iostream>
#include "../../Common/TimeStamp.h"
#include <atomic>
#include "../EventLoopThread.h"
// ./TimerTestClient
using namespace yy;
using namespace yy::net;

class TimerClient;

const int N=20;
std::atomic<int> ConnNum=0;

bool AllDisCon=false;
class TimerClient// stdout是线程不安全的
{
public:
    TimerClient(const Address& serverAddr,EventLoop* loop):
    client_(serverAddr,loop)
    {
        client_.setMessageCallBack([this](TcpConnectionPtr){
            
        });
        client_.setConnectionCallback([this](TcpConnectionPtr conn){
            conn->setEvent(Event(LogicEvent::Read|LogicEvent::Edge));
            ++ConnNum;
            conn->send("hello !",8);
        });
        client_.setCloseCallBack([this](TcpConnectionPtr){
            --ConnNum;
            if(ConnNum==0)AllDisCon=true;
        });
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
    EventLoopThread thread;
    auto loop=thread.run();
    for(int i=0;i<N;i++)
    {
        clients.emplace_back(new TimerClient(serverAddr,loop));
        
    }

    for(int i=0;i<N;++i)
    {
        clients[i]->connect();
    }    
    while(!AllDisCon)
    {
        sleep(1);
    }
    thread.stop();
    return 0;
}