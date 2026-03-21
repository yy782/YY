#ifndef YY_NET_UDP_UDP_CONNECTION_H_
#define YY_NET_UDP_UDP_CONNECTION_H_
#include <memory>
#include "../../Common/noncopyable.h"
#include "../EventLoop.h"

#include "../EventHandler.h"
#include "../InetAddress.h"
#include "../AutoContext.h"
#include "../../Common/stringPiece.h"
#include "../../Common/TimeStamp.h"
namespace yy 
{
namespace net 
{


class UdpConnection : public std::enable_shared_from_this<UdpConnection>,
                      private noncopyable 
{
public:
    typedef std::shared_ptr<UdpConnection> UdpConnectionPtr;
    typedef std::function<void(UdpConnectionPtr,stringPiece, const Address&)> UdpMessageCallBack;
    typedef std::function<void(UdpConnectionPtr)> UdpErrorCallBack;
    typedef AutoContext ServicesData;

    // 创建客户端UDP连接（connect方式，固定对端）
    static UdpConnectionPtr createConnection(EventLoop* loop, 
                                             const char* host, 
                                             unsigned short port);
    
    // 创建服务端UDP（不connect，接收任意来源）
    static UdpConnectionPtr createServer(EventLoop* loop, 
                                         const char* bindHost, 
                                         unsigned short port);

    ~UdpConnection();

    // 设置消息回调
    void onMessage(UdpMessageCallBack cb);
    void onError(UdpErrorCallBack cb);

    // // 发送数据（给固定对端，适用于客户端模式）
    // void send(const char* buf, size_t len);
    // void send(const std::string& s);
    // void send(const char* s);

    // 发送数据到指定地址（适用于服务器模式）
    void sendTo(const char* buf, size_t len, const Address* dest=nullptr);
    void sendTo(const std::string& s, const Address* dest=nullptr);

    // 关闭连接
    void close();

    Address getPeerAddr() const;
    bool ispeerAlive()const{return ispeerAlive_;}

    // 状态查询
    bool isServer() const { return isServer_; }
    bool isClosed() const { return closed_; }
    void startHeartbeat(LTimeInterval interval,LTimeInterval MaxidleTime);
    EventLoop* getLoop() { return loop_; }

    // 上下文管理
    template <typename T>
    T& context() { return data_.context<T>(); }

private:
    UdpConnection(EventLoop* loop, int fd, const Address& local);

    void handleRead();
    void handleError();
    void sendInLoop(const char* buf, size_t len, const Address* dest);

    EventLoop* loop_;
    EventHandler handler_;
    Address local_;
    Address peer_;        // 仅客户端模式有效
    bool closed_;        // 对端是否已关闭
    bool isServer_;


    UdpMessageCallBack messageCallback_;
    UdpErrorCallBack errorCallback_;
    ServicesData data_;

    static const size_t kUdpPacketSize = 65536;  // 最大UDP包

    bool checkUdpAlive_={false};
    EventHandler timerHandler_;    
    LTimeStamp lastSendTimestamp_;
    LTimeStamp lastRecvTimestamp_;
    LTimeInterval MaxidleTime_;
    bool ispeerAlive_={true};
};   
}    
}
#endif