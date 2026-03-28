#include <string.h>
#include <unistd.h>
#include "../TcpClient.h"
#include "../Codec.h"
#include "../ConfigCenter.h"
#include "../EventLoopThread.h"
using namespace yy;
using namespace yy::net;
bool isConnected=true;
// ./EchoClient 
class EchoClient// stdout是线程不安全的
{
public:
    EchoClient(const Address& serverAddr,EventLoopThread* thread,bool isET):
    loop_(thread->run()),
    client_(serverAddr,loop_),
    stdIn_(0,loop_,"stdIn",Event(LogicEvent::Edge|LogicEvent::Read)),
    thread_(thread)
    {
        client_.setConnectionCallback([this,isET](int Cfd,const Address& Caddr,EventLoop* Cloop)
        {
            auto conn=TcpConnection::accept(Cfd,Caddr,Cloop,Event(LogicEvent::Read));
            if(isET)
            {
               conn->setEvent(Event(LogicEvent::Read|LogicEvent::Edge));
            }
            conn->setMessageCallBack([this](TcpConnectionPtr con){
                handleMessage(con);
            });
            conn->setCloseCallBack([this](TcpConnectionPtr con){
                handleClose(con);
            });
            return conn;
        }); 
        stdIn_.setReadCallBack([this](){handleRead();});
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
        TcpBuffer& buffer=conn->recvBuffer();   
        stringPiece msg;
        while(true)
        {
            const char* last=buffer.findBorder("\n",1);
            if(last == buffer.peek() + buffer.readable_size()) // 没有找到\n
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
        isConnected=false;
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
    // SyncLog::getInstance("../CliLog.log").getFilter() 
    //     .set_global_level(LOG_LEVEL_DEBUG) 
    //     .set_module_enabled("TCP")
    //     .set_module_enabled("SYSTEM")
    //     .set_module_enabled("HTTP")
    //     .set_module_enabled("CLIENT")
    //     .set_module_enabled("EVENT")
    //     .set_module_enabled("LOOP")
    //     ;

    std::string host=config.get("server","host");
    int port=static_cast<int>(config.getInteger("server","port"));
    bool isET=config.getBoolean("server","isET");
    Address addr(host.c_str(),port);
    EchoClient client(addr,&thread,isET);
    client.connect(); 
    while(isConnected)
    {
        sleep(1);
    }

    thread.stop();
   
}