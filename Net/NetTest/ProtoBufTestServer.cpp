
//./ProtoBufTestServer

#include <google/protobuf/io/zero_copy_stream.h>

#include "../TcpServer.h"

#include "student.pb.h"
#include "../protobuf/ProtoMsg.h"
#include <memory>
#include <iostream>
using namespace yy;
using namespace yy::net;
class MyServer {
public:
    MyServer(EventLoop* loop) 
        :loop_(loop) ,server_(Address(8080,true),2,loop)
    {
        dispatcher_.onMsg<demo::Student>(
            [this](TcpConnectionPtr conn, demo::Student* msg) {
                handleStudent(conn,msg);
            }
        );
        
        server_.setMessageCallBack([this](TcpConnectionPtr conn){
            onMessage(conn);
        });
        server_.setConnectCallBack([](TcpConnectionPtr conn){
            std::cout<<"Connect!"<<std::endl;
            conn->setEvent(EventType::ReadEvent|EventType::EV_ET);
        });
    }
    
    void start() 
    {
        server_.loop();
    }
    void stop()
    {
        server_.stop();
    }
    
private:
    void onMessage(TcpConnectionPtr conn) {
        std::cout<<"onMessage "<<std::endl;
        TcpBuffer& buf = conn->getRecvBuffer();
        while (ProtoMsgCodec::msgComplete(buf)) {
            Message* msg = ProtoMsgCodec::decode(buf);
            if (msg) {
                dispatcher_.handle(conn,msg);
            }
        }
    }
    
    void handleStudent(TcpConnectionPtr conn,demo::Student* student) {
        std::cout << "服务器收到Student消息:" << std::endl;
        std::cout << "  姓名: " << student->name() << std::endl;
        std::cout << "  年龄: " << student->age() << std::endl;
        std::cout << "  学校: " << student->school() << std::endl;
        demo::StudentResponse response;
        response.set_success(true);
        response.set_message("收到学生信息: " + student->name());
        
        ProtoMsgCodec::encode(&response, conn->getSendBuffer());
        conn->sendOutput();
    }
    
    EventLoop* loop_;
    TcpServer server_;
    ProtoMsgDispatcher dispatcher_;
};


int main() {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

   
    EventLoop loop;
    MyServer server(&loop);
    server.start();
    loop.loop();

    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}