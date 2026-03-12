


#include <google/protobuf/io/zero_copy_stream.h>

#include "../net.h"

#include "student.pb.h"
#include "../protobuf/ProtoMsg.h"
#include <memory>
using namespace yy;
using namespace yy::net;
class MyServer {
public:
    MyServer(EventLoop* loop) 
        : server_(Address(8080,true),2,loop), loop_(loop)
    {
        dispatcher_.onMsg<demo::Student>(
            [this](TcpConnectionPtr conn, demo::Student* msg) {
                handleStudent(conn,msg);
            }
        );
        
        server_.setMessageCallBack([this](TcpConnectionPtr conn,string_view msg){
            onMessage(conn, msg);
        });
    }
    
    void start() 
    {
        server_.loop();
    }
    
private:
    void onMessage(TcpConnectionPtr conn,string_view msg) {
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
        
        // 发送响应
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

// 客户端类
class MyClient {
public:
    MyClient(const Address& addr,EventLoop* loop) 
        : client_(addr,loop)
    {
        
        client_.setMessageCallBack([this](TcpConnectionPtr conn,string_view){
            onMessage(conn);
        });
    }
    
    void connect() 
    {
        client_.connect();
        sendStudent();
    }
    
private:
    void sendStudent() 
    {
        demo::Student student;
        student.set_name("张三");
        student.set_age(20);
        student.set_school("北京大学");
        
        ProtoMsgCodec::encode(&student,client_.getSendBuffer());
        client_.sendOutput();
        
        std::cout << "客户端发送Student消息: " << student.name() << std::endl;
    }
    
    void onMessage(TcpConnectionPtr conn) 
    {
        // 接收服务器响应
        TcpBuffer& buf = conn->getRecvBuffer();
        while (ProtoMsgCodec::msgComplete(buf)) 
        {
            Message* msg = ProtoMsgCodec::decode(buf);
            if (msg) {
                if (msg->GetDescriptor() == demo::StudentResponse::descriptor()) {
                    demo::StudentResponse* resp = 
                        dynamic_cast<demo::StudentResponse*>(msg);
                    std::cout << "客户端收到服务器响应: " 
                              << resp->message() << std::endl;
                }
                delete msg;
            }
        }
    }
    TcpClient client_;
};

int main() {
    // 初始化protobuf
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    
    EventLoop loop;
    
    // 创建服务器
    MyServer server(&loop);
    server.start();
    
    // 在一个单独的线程中运行服务器（可选，这里直接在主线程运行）
    std::thread serverThread([&loop]() {
        loop.loop();
    });
    
    // 给服务器一点启动时间
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EventLoop loop2;
    MyClient client(Address(8080,true),&loop);
    client.connect();
    

    
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}