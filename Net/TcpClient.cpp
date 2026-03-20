#include <stdio.h>  // snprintf
#include "TcpClient.h"
#include "sockets.h"
namespace yy 
{
namespace net 
{
struct TcpClient::Connector:noncopyable
{   
    enum State 
    { 
        kDisconnected, 
        kConnecting, 
        kConnected 
    };
    static const LTimeInterval kInitRetryDelayMs;
    static const LTimeInterval kMaxRetryDelayMs;

    Connector(EventLoop* loop, const Address& serverAddr, TcpClient* client): 
    loop_(loop),
    serverAddr_(serverAddr),
    client_(client),
    state_(kDisconnected),
    retryDelayMs_(kInitRetryDelayMs),
    connect_(false) 
    {
    }

    ~Connector() 
    {
        stop();
    }

    void start() 
    {
        connect_=true;
        loop_->submit(std::bind(&Connector::startInLoop,this));
    }

    void stop() 
    {
        connect_ = false;
        loop_->submit(std::bind(&Connector::stopInLoop,this));
    }

    void restart() 
    {
        state_=State::kDisconnected;
        retryDelayMs_ = kInitRetryDelayMs;
        startInLoop();
    }

private:
    void startInLoop() 
    {
        assert(loop_->isInLoopThread());
        if (!connect_ || state_ != kDisconnected) return;
        connectInLoop();
    }

    void connectInLoop() 
    {
        assert(loop_->isInLoopThread());
        assert(state_ == kDisconnected);
        int fd = sockets::createTcpSocketOrDie(serverAddr_.get_family());
        int ret = sockets::connect(fd, serverAddr_);
        int savedErrno = (ret == 0) ? 0 : errno;
        switch (savedErrno) {
            case 0:
            case EINPROGRESS:
            case EINTR:
            case EISCONN:
                connecting(fd);
                break;
            case EAGAIN:
            case EADDRINUSE:
            case EADDRNOTAVAIL:
            case ECONNREFUSED:
            case ENETUNREACH:
                retry(fd);
                break;
            default:
                LOG_ERRNO(savedErrno);
                sockets::close(fd);
                client_->connectFail();
                break;
        }
    }

    void connecting(int fd) 
    {
        state_=State::kConnecting;

        handler_=std::make_unique<EventHandler>(fd,loop_);
        handler_->setWriteCallBack(std::bind(&Connector::handleWrite,this));
        handler_->setErrorCallBack(std::bind(&Connector::handleError,this));
        handler_->set_name("ConnectorHandler");
        handler_->setWriting();
    }

    void handleWrite() 
    {
        assert(loop_->isInLoopThread());
        if(state_==kConnecting) 
        {
            int sockfd=handler_->get_fd();
            int err = sockets::sockfd_has_error(sockfd);
            if (err!=0) 
            {
                LOG_WARN("Connector::handleWrite - SO_ERROR = "<<err);
                retry();
                return;
            }
            if (sockets::isSelfConnect(sockfd)) 
            {
                LOG_WARN("Connector::handleWrite - Self connect");
                retry();
                return;
            }
            state_=kConnected;
            client_->newConnection(::dup(sockfd));
            resetHandler();
        }
    }

    void handleError() {
        assert(loop_->isInLoopThread());

        if (state_ == kConnecting) 
        {
            int socketfd=handler_->get_fd();
            int err = sockets::sockfd_has_error(socketfd);
            LOG_WARN("Connector::handleError - Self connect"<<err);
            retry();
        }
    }
    void retry(int sockfd) // handler未绑定fd
    {
        assert(handler_->get_fd()==-1);
        if(sockfd >= 0) 
        {
            sockets::close(sockfd);
        }
        state_=kDisconnected;
        if (connect_&&client_->retry()) 
        {
            EXCLUDE_BEFORE_COMPILATION( 
                LOG_CLIENT_INFO("Retry connecting to "<<serverAddr_.toIpPort()<<" in "<<
                retryDelayMs_<<" ms");
            )
            loop_->runTimer<LowPrecision>([this](){
                startInLoop();
            },retryDelayMs_,1);

            // 指数退避
            LTimeInterval NextretryDelayMs=2ms*retryDelayMs_;
            retryDelayMs_=NextretryDelayMs<kMaxRetryDelayMs?NextretryDelayMs:kMaxRetryDelayMs;
        } 
        else 
        {
            client_->connectFail();
        }
    }
    void retry() // handler已经绑定fd
    {
        assert(handler_->get_fd()>0);
        handler_->removeListen();
        handler_.reset();
        state_=kDisconnected;
        if (connect_&&client_->retry()) 
        {
            EXCLUDE_BEFORE_COMPILATION( 
                LOG_CLIENT_INFO("Retry connecting to "<<serverAddr_.toIpPort()<<" in "<<
                retryDelayMs_<<" ms");
            )
            loop_->runTimer<LowPrecision>([this](){
                startInLoop();
            },retryDelayMs_,1);

            // 指数退避
            LTimeInterval NextretryDelayMs=2ms*retryDelayMs_;
            retryDelayMs_=NextretryDelayMs<kMaxRetryDelayMs?NextretryDelayMs:kMaxRetryDelayMs;
        } 
        else 
        {
            client_->connectFail();
        }
    }
    void stopInLoop() 
    {
        assert(loop_->isInLoopThread());
        if (state_ == kConnecting) 
        {
            state_=kDisconnected;
            resetHandler();
        }
    }

    void resetHandler() 
    {
        if(handler_) 
        {
            handler_->removeListen();
            handler_.reset();
        }
    }

  

    EventLoop* loop_;
    Address serverAddr_;
    TcpClient* client_;  
    State state_;
    std::unique_ptr<EventHandler> handler_;
    LTimeInterval retryDelayMs_;
    bool connect_;  
};

const LTimeInterval TcpClient::Connector::kInitRetryDelayMs=500ms;
const LTimeInterval TcpClient::Connector::kMaxRetryDelayMs=30*1000ms;

TcpClient::TcpClient(EventLoop* loop, const Address& serverAddr): 
    loop_(loop),
    serverAddr_(serverAddr),
    retry_(false),
    connector_(std::make_unique<Connector>(loop, serverAddr, this)) 
{
}

TcpClient::~TcpClient() 
{
}

void TcpClient::connect() 
{
    connector_->start();
}

void TcpClient::disconnect() 
{
    if(connection_)
    {
        connection_->disconnect();
    }
}

void TcpClient::stop() 
{
    retry_ = false;
    connector_->stop();
    if (connection_) {
        connection_->disconnect();
    }
}

bool TcpClient::isConnected() const 
{
    return connection_ && connection_->isConnected();
}

bool TcpClient::isConnecting() const 
{
    return connection_ && connection_->isConnecting();
}

void TcpClient::send(std::string&& message) 
{
    connection_->send(std::move(message));
}

void TcpClient::send(const char* data, size_t len) 
{
    if (connection_ && connection_->isConnected()) 
    {
        connection_->send(data, len);
    } else 
    {
       connection_->getSendBuffer().append(data,len);         // 数据挤压问题 
    }
}

void TcpClient::sendOutput()
{
    if(connection_ && connection_->isConnected()) 
    {
        connection_->sendOutput();
    }
}

void TcpClient::newConnection(int sockfd) {
    assert(loop_->isInLoopThread());
    connection_=std::make_shared<TcpConnection>(sockfd,serverAddr_,loop_);
    connection_->setConnectCallBack(SconnectionCallback_);
    connection_->setMessageCallBack(SmessageCallback_);
    connection_->setErrorCallBack(SerrorCallback_);
    connection_->setCloseCallBack(std::bind(&TcpClient::removeConnection, 
                                            this));
    connection_->setReading();
    connection_->setExcept();
    connection_->ConnectSuccess();
}

void TcpClient::removeConnection() {
    assert(loop_->isInLoopThread());
   

    // 先调用用户回调
    if (ScloseCallback_) {
        ScloseCallback_(connection_);      
    }

    // 清理连接
    connection_.reset();

    // 如果需要自动重连
    if (retry_) {
        EXCLUDE_BEFORE_COMPILATION(
            LOG_CLIENT_INFO("Connection lost, will reconnect");
        )
        connector_->restart();
    }
}

void TcpClient::connectFail() {
    assert(loop_->isInLoopThread());

    if (SconnectFailCallback_) {
        SconnectFailCallback_(this);            
    }
}



}
}
