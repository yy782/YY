#include "../include/TcpClient.h"
#include <vector>
#include <iostream>
#include "../../Common/TimeStamp.h"
#include <atomic>
#include "../include/EventLoopThread.h"
// ./TimerTestClient
using namespace yy;
using namespace yy::net;

class TimerClient;

int N;
std::atomic<int> DisConNum=0;

bool AllDisCon=false;
class TimerClient// stdout是线程不安全的
{
public:
    TimerClient(const Address& serverAddr,EventLoop* loop):
    client_(serverAddr,loop)
    {
        client_.setConnectionCallback([this](int Cfd,const Address& Caddr,EventLoop* Cloop){
            auto conn=TcpConnection::accept(Cfd,Caddr,Cloop,Event(LogicEvent::Read));
            conn->send("hello !",8);
            conn->setMessageCallBack([this](TcpConnectionPtr){});
            conn->setCloseCallBack([this](TcpConnectionPtr){
                ++DisConNum;
                if(DisConNum.load()==N)AllDisCon=true;
            });
            return conn;
        });
    }
    void connect()
    {
        client_.connect();
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
int main(int argc, char* argv[])
{ 
    N=50;
    if(argc>1)
    {
        N=std::atoi(argv[1]);
    }
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