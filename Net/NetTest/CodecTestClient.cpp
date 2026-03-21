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
        client_.setMessageCallBack(bind(&CodecTestClient::handleMessage,this,_1));
        client_.setCloseCallBack(bind(&CodecTestClient::handleClose,this,_1));
        client_.setConnectionCallback(bind(&CodecTestClient::handleConnected,this,_1));
    }
    void handleConnected(TcpConnectionPtr)
    {
        std::cout<<"Connected to server"<<std::endl;
        // 连接成功后发送消息
        std::cout<<"Sending Hello 1"<<std::endl;
        send("Hello 1");
        std::cout<<"Sending Hello 2"<<std::endl;
        send("Hello 2");
        std::cout<<"Sending Hello 3"<<std::endl;
        send("Hello 3");
        std::cout<<"Sending Hello 4"<<std::endl;
        send("Hello 4");
        std::cout<<"All messages sent"<<std::endl;
    }
    void send(const std::string& msg)
    {
        LineCodec::encode(stringPiece(msg),client_.getSendBuffer());
        client_.sendOutput();
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
        int p=1;
        while(p>0)
        {
            p=LineCodec::tryDecode(buffer.getReadView(),msg);
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
    void handleClose(TcpConnectionPtr conn) // 对端主动关闭时的回调
    {
        conn->send("bye\n",4);
        exit(0);
    }
    bool isConnected(){return client_.isConnected();}
private:
    TcpClient client_;
    
};


int main()
{
    Address addr(8080,true);
    EventLoopThread thread;
    CodecTestClient client(addr,thread.run());
    client.connect(); 

    while(client.isConnected())
    {
        sleep(1);
    }        
}