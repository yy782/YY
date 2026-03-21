// #include "../UDP/UdpConnection.h"
// #include <string>
// using namespace yy;
// using namespace yy::net;
// class UdpTestClient {
// public:
//     UdpTestClient(EventLoop* loop, const char* serverHost, unsigned short serverPort)
//         : loop_(loop), serverAddr_(serverHost, serverPort) {
        
//         // 创建UDP客户端（连接到指定服务器）
//         client_ = UdpConnection::createConnection(loop, serverHost, serverPort);
        
//         // 设置消息回调 - 处理服务器返回的数据
//         client_->onMessage([this](UdpConnection::UdpConnectionPtr conn,
//                                   stringPiece data,
//                                   const Address& peer) {
//             std::cout << "Client received echo: " << std::string(data.data(), data.size()) 
//                       << " (from " << peer.sockaddrToString() << ")" << std::endl;
            
//             // 收到5条消息后关闭
//             if (++recvCount_ >= 5) {
//                 std::cout << "Received 5 messages, closing..." << std::endl;
//                 conn->close();
//                 loop_->quit();  // 退出事件循环
//             }
//         });
        
//         // 设置错误回调
//         client_->onError([](UdpConnection::UdpConnectionPtr conn) {
//             std::cout << "Client error occurred" << std::endl;
//         });
//     }
    
//     // 发送消息到服务器
//     void sendMessage(const std::string& msg) {
//         if (!client_->isClosed()) {
//             client_->send(msg);
//             std::cout << "Client sent: " << msg << std::endl;
//         }
//     }
    
//     // 获取服务器地址
//     Address getServerAddr() const { return serverAddr_; }
    
// private:
//     EventLoop* loop_;
//     UdpConnection::UdpConnectionPtr client_;
//     Address serverAddr_;
//     int recvCount_ = 0;
// };

// int main() {
//     EventLoop loop;
    
//     // 连接到本地服务器的8888端口
//     UdpTestClient client(&loop, "127.0.0.1", 8888);
    
//     // 启动事件循环，稍后发送消息
//     // loop.runAfter(1.0, [&]() {
//     //     // 发送几条测试消息
//     //     client.sendMessage("Hello");
//     //     client.sendMessage("World");
//     //     client.sendMessage("UDP Test");
//     //     client.sendMessage("Message 4");
//     //     client.sendMessage("Message 5");
//     // });
    
//     std::cout << "UDP Client started, connecting to 127.0.0.1:8888" << std::endl;
    
//     // 运行事件循环
//     loop.loop();
    
//     return 0;
// }

