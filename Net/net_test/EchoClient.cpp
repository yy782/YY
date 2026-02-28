#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include "../net.h"
using namespace std;

//./EchoClient
using namespace yy;
using namespace yy::net;
class EchoClient
{
public:
    EchoClient(const Address& serverAddr):
    client_(serverAddr)
    {
        client_.setMessageCallBack(bind(&EchoClient::handleMessage,this,_1));
        client_.setCloseCallBack(bind(&EchoClient::handleClose,this,_1));
        client_.setRMessageBorder([](TcpConnection::CharContainer msg){
            auto it=std::find(msg.begin(),msg.end(),'\n');
            if(it!=msg.end())
            {
                return TcpServer::CharContainer(msg.begin(),it+1);
            }
            else 
            {
                return TcpServer::CharContainer();
            }            
        });
        client_.setRMessageBorder([](TcpConnection::CharContainer msg){
            auto it=std::find(msg.begin(),msg.end(),'\n');
            if(it!=msg.end())
            {
                return TcpServer::CharContainer(msg.begin(),it+1);
            }
            else 
            {
                return TcpServer::CharContainer();
            }
        });

    }
    void send(const std::string& msg)
    {
        client_.send(msg.c_str(),msg.size());
    }
    void connect()
    {
        client_.connect();
        cout<<"connect success "<<client_.getPeerAddr().sockaddrToString()<<endl;
    }
    void disconnect()
    {
        client_.disconnect();
    }
    void handleMessage(TcpConnectionPtr conn)
    {
        cout<<"recv:"<<conn->recv().data()<<endl;
    }
    void handleClose(TcpConnectionPtr conn)
    {
        conn->send("bye\n",4);
    }
private:
    TcpClient client_;
};

int main(int argc,char* argv[])
{    string port;
    if(argc!=2)
    {
        port="8080";
    }
    else
    {
        port=argv[1];
    }
    Address serverAddr("127.0.0.1",std::stoi(port));
    cout<<serverAddr.sockaddrToString()<<endl;
    EchoClient client(serverAddr);
    client.connect();
    std::string line;
    while(getline(cin,line))
    {
        line+='\n';
        client.send(line);
    }
    return 0;
}