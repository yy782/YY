// #include "../UDP/UdpConnection.h"
// #include <string>
// using namespace yy;
// using namespace yy::net;

// class UdpEchoServer {
// public:
//     UdpEchoServer(EventLoop* loop, const char* host, unsigned short port)
//         : loop_(loop) {
        
//         // 创建UDP服务器
//         server_ = UdpConnection::createServer(loop, host, port);
        
//         // 设置消息回调 - 收到消息后原样返回
//         server_->onMessage([this](UdpConnection::UdpConnectionPtr conn, 
//                                   stringPiece data, 
//                                   const Address& peer) {
//             std::cout << "Server received " << data.size() << " bytes from " 
//                       << peer.sockaddrToString()<< ": " << std::string(data.data(), data.size()) 
//                       << std::endl;
            
//             // 原样返回给客户端
//             conn->sendTo(data.data(), data.size(), peer);
//         });
        
//         设置错误回调
//         server_->onError([](UdpConnection::UdpConnectionPtr conn) {
//             std::cout << "Server error occurred" << std::endl;
//         });
        
//         std::cout << "UDP Echo server started on " << host << ":" << port << std::endl;
//     }
    
// private:
//     EventLoop* loop_;
//     UdpConnection::UdpConnectionPtr server_;
// };

// int main() {
//     EventLoop loop;
    
//     // 创建echo服务器，监听本地8888端口
//     UdpEchoServer server(&loop, "0.0.0.0", 8888);
    
//     // 启动事件循环
//     loop.loop();
    
//     return 0;
// }
int main()
{
    return 0;
}