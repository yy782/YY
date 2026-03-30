#include "TcpConnection.h"
#include <algorithm>
#include "../Common/locker.h"
using namespace yy::net;
namespace yy /////////////////////////////////
{
template<typename T>
class ObjectPool;
template<>
class ObjectPool<TcpConnectionPtr>;
typedef class ObjectPool<TcpConnectionPtr> TcpConPool;

const int ExpendNum=1000;
template<>
class ObjectPool<TcpConnectionPtr>: noncopyable 
{
public:
    typedef Thread::Pid_t Pid_t;
    struct Config:copyable
    {
        typedef TcpConnection::ServicesMessageCallBack ServicesMessageCallBack;
        typedef TcpConnection::ServicesExceptCallBack ServicesExceptCallBack;
        typedef TcpConnection::ServicesCloseCallBack ServicesCloseCallBack;
        typedef TcpConnection::ServicesErrorCallBack ServicesErrorCallBack;
        typedef TcpConnection::ServicesData ServicesData;        
        Event event_;
        bool isTcpAlive_;
        ServicesMessageCallBack SmessageCallBack_;
        ServicesCloseCallBack ScloseCallBack_;
        ServicesErrorCallBack SerrorCallBack_;
        ServicesExceptCallBack SexceptCallBack_;  
        ServicesData data_;
    };
    explicit ObjectPool(int num,struct Config& config):
    config_(config)
    {
        expend(num);
    }
    void expend(int num)
    {
        for(int i=0;i<num;++i)
        {
            auto conn=std::make_shared<TcpConnection>(config_.event_);
            if(config_.isTcpAlive_)conn->setTcpAlive(true);
            conn->setMessageCallBack(config_.SmessageCallBack_);
            conn->setCloseCallBack(config_.ScloseCallBack_);
            conn->setExceptCallBack(config_.SexceptCallBack_);
            conn->setErrorCallBack(config_.SerrorCallBack_);
            conn->date()=config_.data_;
            free_list_.push_back(conn);
        }
    }
    TcpConnectionPtr acquire(int fd,const Address& addr,EventLoop* loop)
    {

        if(id_==-1)
        {
            id_=loop->id();
        }
        else 
        {
            assert(id_==loop->id());
        }


        if(free_list_.empty())
        {
            expend(ExpendNum);
        }
        auto conn=free_list_.front();
        free_list_.pop_front();
        //active_list_.push_back(conn);
        conn->init(fd,addr,loop); 
        return conn;
    }
    void release(TcpConnectionPtr conn)
    {

        assert(conn);
        assert(conn->loop()->id()==id_);
        conn->reset();
        //assert(std::find(active_list_.begin(),active_list_.end(),conn));
        assert(std::find(free_list_.begin(),free_list_.end(),conn)==free_list_.end());
        free_list_.push_back(conn);
    }
private:
    Config config_;// 需外部强行保证
    //std::list<TcpConnectionPtr> active_list_;
    std::list<TcpConnectionPtr> free_list_;
    int id_={-1};
};
}