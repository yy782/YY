#include <string.h>
#include <unistd.h>
#include "../net.h"
using namespace yy;
using namespace yy::net;

class EchoClient// stdout是线程不安全的
{
public:
    EchoClient(const Address& serverAddr,EventLoop* loop):
    client_(serverAddr,loop)
    {
        client_.setMessageCallBack(bind(&EchoClient::handleMessage,this,_1));
        client_.setCloseCallBack(bind(&EchoClient::handleClose,this,_1));


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
        const char* last=buffer.findBorder("\n",1);
        std::string msg(buffer.peek(),last);
        std::cout<<"recv:"<<msg<<std::endl;
    }
    void handleRead()
    {
        std::string line;
        while(getline(std::cin,line))
        {
            line+='\n';
            client_.send(line.c_str(),line.size());
        }
    }
    void handleClose(TcpConnectionPtr conn) // 对端关闭时的回调
    {
        conn->send("bye\n",4);
        exit(0);
    }
    bool isConnected(){return client_.isConnected();}
private:
    TcpClient client_;
};


int main(int argc,char* argv[])
{
    Address addr;
    if(argc>1)
    {
        std::string ip=argv[1];
        std::string port=argv[2];
        addr=Address(ip.c_str(),atoi(port.c_str()));
    }
    else
    {
        addr=Address("127.0.0.1",8888);
    }
    
    EventLoop client_loop;
    EchoClient client(addr,&client_loop);
    client.connect(); 
}