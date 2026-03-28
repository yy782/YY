#include <google/protobuf/io/zero_copy_stream.h>

#include "../TcpClient.h"
#include "../EventLoopThread.h"
#include "student.pb.h"
#include "../protobuf/ProtoMsg.h"
#include <memory>
using namespace yy;
using namespace yy::net;
//./ProtoBufTestClient
// 客户端类

bool isConnected=true;

class MyClient {
public:
    MyClient(const Address& addr,EventLoop* loop) 
        : client_(addr,loop)
    {
        client_.setConnectionCallback([this](int Cfd,const Address& Caddr,EventLoop* Cloop){
            auto conn=TcpConnection::accept(Cfd,Caddr,Cloop,Event(LogicEvent::Read));
            
            this->sendStudent(conn);
            conn->setMessageCallBack([this](TcpConnectionPtr con){
                onMessage(con);
            });
            conn->setCloseCallBack([this,Cloop](TcpConnectionPtr){
                isConnected=false;
            });
            return conn;
        });
    }
    
    void connect() 
    {
        client_.connect();
    
    }
    void sendStudent(TcpConnectionPtr conn) 
    {
        demo::Student student;
        student.set_name("张三");
        student.set_age(20);
        student.set_school("北京大学");
        
        ProtoMsgCodec::encode(&student,conn->sendBuffer()); 
        conn->sendOutput();
        
        std::cout << "客户端发送Student消息: " << student.name() << std::endl;
    }    
private:

    
    void onMessage(TcpConnectionPtr conn) 
    {
        // 接收服务器响应
        TcpBuffer& buf = conn->recvBuffer();    
        while (ProtoMsgCodec::msgComplete(buf)) 
        {
            auto msg =std::unique_ptr<Message>(ProtoMsgCodec::decode(buf));
            
            if (msg) {
                if (msg->GetDescriptor() == demo::StudentResponse::descriptor()) {
                    demo::StudentResponse* resp = 
                        dynamic_cast<demo::StudentResponse*>(msg.get());
                    std::cout << "客户端收到服务器响应: " 
                              << resp->message() << std::endl;        
                }
                client_.disconnect();
            }
        }
    }
    TcpClient client_;
};

int main()
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    EventLoopThread thread;
    MyClient client(Address(8080,true),thread.run());
    client.connect();
    sleep(1);
    while(isConnected)
    {
        sleep(1);
    }
    thread.stop();
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
