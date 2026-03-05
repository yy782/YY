

#include <google/protobuf/io/zero_copy_stream.h>

#include "../net.h"

#include "student.pb.h"
#include "../ProtocolCodec.h"
using namespace yy;
using namespace yy::net;
class MyServer {
public:
    MyServer(const Address& addr) 
        : server_(addr, 4)  // 4个线程
    {
        // 设置消息回调
        server_.setMessageCallBack(
            std::bind(&MyServer::onMessage,this, 
                      _1,_2));
    }
    
    void start() {
        server_.loop();
    }
    
private:
    void onMessage(TcpConnectionPtr conn,Buffer* buffer) 
    {
        
        
        ProtocolCodec  codec(buffer);
        
        // 根据消息类型解析
        demo::Student student;
        if (codec.decode(student)) 
        {
            std::cout << "收到Student消息:" << std::endl;
            std::cout << "  姓名: " << student.name() << std::endl;
            std::cout << "  年龄: " << student.age() << std::endl;
            std::cout << "  学校: " << student.school() << std::endl;
            
            sendResponse(conn, student);
        }
    }
    
    void sendResponse(TcpConnectionPtr conn, const demo::Student& student) {
        // 准备响应消息
        demo::StudentResponse response;
        response.set_success(true);
        response.set_message("收到学生信息: " + student.name());
        
        // 获取连接的输出Buffer（假设TcpConnection有outputBuffer）
        Buffer& output = conn->getWriteBuffer();  // 你需要有这个接口
        
        // 创建编码器
        ProtocolCodec codec(&output);
        
        // 编码并发送
        if (codec.encode(response)) {
            conn->send(output.peek(), output.get_readable_size());
            codec.reset();  // 重置Buffer
        }
    }
    
    TcpServer server_;
};
class MyClient {
public:
    MyClient(const Address& serverAddr)
        : client_(serverAddr)
    {       
        client_.setMessageCallBack(
            std::bind(&MyClient::onConnect, this,_1));
        client_.setMessageCallBack(std::bind(&MyClient::onMessage, this,
                      _1,_2));
    }
    
    void connect() {
        client_.connect();
    }
    
    // 发送Student消息的接口
    void sendStudent(TcpConnectionPtr conn,const std::string& name, int age, const std::string& school) {
        demo::Student student;
        student.set_name(name);
        student.set_age(age);
        student.set_school(school);
        
        Buffer& output=conn->getWriteBuffer();
        
        ProtocolCodec codec(&output);
        if (codec.encode(student)) {
            // 发送数据
            conn->send(output.peek(), output.get_readable_size());
            codec.reset();  // 发送后重置
            std::cout << "发送Student消息: " << name << std::endl;
        }
    }
    
private:
    void onConnect(TcpConnectionPtr conn) {

        sendStudent(conn,"张三", 20, "北京大学");
        
    }
    
    void onMessage(TcpConnectionPtr conn,Buffer* buffer) 
    {
        ProtocolCodec codec(buffer);
        demo::StudentResponse response;
        
        if(codec.decode(response)) 
        {
            std::cout << "服务器响应: " << response.message() << std::endl;
        }
    }
    
    TcpClient client_;
};
int main() {
    GOOGLE_PROTOBUF_VERIFY_VERSION;  // 初始化protobuf
    
    Address serverAddr("127.0.0.1", 8888);
    MyServer server(serverAddr);
    
    std::thread serverThread([&server]() {
        std::cout << "服务器启动..." << std::endl;
        server.start();
    });
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    MyClient client(serverAddr);
    client.connect();
    
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    google::protobuf::ShutdownProtobufLibrary();
    
    return 0;
}
