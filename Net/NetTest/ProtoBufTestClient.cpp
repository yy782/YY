#include <google/protobuf/io/zero_copy_stream.h>

#include "../TcpClient.h"

#include "student.pb.h"
#include "../protobuf/ProtoMsg.h"
#include <memory>
using namespace yy;
using namespace yy::net;

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

int main()
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    EventLoop loop;
    MyClient client(Address(8080,true),&loop);
    client.connect();
    loop.loop();
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
