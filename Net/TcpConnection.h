#ifndef _YY_NET_TCPCONNECTION_H_
#define _YY_NET_TCPCONNECTION_H_
#include "../Common/noncopyable.h"
#include "InetAddress.h"
#include "EventHandler.h"
#include "TcpBuffer.h"
#include <memory>
#include <any>
#include "../Common/TimeStamp.h"
#include "Codec.h"
#include "AutoContext.h"
#include "EventLoop.h"
#include <string_view>
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
    typedef std::function<void(TcpConnectionPtr)> ServicesReadCallback;
    typedef std::function<void(TcpConnectionPtr,char oob_buf[1])> ServicesExceptCallBack;
    typedef std::function<void(TcpConnectionPtr)> ServicesWriteCompleteCallBack;
    typedef std::function<void(TcpConnectionPtr)> ServicesErrorCallBack;
    typedef std::function<void(TcpConnectionPtr)> BackpressureAfterSendCallBack;
    typedef std::function<void(TcpConnectionPtr)> BackpressureAfterReadCallBack;
    typedef AutoContext ServicesData;



    TcpConnection()=delete;
    TcpConnection(int fd,const Address& addr,EventLoop* loop,const char* name="NoName");
    ~TcpConnection();// 构析函数不能触发回调了，TcpConnectionPtr不允许
    void ConnectSuccess()
    {
        assert(Connstatus_==ConnectStatus::Connecting);
        Connstatus_=ConnectStatus::Connected;
        handler_.tie(shared_from_this());
        if(SconnectCallback_)
        {
            SconnectCallback_(shared_from_this());
        }
    }
    EventHandler* getHandler(){return &handler_;}    
    int get_fd()const{return handler_.get_fd();}
    const Address& getAddr()const{return addr_;}
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
    void setEvent(Event e)
    {   
        handler_.set_event(e);  
    }
    void setEvent(LogicEvent e)
    {
        handler_.set_event(e);
    }
    void setConnectCallBack(ServicesConnectionCallBack cb){SconnectCallback_=std::move(cb);}
    void setMessageCallBack(ServicesMessageCallBack cb){SmessageCallBack_=std::move(cb);}
    void setReadCallBack(ServicesReadCallback cb){SreadCallback_=std::move(cb);}
    void setWriteCompleteCallBack(ServicesWriteCompleteCallBack cb){SwriteCompleteCallBack_=std::move(cb);}
    void setCloseCallBack(CloseCallBack cb){ScloseCallBack_=std::move(cb);} // @brief 这是对端关闭的回调
    void setErrorCallBack(ServicesErrorCallBack cb){SerrorCallBack_=std::move(cb);}
    void setExceptCallBack(ServicesExceptCallBack cb){SexceptCallBack_=std::move(cb);}

    void setBackpressureCallback(BackpressureAfterSendCallBack cb1,BackpressureAfterReadCallBack cb2)
    {
        BackpressureAfterSend_=std::move(cb1);
        BackpressureAfterRead_=std::move(cb2);
    }

    void setTcpAlive(bool on,int idleSeconds=7200, 
                  int intervalSeconds=75,int maxProbes=9)
    {
        sockets::setKeepAlive(handler_.get_fd(),on,idleSeconds,intervalSeconds,maxProbes);
    }
    void setTcpNoDelay(bool on)
    {
        sockets::setTcpNoDelay(handler_.get_fd(),on);
    }
    // @brief 这些是有多线程安全问题的
    void disconnect(); // @brief 这是我端主动关闭连接时的回调,关闭我方的写端    
    
    
    void send(const char* message,size_t len);
    void send(std::string&& message);
    void send(const std::string& message);
    //void send(const std::string_view& msg);
    void sendOutput(); //配合ProtoMsgCodec使用的接口，把缓冲区的数据发送出去 
    template<typename T>
    T& context(){ return data_.context<T>();}
    bool isConnected(){return Connstatus_==ConnectStatus::Connected;}

    Buffer& getSendBuffer(){return SendBuffer_;}
    Buffer& getRecvBuffer(){return RecvBuffer_;}

    enum class ConnectStatus
    {
        Connecting,
        Connected,
        DisConnecting, 
        DisConnected
    };
    enum class BackpressureState
    {
        kNormal,     
        kHighWaterMark
    }; 
    struct BackpressureConfig
    {   
        static const size_t highWaterMark=64*1024;
        static const size_t lowWaterMark=16*1024;
    };
    BackpressureState getRecvBufferState()const{return RecvbpState_;}
    BackpressureState getSendBufferState()const{return SendbpState_;}
    void updateWaterMark();

    void handleETRead(ServicesReadCallback cb);
    void RhandleClose(){handleClose();}
    void RhandleError(){handleError();}
    void handleBackpressureAfterSend();
    void handleBackpressureAfterRead();
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
    BackpressureState RecvbpState_{BackpressureState::kNormal};
    BackpressureAfterSendCallBack BackpressureAfterSend_;
    Buffer SendBuffer_;
    BackpressureState SendbpState_{BackpressureState::kNormal};
    BackpressureAfterReadCallBack BackpressureAfterRead_;

    ServicesConnectionCallBack SconnectCallback_;
    ServicesReadCallback SreadCallback_;
    ServicesMessageCallBack SmessageCallBack_;
    ServicesWriteCompleteCallBack SwriteCompleteCallBack_;
    CloseCallBack ScloseCallBack_; 
    ServicesErrorCallBack SerrorCallBack_;
    ServicesExceptCallBack SexceptCallBack_;
    ServicesData data_;

    std::atomic<ConnectStatus> Connstatus_;

    
    EventHandler handler_;
};



enum class BufferBackpressureStrategy
{
    kDiscard,         /**< 丢弃新数据 - 适用于实时数据（如音视频流、监控数据），允许丢包以保护内存 */ //通用
    kCloseConnection, /**< 断开连接 - 极端保护措施，用于检测到恶意客户端或资源严重受限时 */
    kPass,             /**< 不处理 - 透传策略，继续追加数据，需要业务层自行保证不会OOM */   
    
    kThrottle,        /**< 限速发送 - 令牌桶算法控制发送速率，适用于带宽控制和流量整形 */  // Send Buffer

    kBackoff         /**< 停止读取 - 反向压力机制，暂停从socket读取新数据，最推荐的优雅策略 */ // Recv Buffer
};

// 主模板声明（必须有！）
template<BufferBackpressureStrategy strategy>
void handleAfterSend(TcpConnectionPtr conn);

// 特化声明
template<>
void handleAfterSend<BufferBackpressureStrategy::kDiscard>(TcpConnectionPtr conn);

template<>
void handleAfterSend<BufferBackpressureStrategy::kCloseConnection>(TcpConnectionPtr conn);

template<>
void handleAfterSend<BufferBackpressureStrategy::kPass>(TcpConnectionPtr conn);

// 主模板声明
template<BufferBackpressureStrategy strategy>
void handleAfterRecv(TcpConnectionPtr conn);

// 特化声明
template<>
void handleAfterRecv<BufferBackpressureStrategy::kDiscard>(TcpConnectionPtr conn);

template<>
void handleAfterRecv<BufferBackpressureStrategy::kCloseConnection>(TcpConnectionPtr conn);

template<>
void handleAfterRecv<BufferBackpressureStrategy::kPass>(TcpConnectionPtr conn);

template<>
void handleAfterRecv<BufferBackpressureStrategy::kBackoff>(TcpConnectionPtr conn);
}    
}


#endif