#include "../UDP/UdpConnection.h"
#include <string>
#include "../include/SignalHandler.h"
using namespace yy;
using namespace yy::net;
//./UdpServer
class UdpEchoServer {
public:
    UdpEchoServer(EventLoop* loop, const char* host, unsigned short port)
        : loop_(loop) {
        
        // 创建UDP服务器
        server_ = UdpConnection::createServer(loop, host, port);
        
        // 设置消息回调 - 收到消息后原样返回
        server_->onMessage([this](UdpConnection::UdpConnectionPtr conn, 
                                  stringPiece data, 
                                  const Address& peer) {
            std::cout << "Server received " << data.size() << " bytes from " 
                      << peer.sockaddrToString()<< ": " << std::string(data.data(), data.size()) 
                      << std::endl;
            
            // 原样返回给客户端
            conn->sendTo(data.data(), data.size(), &peer);
        });
        std::cout << "UDP Echo server started on " << host << ":" << port << std::endl;
    }
    void stop()
    {
        server_->close();
    }
private:
    EventLoop* loop_;
    UdpConnection::UdpConnectionPtr server_;
};

int main() {
    EventLoop loop;
    std::cout<<getpid()<<std::endl;
    UdpEchoServer server(&loop, "127.0.0.1", 8888);
    Signal::signal(SIGTERM,[&server,&loop](){
        loop.quit();
        server.stop();
    });    
    
    
    // 启动事件循环
    loop.loop();
    
    return 0;
}
