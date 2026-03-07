#ifndef _YY_NET_TCPCONNECTION_H_
#define _YY_NET_TCPCONNECTION_H_
#include "../Common/noncopyable.h"
#include "InetAddress.h"
#include "EventHandler.h"
#include "TcpBuffer.h"
#include <memory>
#include <any>
#include "../Common/locker.h"
#include "../Common/TimeStamp.h"
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
    typedef std::function<void(TcpConnectionPtr,Buffer*)> ServicesMessageCallBack;
    typedef std::function<void(TcpConnectionPtr)> ServicesWriteCompleteCallBack;
    typedef std::function<void(TcpConnectionPtr)> ServicesCloseCallBack;
    typedef std::function<void(TcpConnectionPtr)> ServicesErrorCallBack;
    typedef std::vector<std::any> ServicesData;



    TcpConnection(int fd,const Address& addr,EventLoop* loop);
    ~TcpConnection();// 构析函数不能触发回调了，TcpConnectionPtr不允许
    //TcpConnect(TcpConnect&& other);
    void connect()
    {
        sockets::connect(handler_.get_fd(),addr_);
        assert(Connstatus_==ConnectStatus::DisConnected);
        Connstatus_=ConnectStatus::Connected;
    }
    static TcpConnectionPtr accept(int fd,const Address& addr,EventLoop* loop)// @note 服务端用这个接口
    {
        auto conn=std::make_shared<TcpConnection>(fd,addr,loop);
        conn->Connstatus_=ConnectStatus::Connected;
        return conn;
    }

    EventHandler* getHandler(){return &handler_;}    
    int get_fd()const{return handler_.get_fd();}
    Address getAddr()const{return addr_;}
    void setReading()
    {
        handler_.setReading();
        handler_.setExcept();
    }
/**
 * 设置服务消息回调函数
 * @param cb 回调函数对象，通过移动语义传递以提高效率
 */
    void setMessageCallBack(ServicesMessageCallBack cb){SmessageCallBack_=std::move(cb);}
    void setWriteCompleteCallBack(ServicesWriteCompleteCallBack cb){SwriteCompleteCallBack_=std::move(cb);}
    void setCloseCallBack(ServicesCloseCallBack cb){ScloseCallBack_=std::move(cb);} // @brief 这是对端关闭的回调
    void setErrorCallBack(ServicesErrorCallBack cb){SerrorCallBack_=std::move(cb);}

    void setTcpAlive(bool on,int idleSeconds=7200, 
                  int intervalSeconds=75,int maxProbes=9)
    {
        sockets::setKeepAlive(handler_.get_fd(),on,idleSeconds,intervalSeconds,maxProbes);
    }
    void setName(const char* name){handler_.set_name(name);}

    // @brief 这些是有多线程安全问题的
    void disconnect(); // @brief 这是我端主动关闭连接时的回调,关闭我方的写端    
    void send(const char* message,size_t len);
    ServicesData& getData(){return data_;}
    bool isConnected(){return Connstatus_==ConnectStatus::Connected;}

    Buffer& getWriteBuffer(){return SendBuffer_;}



private:
    void sendInLoop(const char* message,size_t len);
    void disconnectInLoop();
    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();

    enum class ConnectStatus
    {
        Connected=1,
        DisConnected
    };

    //缓冲区的背压水位
    struct BackpressureConfig
    {
        static size_t highWaterMark;
        static size_t lowWaterMark;
    };
    enum class BackpressureState
    {
        kNormal,     
        kHighWater,  
    }; 

    enum class BufferBackpressureStrategy
    {
        kDiscard,         /**< 丢弃新数据 - 适用于实时数据（如音视频流、监控数据），允许丢包以保护内存 */ //通用
        kCloseConnection, /**< 断开连接 - 极端保护措施，用于检测到恶意客户端或资源严重受限时 */
        kPass,             /**< 不处理 - 透传策略，继续追加数据，需要业务层自行保证不会OOM */   
        
        kThrottle,        /**< 限速发送 - 令牌桶算法控制发送速率，适用于带宽控制和流量整形 */  // Send Buffer

        kBackoff         /**< 停止读取 - 反向压力机制，暂停从socket读取新数据，最推荐的优雅策略 */ // Recv Buffer
    };    
    //Send Buffer的背压处理
    bool handleBackpressureBeforeSend(size_t len);
    void handleBackpressureAfterWrite();

    //recv Buffer的背压处理
    bool handleBackpressureBeforeRead();
    void handleBackpressureAfterRead(size_t len);



    Address addr_; // @prief 对端的地址
    EventHandler handler_;
    Buffer RecvBuffer_;
    BackpressureState RecvbpState_{BackpressureState::kNormal};
    BufferBackpressureStrategy RecvbpStrategy_;
    Buffer SendBuffer_;
    BackpressureState SendbpState_{BackpressureState::kNormal};
    BufferBackpressureStrategy SendbpStrategy_;

    ServicesMessageCallBack SmessageCallBack_;
    ServicesWriteCompleteCallBack SwriteCompleteCallBack_;
    ServicesCloseCallBack ScloseCallBack_; // @brief 这是对端选择断开连接时的回调
    ServicesErrorCallBack SerrorCallBack_;
    ServicesData data_;

    std::atomic<ConnectStatus> Connstatus_;
};
}    
}


#endif