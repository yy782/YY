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
namespace yy
{
namespace net
{

    
class TcpConnection:noncopyable,
                    public std::enable_shared_from_this<TcpConnection>
{
public:
    typedef TcpBuffer Buffer;
    
    typedef Buffer::CharContainer CharContainer;
    typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
    typedef std::function<void(TcpConnectionPtr,string_view)> ServicesMessageCallBack;
    typedef std::function<void(TcpConnectionPtr,char oob_buf[1])> ServicesExceptCallBack;
    typedef std::function<void(TcpConnectionPtr)> ServicesWriteCompleteCallBack;
    typedef std::function<void(TcpConnectionPtr)> ServicesCloseCallBack;
    typedef std::function<void(TcpConnectionPtr)> ServicesErrorCallBack;

    typedef std::shared_ptr<CodecBase> CodecPtr;

    typedef std::function<void(TcpConnectionPtr)> BackpressureAfterSendCallBack;
    typedef std::function<void(TcpConnectionPtr)> BackpressureAfterReadCallBack;
    typedef AutoContext ServicesData;



    TcpConnection(int fd,const Address& addr,EventLoop* loop);
    TcpConnection(int fd,const Address& addr); // 服务端的构造函数
    ~TcpConnection();// 构析函数不能触发回调了，TcpConnectionPtr不允许
    void connect()
    {
        sockets::connect(handler_.get_fd(),addr_);
        assert(Connstatus_==ConnectStatus::DisConnected);
        Connstatus_=ConnectStatus::Connected;
    }
    static TcpConnectionPtr accept(int fd,const Address& addr)// @note 服务端用这个接口
    {
        auto conn=std::make_shared<TcpConnection>(fd,addr);
        conn->Connstatus_=ConnectStatus::Connected;
        return conn;
    }

    EventHandler* getHandler(){return &handler_;}    
    int get_fd()const{return fd_;}
    Address getAddr()const{return addr_;}


    void setReading()
    {
        handler_.setReading();
    
    }
    void setExcept()
    {
        handler_.setExcept();
    }
    void setMessageCallBack(ServicesMessageCallBack cb){SmessageCallBack_=std::move(cb);}
    void setWriteCompleteCallBack(ServicesWriteCompleteCallBack cb){SwriteCompleteCallBack_=std::move(cb);}
    void setCloseCallBack(ServicesCloseCallBack cb){ScloseCallBack_=std::move(cb);} // @brief 这是对端关闭的回调
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
    void setName(const char* name){handler_.set_name(name);}
    void setCodec(CodecPtr codec) 
    {
        codec_=codec;
    }

    
    // @brief 这些是有多线程安全问题的
    void disconnect(); // @brief 这是我端主动关闭连接时的回调,关闭我方的写端    
    
    
    void send(const char* message,size_t len);
    void sendOutput(); //配合ProtoMsgCodec使用的接口，把缓冲区的数据发送出去 
    template<typename T>
    T& context(){ return data_.context<T>();}
    bool isConnected(){return Connstatus_==ConnectStatus::Connected;}

    Buffer& getSendBuffer(){return SendBuffer_;}
    Buffer& getRecvBuffer(){return RecvBuffer_;}

    enum class ConnectStatus
    {
        Connected=1,
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
private:
    void sendInLoop(const char* message,size_t len);
    void disconnectInLoop();
    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();
    void handleException();
    void parseMessagesWithCodec();

    void handleBackpressureAfterSend();
    void handleBackpressureAfterRead();
    


    Address addr_; // @prief 对端的地址
    const int fd_;
    EventHandler handler_;
    Buffer RecvBuffer_;
    BackpressureState RecvbpState_{BackpressureState::kNormal};
    BackpressureAfterSendCallBack BackpressureAfterSend_;
    Buffer SendBuffer_;
    BackpressureState SendbpState_{BackpressureState::kNormal};
    BackpressureAfterReadCallBack BackpressureAfterRead_;

    ServicesMessageCallBack SmessageCallBack_;
    ServicesWriteCompleteCallBack SwriteCompleteCallBack_;
    ServicesCloseCallBack ScloseCallBack_; // @brief 这是对端选择断开连接时的回调
    ServicesErrorCallBack SerrorCallBack_;
    ServicesExceptCallBack SexceptCallBack_;
    ServicesData data_;

    std::atomic<ConnectStatus> Connstatus_;

    CodecPtr codec_;
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
void handleAfterSend(TcpConnection::TcpConnectionPtr conn);

// 特化声明
template<>
void handleAfterSend<BufferBackpressureStrategy::kDiscard>(TcpConnection::TcpConnectionPtr conn);

template<>
void handleAfterSend<BufferBackpressureStrategy::kCloseConnection>(TcpConnection::TcpConnectionPtr conn);

template<>
void handleAfterSend<BufferBackpressureStrategy::kPass>(TcpConnection::TcpConnectionPtr conn);

// 主模板声明
template<BufferBackpressureStrategy strategy>
void handleAfterRecv(TcpConnection::TcpConnectionPtr conn);

// 特化声明
template<>
void handleAfterRecv<BufferBackpressureStrategy::kDiscard>(TcpConnection::TcpConnectionPtr conn);

template<>
void handleAfterRecv<BufferBackpressureStrategy::kCloseConnection>(TcpConnection::TcpConnectionPtr conn);

template<>
void handleAfterRecv<BufferBackpressureStrategy::kPass>(TcpConnection::TcpConnectionPtr conn);

template<>
void handleAfterRecv<BufferBackpressureStrategy::kBackoff>(TcpConnection::TcpConnectionPtr conn);
}    
}


#endif