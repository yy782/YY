#include "../UDP/UdpConnection.h"
#include <string>
#include <algorithm>
#include "../TimerQueue.h"
using namespace yy;
using namespace yy::net;
//./UdpClient 
class UdpTestClient {
public:
    UdpTestClient(EventLoop* loop, const char* serverHost, unsigned short serverPort)
        : loop_(loop), serverAddr_(serverHost, serverPort) {
        client_ = UdpConnection::createConnection(loop, serverHost, serverPort);
        
        // 设置消息回调 - 处理服务器返回的数据
        client_->onMessage([this](UdpConnection::UdpConnectionPtr conn,
                                  stringPiece data,
                                  const Address& peer) {
            std::string msg(data.data(), data.size());
            std::cout << "Client received echo: " << msg
                      << " (from " << peer.sockaddrToString() << ")" << std::endl;
            recvCount_+=static_cast<int>(std::count(msg.begin(),msg.end(),'\n'));                      
            if (recvCount_ >= 5) {
                std::cout << "Received 5 messages, closing..." << std::endl;
                conn->close();
                loop_->quit(); 
            }
        });
    }
    void sendMessage(const std::string& msg) {
        if (!client_->isClosed()) {
            client_->sendTo(msg);
            std::cout << "Client sent: " << msg << std::endl;
        }
    }
    
    Address getServerAddr() const { return serverAddr_; }
    
private:
    EventLoop* loop_;
    UdpConnection::UdpConnectionPtr client_;
    Address serverAddr_;
    int recvCount_ = 0;
};

int main() {
    EventLoop loop;
    
    // 连接到本地服务器的8888端口
    UdpTestClient client(&loop, "127.0.0.1", 8888);
    
    loop.runTimer<LowPrecision>([&]() {
        // 发送几条测试消息
        client.sendMessage("Hello\n");
        client.sendMessage("World\n");
        client.sendMessage("UDP Test\n");
        client.sendMessage("Message 4\n");
        client.sendMessage("Message 5\n");
    },2s,1);
    std::cout << "UDP Client started, connecting to 127.0.0.1:8888" << std::endl;
    loop.loop();
    
    return 0;
}

