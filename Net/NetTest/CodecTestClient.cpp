#include <string.h>
#include <unistd.h>
#include "../TcpClient.h"
#include "../Codec.h"
#include "../ConfigCenter.h"
#include "../EventLoopThread.h"
using namespace yy;
using namespace yy::net;
int MsgCount=0;
// ./CodecTestClient
class CodecTestClient// stdout是线程不安全的
{
public:
    CodecTestClient(const Address& serverAddr,EventLoop* loop):
    client_(serverAddr,loop)
    {
        client_.setConnectionCallback([this](int Cfd,const Address& Caddr,EventLoop* Cloop)
        {
            auto conn=TcpConnection::accept(Cfd,Caddr,Cloop,Event(LogicEvent::Read));
            conn->setMessageCallBack([this](TcpConnectionPtr con){
                handleMessage(con);
            });
            handleConnected(conn);
            return conn;
        });
    }
    void handleConnected(TcpConnectionPtr con)
    {
        
        std::cout<<"Connected to server"<<std::endl;
        // 连接成功后发送消息
        std::cout<<"Sending Hello 1"<<std::endl;
        send("Hello 1",con);
        std::cout<<"Sending Hello 2"<<std::endl;
        send("Hello 2",con);
        std::cout<<"Sending Hello 3"<<std::endl;
        send("Hello 3",con);
        std::cout<<"Sending Hello 4"<<std::endl;
        send("Hello 4",con);
        std::cout<<"All messages sent"<<std::endl;
        
    }
    void send(const std::string& msg,TcpConnectionPtr con)
    {
        LineCodec::encode(stringPiece(msg),con->sendBuffer());
        con->sendOutput();
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
        TcpBuffer& buffer=conn->recvBuffer();
        stringPiece msg;
        int p=1;
        while(p>0)
        {
            p=LineCodec::tryDecode(buffer.readView(),msg);
            if(p<=0)
            {
                return;
            }
            std::cout<<"recv:"<<std::string(msg.data(),msg.size())<<std::endl;
            buffer.consume(p); 
            ++MsgCount;
            if(MsgCount==4)
            {
                disconnect();
            }
        }

    }
    bool isConnected(){return client_.isConnected();}
    bool isConnecting(){return client_.isConnecting();}
private:
    TcpClient client_;
    
};


int main()
{
    Address addr(8080,true);
    EventLoopThread thread;
    CodecTestClient client(addr,thread.run());
    client.connect(); 
    sleep(1); // 等等线程2处理连接
    while(client.isConnected()||client.isConnecting())
    {
        sleep(1);
    } 
    thread.stop();
}