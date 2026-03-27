#ifndef _YY_NET_TCPCONNECTION_H_
#define _YY_NET_TCPCONNECTION_H_
#include "../Common/noncopyable.h"
#include "InetAddress.h"
#include "EventHandler.h"
#include "TcpBuffer.h"
#include <memory>
#include "../Common/TimeStamp.h"
#include "Codec.h"
#include "AutoContext.h"
#include "EventLoop.h"

namespace yy
{
namespace net
{
class TcpConnection;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;    
class TcpConnection:noncopyable,
                    public std::enable_shared_from_this<TcpConnection>
{
public:
    typedef TcpBuffer Buffer;
    
    typedef Buffer::CharContainer CharContainer;
    typedef std::function<void(TcpConnectionPtr)> ServicesConnectionCallBack;
    typedef std::function<void(TcpConnectionPtr)> CloseCallBack;
    typedef std::function<void(TcpConnectionPtr)> ServicesMessageCallBack;
    typedef std::function<void(TcpConnectionPtr,char oob_buf[1])> ServicesExceptCallBack;
    typedef std::function<void(TcpConnectionPtr)> ServicesWriteCompleteCallBack;
    typedef std::function<void(TcpConnectionPtr)> ServicesErrorCallBack;
    // typedef std::function<void(TcpConnectionPtr)> BackpressureAfterSendCallBack;
    // typedef std::function<void(TcpConnectionPtr)> BackpressureAfterReadCallBack; 
    typedef AutoContext ServicesData;



    TcpConnection()=delete;
    TcpConnection(int fd,const Address& addr,EventLoop* loop,const char* name="NoName");
    ~TcpConnection();// 构析函数不能触发回调了，TcpConnectionPtr不允许
    void ConnectSuccess();
    EventHandler* handler(){return &handler_;}    
    int fd()const{return handler_.fd();}
    const Address& addr()const{return addr_;}
    EventLoop* loop(){return loop_;}
    Buffer& sendBuffer(){return SendBuffer_;}
    Buffer& recvBuffer(){return RecvBuffer_;}
    
    bool isConnecting()const{return Connstatus_==ConnectStatus::Connecting;}


    template<typename Tag,typename... Args>
    typename Tag::ReturnType Extend(Tag(Args...));


    void setReading()
    {
        handler_.setReading();
    }
    void setExcept()
    {
        handler_.setExcept();
    }
    void setEvent(Event e);
    template<typename Callable>
    void setConnectCallBack(Callable&& cb) {
        SconnectCallback_ = std::forward<Callable>(cb);
    }

    template<typename Callable>
    void setMessageCallBack(Callable&& cb) {
        SmessageCallBack_ = std::forward<Callable>(cb);
    }

    template<typename Callable>
    void setWriteCompleteCallBack(Callable&& cb) {
        SwriteCompleteCallBack_ = std::forward<Callable>(cb);
    }

    template<typename Callable>
    void setCloseCallBack(Callable&& cb) {
        ScloseCallBack_ = std::forward<Callable>(cb);
    }

    template<typename Callable>
    void setErrorCallBack(Callable&& cb) {
        SerrorCallBack_ = std::forward<Callable>(cb);
    }

    template<typename Callable>
    void setExceptCallBack(Callable&& cb) {
        SexceptCallBack_ = std::forward<Callable>(cb);
    }

    // void setBackpressureCallback(BackpressureAfterSendCallBack cb1,BackpressureAfterReadCallBack cb2)
    // {
    //     BackpressureAfterSend_=std::move(cb1);
    //     BackpressureAfterRead_=std::move(cb2);
    // }

    void setTcpAlive(bool on,int idleSeconds=7200, 
                  int intervalSeconds=75,int maxProbes=9)
    {
        sockets::setKeepAlive(handler_.fd(),on,idleSeconds,intervalSeconds,maxProbes);
    }
    void setTcpNoDelay(bool on)
    {
        sockets::setTcpNoDelay(handler_.fd(),on);   
    }
    // @brief 这些是有多线程安全问题的
    void disconnect(); // @brief 这是我端主动关闭连接时的回调,关闭我方的写端    
    
    
    void send(const char* message,size_t len);
    void send(std::string&& message);
    void sendOutput(); //配合ProtoMsgCodec使用的接口，把缓冲区的数据发送出去 
    template<typename T>
    T& context(){ return data_.context<T>();}
    bool isConnected(){return Connstatus_==ConnectStatus::Connected;}



    enum class ConnectStatus
    {
        Connecting,
        Connected,
        DisConnecting, 
        DisConnected
    };
    // enum class BackpressureState
    // {
    //     kNormal,     
    //     kHighWaterMark
    // }; 
    // struct BackpressureConfig
    // {   
    //     static const size_t highWaterMark=64*1024;
    //     static const size_t lowWaterMark=16*1024;
    // };
    // BackpressureState getRecvBufferState()const{return RecvbpState_;}
    // BackpressureState getSendBufferState()const{return SendbpState_;}
    //void updateWaterMark();

    void handleETRead();
    // void handleBackpressureAfterSend();
    // void handleBackpressureAfterRead();
private:
    void sendInLoop(const char* message,size_t len);
    void disconnectInLoop();
    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();
    void handleException();
    //void parseMessagesWithCodec();


    

    int fd_;
    Address addr_; // @prief 对端的地址
    EventLoop* loop_;
    Buffer RecvBuffer_;
    //BackpressureState RecvbpState_{BackpressureState::kNormal};
    //BackpressureAfterSendCallBack BackpressureAfterSend_;
    Buffer SendBuffer_;
    //BackpressureState SendbpState_{BackpressureState::kNormal};
    //BackpressureAfterReadCallBack BackpressureAfterRead_;

    ServicesConnectionCallBack SconnectCallback_;
    ServicesMessageCallBack SmessageCallBack_;
    ServicesWriteCompleteCallBack SwriteCompleteCallBack_;
    CloseCallBack ScloseCallBack_; 
    ServicesErrorCallBack SerrorCallBack_;
    ServicesExceptCallBack SexceptCallBack_;
    ServicesData data_;

    std::atomic<ConnectStatus> Connstatus_;

    
    EventHandler handler_;
    bool isET={false};


};



// enum class BufferBackpressureStrategy
// {
//     kDiscard,         /**< 丢弃新数据 - 适用于实时数据（如音视频流、监控数据），允许丢包以保护内存 */ //通用
//     kCloseConnection, /**< 断开连接 - 极端保护措施，用于检测到恶意客户端或资源严重受限时 */
//     kPass,             /**< 不处理 - 透传策略，继续追加数据，需要业务层自行保证不会OOM */   
    
//     kThrottle,        /**< 限速发送 - 令牌桶算法控制发送速率，适用于带宽控制和流量整形 */  // Send Buffer

//     kBackoff         /**< 停止读取 - 反向压力机制，暂停从socket读取新数据，最推荐的优雅策略 */ // Recv Buffer
// };

// // 主模板声明（必须有！）
// template<BufferBackpressureStrategy strategy>
// void handleAfterSend(TcpConnectionPtr conn);

// // 特化声明
// template<>
// void handleAfterSend<BufferBackpressureStrategy::kDiscard>(TcpConnectionPtr conn);

// template<>
// void handleAfterSend<BufferBackpressureStrategy::kCloseConnection>(TcpConnectionPtr conn);

// template<>
// void handleAfterSend<BufferBackpressureStrategy::kPass>(TcpConnectionPtr conn);

// // 主模板声明
// template<BufferBackpressureStrategy strategy>
// void handleAfterRecv(TcpConnectionPtr conn);

// // 特化声明
// template<>
// void handleAfterRecv<BufferBackpressureStrategy::kDiscard>(TcpConnectionPtr conn);

// template<>
// void handleAfterRecv<BufferBackpressureStrategy::kCloseConnection>(TcpConnectionPtr conn);

// template<>
// void handleAfterRecv<BufferBackpressureStrategy::kPass>(TcpConnectionPtr conn);

// template<>
// void handleAfterRecv<BufferBackpressureStrategy::kBackoff>(TcpConnectionPtr conn);
// }    
}
}

#endif