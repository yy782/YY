#include <string.h>
#include <unistd.h>
#include "../TcpClient.h"
#include "../Codec.h"
#include "../ConfigCenter.h"
#include "../EventLoopThread.h"
using namespace yy;
using namespace yy::net;
std::atomic<bool> isConnected=true;
// ./EchoClient 
class EchoClient// stdout是线程不安全的
{
public:
    EchoClient(const Address& serverAddr,EventLoopThread* thread):
    loop_(thread->run()),
    client_(serverAddr,loop_),
    stdIn_(0,loop_,"stdIn"),
    thread_(thread)
    {
        client_.enableRetry();
        client_.setConnectionCallback([this](TcpConnectionPtr con){
            //client_.setEvent(EventType::ReadEvent|EventType::EV_ET);
            con->setReading();
        });
        client_.setMessageCallBack(bind(&EchoClient::handleMessage,this,_1));
        client_.setCloseCallBack(bind(&EchoClient::handleClose,this,_1)); 
        stdIn_.setReadCallBack([this](){handleRead();});
       
        stdIn_.set_event(LogicEvent::Edge|LogicEvent::Read);
         
    }
    ~EchoClient()
    {
        sockets::close(0);
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
    void handleMessage(TcpConnectionPtr conn) 
    {
        TcpBuffer& buffer=conn->getRecvBuffer();
        stringPiece msg;
        while(true)
        {
            const char* last=buffer.findBorder("\n",1);
            if(last == buffer.peek() + buffer.get_readable_size()) // 没有找到\n
            {
                return;
            }
            msg=stringPiece(buffer.peek(),last);
            
            
            std::cout<<"recv:"<<std::string(msg)<<std::endl;
            buffer.consume(msg.size()+1);             
        }
    }
    void handleRead()
    {
        std::string line;
        if(getline(std::cin,line))
        {
            if(line=="quit")
            {
                client_.disconnect();
            }
            else 
            {
                line+="\n";
                client_.getConnection()->send(line.c_str(),line.size());  
            }
            
        }
    }
    void handleClose(TcpConnectionPtr) 
    {
        isConnected.store(false);
    }
private:
    EventLoop* loop_;

    TcpClient client_;
    EventHandler stdIn_;
    EventLoopThread* thread_;
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
    EventLoopThread thread;
    
    std::string host=config.get("server","host");
    int port=static_cast<int>(config.getInteger("server","port"));
    Address addr(host.c_str(),port);
    EchoClient client(addr,&thread);
    client.connect(); 
    while(isConnected.load())
    {
        sleep(1);
    }

    thread.stop();
   
}