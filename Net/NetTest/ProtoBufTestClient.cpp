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
class MyClient {
public:
    MyClient(const Address& addr,EventLoopThread* thread) 
        : client_(addr,thread->getEventLoop()),
        thread_(thread)
    {
        auto loop=thread->getEventLoop();
        client_.setMessageCallBack([this](TcpConnectionPtr conn){
            onMessage(conn);
        });
        client_.setCloseCallBack([this,loop](TcpConnectionPtr){
            loop->quit();
            exit(0);
        });
        client_.setConnectionCallback([this](TcpConnectionPtr conn){
            //client_.setEvent(EventType::ReadEvent|EventType::EV_ET);
            conn->setReading();
            this->sendStudent(conn);
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
        
        ProtoMsgCodec::encode(&student,conn->getSendBuffer());
        conn->sendOutput();
        
        std::cout << "客户端发送Student消息: " << student.name() << std::endl;
    }    
private:

    
    void onMessage(TcpConnectionPtr conn) 
    {
        // 接收服务器响应
        TcpBuffer& buf = conn->getRecvBuffer();
        while (ProtoMsgCodec::msgComplete(buf)) 
        {
            auto msg =std::unique_ptr<Message>(ProtoMsgCodec::decode(buf));
            
            if (msg) {
                if (msg->GetDescriptor() == demo::StudentResponse::descriptor()) {
                    demo::StudentResponse* resp = 
                        dynamic_cast<demo::StudentResponse*>(msg.get());
                    std::cout << "客户端收到服务器响应: " 
                              << resp->message() << std::endl;
                    Exit();          
                }
            }
        }
    }
    void Exit()
    {
        thread_->stop(); // 保证thread_先构析
        exit(0);
    }
    TcpClient client_;
    EventLoopThread* thread_;
};

int main()
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    EventLoopThread thread;
    MyClient client(Address(8080,true),&thread);
    
    thread.run();
    client.connect();
    
    // 主线程睡眠一段时间，让EventLoop线程有时间处理连接
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
