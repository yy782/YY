#include <string.h>
#include <unistd.h>
#include "../TcpClient.h"
#include "../Codec.h"
#include "../ConfigCenter.h"
using namespace yy;
using namespace yy::net;

// ./EchoClient 
class EchoClient// stdout是线程不安全的
{
public:
    EchoClient(const Address& serverAddr,EventLoop* loop):
    client_(serverAddr,loop),
    stdIn_(0,loop)
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
        string_view msg;
        while(true)
        {
            const char* last=buffer.findBorder("\n",1);
            if(last == buffer.peek() + buffer.get_readable_size()) // 没有找到\n
            {
                return;
            }
            msg=string_view(buffer.peek(),last);
            
            
            std::cout<<"recv:"<<msg.data()<<std::endl;
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
                exit(0);
            }
            else 
            {

                line+="\n";
                client_.send(line.c_str(),line.size());  
              
              
            }
            
        }
    }
    void handleClose(TcpConnectionPtr conn) // 对端主动关闭时的回调
    {
        conn->send("bye\n",4);
        exit(0);
    }
    bool isConnected(){return client_.isConnected();}
private:
    TcpClient client_;
    EventHandler stdIn_;
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
    Address addr(host.c_str(),port);
    EventLoop client_loop;
    EchoClient client(addr,&client_loop);
    client.connect(); 

    client_loop.loop(); // 应该让loop在其他线程先connect运行，这里简略了
}