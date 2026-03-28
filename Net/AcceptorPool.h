
#include "EventLoopThread.h"
#include "Acceptor.h"
namespace yy 
{
namespace net 
{
class AcceptorPool:noncopyable
{
public:
    typedef Acceptor::ServicesConnectCallBack ServicesConnectCallBack ;
     explicit AcceptorPool(const Address& addr,TcpServer* Ser,int num):
     addr_(addr),
     Ser_(Ser)
     {
        threads_.reserve(num);
        for(int i=0;i<num;++i)
        {
            threads_.emplace_back(std::make_unique<EventLoopThread>(i+1+1000));
        }
        acceptors_.reserve(num);
        // for(int i=0;i<num;++i)
        // {
        //     acceptors_.emplace_back(std::make_unique<Acceptor>());
        // }         
     }
    ~AcceptorPool()
    {
   
        for(auto it=threads_.begin();it!=threads_.end();++it)
        {
            auto& t=(*it);
            if(t->joinable())
            {
                t->stop();
            }
        }         
    }
    void setNewConnectCallBack(ServicesConnectCallBack cb){SconnectCallBack_=std::move(cb);}
    void run()
    {
        for(size_t i=0;i<threads_.size();++i)
        {
            auto loop=threads_[i]->run();
            acceptors_.emplace_back(std::make_unique<Acceptor>(addr_,loop,Ser_,i));
            acceptors_[i]->setNewConnectCallBack(SconnectCallBack_);
            acceptors_[i]->listen();
        }
    }
    void stop()
    {

        for(auto it=threads_.begin();it!=threads_.end();++it)
        {
            (*it)->stop();
        } 
    }  
private:
    const Address addr_;//isOne
    TcpServer* Ser_;
    std::vector<std::unique_ptr<Acceptor>> acceptors_; 
    std::vector<std::unique_ptr<EventLoopThread>> threads_;

    ServicesConnectCallBack SconnectCallBack_;
}; 
}
}