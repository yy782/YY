
#ifndef _ECHOCLIENT_H_
#define _ECHOCLIENT_H_

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

using namespace yy;
using namespace yy::net;
class EchoClient// stdout是线程不安全的
{
public:
    EchoClient(const Address& serverAddr):
    client_(serverAddr)
    {
        client_.setMessageCallBack(bind(&EchoClient::handleMessage,this,_1,_2));
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
    void handleMessage(TcpConnectionPtr conn,Buffer* buffer) // @note 如果需要，要判断对端断开连接的消息
    {
        char* last=buffer->findBorder("\n",1);
        std::string msg(buffer->peek(),last);
        cout<<"recv:"<<msg<<endl;
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



#endif