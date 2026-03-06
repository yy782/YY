#ifndef _YY_NET_TCPCONNECTION_H_
#define _YY_NET_TCPCONNECTION_H_
#include "../Common/noncopyable.h"
#include "InetAddress.h"
#include "EventHandler.h"
#include "../Common/Buffer.h"
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

    Buffer& getWriteBuffer(){return writeBuffer_;}

        // 新增：设置背压策略
    void setBackpressureStrategy(BackpressureStrategy strategy)
    {
       backpressureStrategy_=strategy;
    }

    // 新增：设置水位线
    void setWaterMarks(size_t high, size_t low)
    {
        backpressureConfig_.highWaterMark = high;
        backpressureConfig_.lowWaterMark = low;
    }

    // 新增：检查当前背压状态
    bool isBackpressured() const
    {
        return backpressureState_ == BackpressureState::kHighWater;
    }

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
    /**
     * @brief 背压策略枚举
     * 
     * 定义当发送缓冲区达到高水位时采取的不同处理策略。
     * 每种策略适用于不同的业务场景和可靠性要求。
     */
    enum class BackpressureStrategy
    {
        kDiscard,         /**< 丢弃新数据 - 适用于实时数据（如音视频流、监控数据），允许丢包以保护内存 */
        kCloseConnection, /**< 断开连接 - 极端保护措施，用于检测到恶意客户端或资源严重受限时 */
        kBackoff,         /**< 停止读取 - 反向压力机制，暂停从socket读取新数据，最推荐的优雅策略 */
        kThrottle,        /**< 限速发送 - 令牌桶算法控制发送速率，适用于带宽控制和流量整形 */
        kPass             /**< 不处理 - 透传策略，继续追加数据，需要业务层自行保证不会OOM */
    };

    /**
     * @brief 背压配置结构体
     * 
     * 包含背压机制的所有可调参数，用于精细控制单个连接的流量行为。
     */
    struct BackpressureConfig
    {
        size_t highWaterMark = 64 * 1024;  /**< 高水位阈值（默认64KB）- 当发送缓冲区超过此值时触发背压策略 */
        size_t lowWaterMark = 16 * 1024;    /**< 低水位阈值（默认16KB）- 当发送缓冲区降至此值以下时解除背压 */
        /**
         * @brief 令牌桶限速配置（仅当strategy为kThrottle时生效）
         */
        size_t rateLimit = 0;                /**< 每秒发送字节数限制，0表示不限速 */
        size_t burstSize = 0;                 /**< 突发大小，允许瞬间超过rateLimit的最大字节数 */
    };

    /**
     * @brief 背压状态枚举
     * 
     * 表示当前连接所处的背压阶段，用于内部状态跟踪和监控。
     */
    enum class BackpressureState
    {
        kNormal,      /**< 正常状态 - 发送缓冲区低于高水位，一切正常 */
        kHighWater,   /**< 高水位状态 - 已触发背压，正在执行相应策略 */
    };

    // 新增：背压处理方法
    bool handleBackpressureBeforeSend(size_t len);
    void handleBackpressureAfterWrite();
    void checkWaterLevels();
    void startBackoff();   // 停止读取
    void resumeReading();  // 恢复读取
    void throttleSend();    // 限速发送

    // ... 现有成员 ...

    // 新增成员
    BackpressureStrategy backpressureStrategy_;
    BackpressureConfig backpressureConfig_;
    BackpressureState backpressureState_{BackpressureState::kNormal};
    std::atomic<size_t> bytesSentThisSecond_{0};     // 当前秒已发送字节
    LTimeStamp lastRateCheck_;                          // 限速检查时间点


    Address addr_; // @prief 对端的地址
    EventHandler handler_;
    Buffer readBuffer_;
    Buffer writeBuffer_;

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