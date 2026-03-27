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
/**
 * @brief TCP连接类，负责管理TCP连接的生命周期和数据传输
 * 
 * TcpConnection封装了TCP连接的管理，包括连接的建立、数据的发送和接收、连接的关闭等。
 * 它使用EventHandler来处理文件描述符上的事件，并使用TcpBuffer来管理数据的读写。
 */
class TcpConnection:noncopyable,
                    public std::enable_shared_from_this<TcpConnection>
{
public:
    /**
     * @brief 缓冲区类型
     */
    typedef TcpBuffer Buffer;
    
    /**
     * @brief 字符容器类型
     */
    typedef Buffer::CharContainer CharContainer;
    /**
     * @brief 连接回调函数类型
     */
    typedef std::function<void(TcpConnectionPtr)> ServicesConnectionCallBack;
    /**
     * @brief 关闭回调函数类型
     */
    typedef std::function<void(TcpConnectionPtr)> CloseCallBack;
    /**
     * @brief 消息回调函数类型
     */
    typedef std::function<void(TcpConnectionPtr)> ServicesMessageCallBack;
    /**
     * @brief 异常回调函数类型
     */
    typedef std::function<void(TcpConnectionPtr,char oob_buf[1])> ServicesExceptCallBack;
    /**
     * @brief 写完成回调函数类型
     */
    typedef std::function<void(TcpConnectionPtr)> ServicesWriteCompleteCallBack;
    /**
     * @brief 错误回调函数类型
     */
    typedef std::function<void(TcpConnectionPtr)> ServicesErrorCallBack;
    // typedef std::function<void(TcpConnectionPtr)> BackpressureAfterSendCallBack;
    // typedef std::function<void(TcpConnectionPtr)> BackpressureAfterReadCallBack; 
    /**
     * @brief 服务数据类型
     */
    typedef AutoContext ServicesData;

    /**
     * @brief 连接状态枚举
     */
    enum class ConnectStatus
    {
        Connecting,    /**< 连接中 */
        Connected,     /**< 已连接 */
        DisConnecting, /**< 断开连接中 */
        DisConnected   /**< 已断开连接 */
    };

    /**
     * @brief 删除默认构造函数
     */
    TcpConnection()=delete;
    
    /**
     * @brief 构造函数
     * 
     * @param fd 文件描述符
     * @param addr 对端地址
     * @param loop 事件循环
     * @param name 连接名称
     */
    TcpConnection(int fd,const Address& addr,EventLoop* loop,const char* name="NoName");
    
    /**
     * @brief 析构函数
     * 
     * 构析函数不能触发回调了，TcpConnectionPtr不允许
     */
    ~TcpConnection();
    
    /**
     * @brief 连接成功
     * 
     * 当连接建立成功时调用。
     */
    void ConnectSuccess();
    
    /**
     * @brief 获取事件处理器
     * 
     * @return EventHandler* 事件处理器
     */
    EventHandler* handler(){return &handler_;}    
    
    /**
     * @brief 获取文件描述符
     * 
     * @return int 文件描述符
     */
    int fd()const noexcept{return handler_.fd();}
    
    /**
     * @brief 获取对端地址
     * 
     * @return const Address& 对端地址
     */
    const Address& addr()const noexcept{return addr_;}
    
    /**
     * @brief 获取事件循环
     * 
     * @return EventLoop* 事件循环
     */
    EventLoop* loop() noexcept{return loop_;}
    
    /**
     * @brief 获取发送缓冲区
     * 
     * @return Buffer& 发送缓冲区
     */
    Buffer& sendBuffer(){return SendBuffer_;}
    
    /**
     * @brief 获取接收缓冲区
     * 
     * @return Buffer& 接收缓冲区
     */
    Buffer& recvBuffer(){return RecvBuffer_;}
    
    /**
     * @brief 检查是否正在连接
     * 
     * @return bool 是否正在连接
     */
    bool isConnecting()const noexcept{return Connstatus_==ConnectStatus::Connecting;}
    
    /**
     * @brief 检查是否已连接
     * 
     * @return bool 是否已连接
     */
    bool isConnected() noexcept{return Connstatus_==ConnectStatus::Connected;}

    /**
     * @brief 扩展功能
     * 
     * @tparam Tag 标签类型
     * @tparam Args 参数类型
     * @param args 参数
     * @return typename Tag::ReturnType 返回值
     */
    template<typename Tag,typename... Args>
    typename Tag::ReturnType Extend(Tag(Args...));

    /**
     * @brief 设置监听读事件
     */
    void setReading()
    {
        handler_.setReading();
    }
    
    /**
     * @brief 设置监听异常事件
     */
    void setExcept()
    {
        handler_.setExcept();
    }
    
    /**
     * @brief 设置事件
     * 
     * @param e 事件
     */
    void setEvent(Event e);
    
    /**
     * @brief 设置连接回调函数
     * 
     * @tparam Callable 可调用对象类型
     * @param cb 连接回调函数
     */
    template<typename Callable>
    void setConnectCallBack(Callable&& cb) {
        SconnectCallback_ = std::forward<Callable>(cb);
    }

    /**
     * @brief 设置消息回调函数
     * 
     * @tparam Callable 可调用对象类型
     * @param cb 消息回调函数
     */
    template<typename Callable>
    void setMessageCallBack(Callable&& cb) {
        SmessageCallBack_ = std::forward<Callable>(cb);
    }

    /**
     * @brief 设置写完成回调函数
     * 
     * @tparam Callable 可调用对象类型
     * @param cb 写完成回调函数
     */
    template<typename Callable>
    void setWriteCompleteCallBack(Callable&& cb) {
        SwriteCompleteCallBack_ = std::forward<Callable>(cb);
    }

    /**
     * @brief 设置关闭回调函数
     * 
     * @tparam Callable 可调用对象类型
     * @param cb 关闭回调函数
     */
    template<typename Callable>
    void setCloseCallBack(Callable&& cb) {
        ScloseCallBack_ = std::forward<Callable>(cb);
    }

    /**
     * @brief 设置错误回调函数
     * 
     * @tparam Callable 可调用对象类型
     * @param cb 错误回调函数
     */
    template<typename Callable>
    void setErrorCallBack(Callable&& cb) {
        SerrorCallBack_ = std::forward<Callable>(cb);
    }

    /**
     * @brief 设置异常回调函数
     * 
     * @tparam Callable 可调用对象类型
     * @param cb 异常回调函数
     */
    template<typename Callable>
    void setExceptCallBack(Callable&& cb) {
        SexceptCallBack_ = std::forward<Callable>(cb);
    }

    // void setBackpressureCallback(BackpressureAfterSendCallBack cb1,BackpressureAfterReadCallBack cb2)
    // {
    //     BackpressureAfterSend_=std::move(cb1);
    //     BackpressureAfterRead_=std::move(cb2);
    // }

    /**
     * @brief 设置TCP保活
     * 
     * @param on 是否启用
     * @param idleSeconds 空闲时间
     * @param intervalSeconds 间隔时间
     * @param maxProbes 最大探测次数
     */
    void setTcpAlive(bool on,int idleSeconds=7200, 
                  int intervalSeconds=75,int maxProbes=9)
    {
        sockets::setKeepAlive(handler_.fd(),on,idleSeconds,intervalSeconds,maxProbes);
    }
    
    /**
     * @brief 设置TCP_NODELAY
     * 
     * @param on 是否启用
     */
    void setTcpNoDelay(bool on)
    {
        sockets::setTcpNoDelay(handler_.fd(),on);   
    }
    
    // @brief 这些是有多线程安全问题的
    /**
     * @brief 断开连接
     * 
     * 这是我端主动关闭连接时的回调,关闭我方的写端
     */
    void disconnect();    
    
    /**
     * @brief 发送数据
     * 
     * @param message 数据指针
     * @param len 数据大小
     */
    void send(const char* message,size_t len);
    
    /**
     * @brief 发送数据
     * 
     * @param message 数据字符串
     */
    void send(std::string&& message);
    
    /**
     * @brief 发送输出
     * 
     * 配合ProtoMsgCodec使用的接口，把缓冲区的数据发送出去
     */
    void sendOutput(); 
    
    /**
     * @brief 获取上下文
     * 
     * @tparam T 上下文类型
     * @return T& 上下文引用
     */
    template<typename T>
    T& context(){ return data_.context<T>();}
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