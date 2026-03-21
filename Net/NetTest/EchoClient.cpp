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
    stdIn_(0,loop_),
    thread_(thread)
    {
        client_.setMessageCallBack(bind(&EchoClient::handleMessage,this,_1));
        client_.setCloseCallBack(bind(&EchoClient::handleClose,this,_1)); 
        stdIn_.setReadCallBack([this](){handleRead();});
        stdIn_.set_name("stdIn");
        stdIn_.setReading();
         
    }
    void send(const std::string& msg)
    {
        client_.send(msg.c_str(),msg.size());
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
    void handleMessage(TcpConnectionPtr conn) // @note 如果需要，要判断对端断开连接的消息
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
                client_.send(line.c_str(),line.size());  
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
    // 主线程一直休眠
    sleep(1);
    while(isConnected.load())
    {
        sleep(1);
    }

    thread.stop();
   
}