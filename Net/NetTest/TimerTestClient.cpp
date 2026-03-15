#include "../TcpClient.h"
#include <vector>
#include <iostream>
#include "../../Common/TimeStamp.h"
// ./TimerTestClient
using namespace yy;
using namespace yy::net;

class TimerClient;

const int N=20;
std::vector<std::unique_ptr<TimerClient>> clients;
int MsgCount=0;
class TimerClient// stdout是线程不安全的
{
public:
    TimerClient(const Address& serverAddr,EventLoop* loop):
    client_(serverAddr,loop)
    {
        client_.setMessageCallBack([this](TcpConnectionPtr,string_view){
            MsgCount++;
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
        disConnectStamp_=HTimeStamp::now();
        client_.disconnect();
    }
    HTimeStamp DisConnectStamp()
    {
        return disConnectStamp_;
    }
private:
    TcpClient client_;
    HTimeStamp disConnectStamp_;
};
int main()
{ 
    Address serverAddr("127.0.0.1",8080);
    EventLoop loop;

    for(int i=0;i<N;i++)
    {
        clients.emplace_back(new TimerClient(serverAddr,&loop));
        
    }
    std::thread t([&loop](){
        loop.loop();
    });
    for(int i=0;i<N;++i)
    {
        clients[i]->connect();
    }
    sleep(10);
    for(int i=0;i<N/2;++i)
    {
        clients[i]->send("hello world",11);
    }
    sleep(60); 
    assert(MsgCount==N);
    for(int i=0;i<N/2;++i)
    {
        for(int j=N/2;j<N;++j)
        {
            assert(clients[i]->DisConnectStamp()>clients[j]->DisConnectStamp());
        }
    }
    loop.quit();
    t.join();
    return 0;
}