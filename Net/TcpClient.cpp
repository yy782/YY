#include <stdio.h>  // snprintf
#include "TcpClient.h"
#include "sockets.h"
#include "TimerQueue.h"
#include <memory>

namespace yy 
{
namespace net 
{
struct TcpClient::Connector:noncopyable,
                            std::enable_shared_from_this<Connector>
{   
    typedef std::function<void(int)> NewConCallBack;
    typedef std::function<void()> FailConCallBack;

    enum State 
    { 
        kDisconnected, 
        kConnecting, 
        kConnected 
    };
    static const HTimeInterval kInitRetryDelayMs;
    static const HTimeInterval kMaxRetryDelayMs;

    Connector(EventLoop* loop,const Address& serverAddr,bool* retry ,const NewConCallBack& Ncb,const FailConCallBack& Fcb): 
    loop_(loop),
    serverAddr_(serverAddr),
    state_(kDisconnected),
    handler_(nullptr),
    retryDelayMs_(kInitRetryDelayMs),
    retry_(retry),
    Ncb_(std::move(Ncb)),
    Fcb_(std::move(Fcb)) 
    {
    }

    ~Connector() 
    {   
        if(handler_!=nullptr)
        {
            delete handler_;
        }
        if(state_==kConnected)return;
        if(fd_!=-1)
            sockets::close(fd_);
        
    }

    void start() 
    {

        loop_->submit([c=weak_from_this()]()
        {
            auto con=c.lock();
            if(con)
            {
                con->startInLoop();
            }
        },"Connector::start");
    }

    void stop() 
    {

        loop_->submit([c=weak_from_this()]()
        {
            auto con=c.lock();
            if(con)
            {
                con->stopInLoop();
            }
        },"Connector::stop");
    }

    void restart() 
    {
        state_=State::kDisconnected;
        retryDelayMs_ = kInitRetryDelayMs;
        startInLoop();
    }
    bool isConnected()const noexcept{return state_==State::kDisconnected;}
    bool isConnecting()const noexcept{return state_==State::kConnecting;}
private:
    void startInLoop() 
    {
        if(state_ != kDisconnected) return;
        connectInLoop();
    }

    void connectInLoop() 
    {
        assert(loop_->isInLoopThread());
        assert(state_ == kDisconnected);
        fd_ = sockets::createTcpSocketOrDie(serverAddr_.family());/////////////////////////////////////////////////////////
        int ret = sockets::connect(fd_, serverAddr_);
        int savedErrno = (ret == 0) ? 0 : errno;
        switch (savedErrno) {
            case 0:
            case EINPROGRESS:
            case EINTR:
            case EISCONN:
                connecting();
                break;
            case EAGAIN:
            case EADDRINUSE:
            case EADDRNOTAVAIL:
            case ECONNREFUSED:
            case ENETUNREACH:
                retry();
                break;
            default:
                LOG_ERRNO(savedErrno);
                sockets::close(fd_);
                fd_=-1;
                Fcb_();
                break;
        }
    }

    void connecting() 
    {
        state_=State::kConnecting;
        handler_=new EventHandler(fd_,loop_,"Connector::connecting");
        handler_->setWriteCallBack([this]()
        {
            handleWrite();
        });
        handler_->setErrorCallBack([this]()
        {
            handleError();
        });
        handler_->setWriting();
    }

    void handleWrite() 
    {
        if(state_==kConnecting) 
        {
           
            int err = sockets::sockfd_has_error(fd_);
            if (err!=0) 
            {
                LOG_WARN("Connector::handleWrite - SO_ERROR = "<<err);
                handler_->removeListen("Conector::handleWrite,err");
                resetHandler();
                retry();
                return;
            }
            if (sockets::isSelfConnect(fd_)) 
            {
                LOG_WARN("Connector::handleWrite - Self connect");
                handler_->removeListen("Conector::handleWrite,isSelfCon");
                resetHandler();
                retry();
                return;
            }
            state_=kConnected;
            handler_->removeListen("Conector::handleWrite,NewCon");
            Ncb_(fd_);
        }
    }

    void handleError() {
        assert(loop_->isInLoopThread());

        if (state_ == kConnecting) 
        {

            int err = sockets::sockfd_has_error(fd_);
            LOG_WARN("Connector::handleError - Self connect"<<err);
            handler_->removeListen("Connector::handleError");
            resetHandler();
            retry();
        }
    }
    void retry() 
    {

        if(fd_ >0) 
        {
            sockets::close(fd_);
            fd_=-1;
        }
        state_=kDisconnected;

        if (*retry_ ) 
        {
            EXCLUDE_BEFORE_COMPILATION( 
                LOG_CLIENT_INFO("Retry connecting to "<<serverAddr_.sockaddrToString()<<" in "<<
                retryDelayMs_.getTimes()<<" ms");
            )
            loop_->runTimer<HighPrecision>([c=weak_from_this()]()
            {
                auto p=c.lock();
                if(p)
                {
                    p->startInLoop();                    
                }
            },retryDelayMs_,1);

            // 指数退避
            HTimeInterval NextretryDelayMs=2*retryDelayMs_;
            retryDelayMs_=NextretryDelayMs<kMaxRetryDelayMs?NextretryDelayMs:kMaxRetryDelayMs;
        } 
        else 
        {
            Fcb_();
        }            
        

    }
    void stopInLoop() 
    {
        assert(loop_->isInLoopThread());
        if (state_ == kConnecting) 
        {
            state_=kDisconnected;
            handler_->removeListen("Connector::stopInLoop");
        }
    }
    void resetHandler()
    {
        EventHandler* p=handler_;
        loop_->DelayedExecution<true>([p,c=weak_from_this()]()
        {
            delete p;
            auto con=c.lock();
            if(con)
            {
                con->handler_=nullptr;
            }
        },"Connector::resetHandler");
    }

    // void resetHandler() 
    // {
    //     if(handler_) 
    //     {
    //         handler_->removeListen();
    //     }
    // }

  
    int fd_={-1};// InOne
    EventLoop* loop_;
    const Address& serverAddr_;
    
    State state_;
    EventHandler* handler_;// handlerError和handlerWrite中要延迟释放handler资源，但是要捕捉对象，不一定能保证资源释放，用原始指针
    HTimeInterval retryDelayMs_;
    bool* retry_ ;  

    NewConCallBack Ncb_;
    FailConCallBack Fcb_;
};

const HTimeInterval TcpClient::Connector::kInitRetryDelayMs=500ms;
const HTimeInterval TcpClient::Connector::kMaxRetryDelayMs=30*1000ms;

TcpClient::TcpClient(const Address& serverAddr,EventLoop* loop): 
    loop_(loop),
    serverAddr_(serverAddr),
    retry_(false),
    connector_(std::make_shared<Connector>(loop, serverAddr, &retry_, [this](int fd)
    {
        newConnection(fd);
    }, [this]()
    {
        removeConnection();
    })
    )
{
    //connector_=std::make_shared<Connector>(loop, serverAddr,weak_from_this());
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

bool TcpClient::isConnected() const noexcept
{
    return connection_?true:false;
}

bool TcpClient::isConnecting() const noexcept
{
    assert(connector_);
    return connector_->isConnecting();
}

// void TcpClient::newConnection(int sockfd) 
// {
    
//     assert(loop_->isInLoopThread());
//     connection_=std::make_shared<TcpConnection>(sockfd,serverAddr_,loop_,"TcpCli");



//     connection_->setConnectCallBack(SconnectionCallback_);
//     connection_->setMessageCallBack(SmessageCallback_);
//     connection_->setErrorCallBack(SerrorCallback_);
//     connection_->setCloseCallBack(std::bind(&TcpClient::removeConnection, 
//                                             this));

//     connection_->ConnectSuccess();
// }
void TcpClient::newConnection(int sockfd) 
{
    
    assert(loop_->isInLoopThread());
    assert(SconnectionCallback_);
    connection_=SconnectionCallback_(sockfd,addr_,loop_);


    connection_->setDestructorCallBack([this](TcpConnectionPtr)
    {
        removeConnection();
    });
}
void TcpClient::removeConnection() {
    assert(loop_->isInLoopThread());
   

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
        SconnectFailCallback_();            
    }
}



}
}
