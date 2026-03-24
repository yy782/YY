#ifndef _YY_NET_EVENTLOOP_H_
#define _YY_NET_EVENTLOOP_H_
#include <vector>
#include "EventHandler.h"
#include "sockets.h"
#include "Poller.h"
#include "PollerSpecificType.h"
#include "../Common/noncopyable.h"
#include "Timer.h"
#include "../Common/locker.h"

#include <queue>
#include "../Common/RingBuffer.h"
#include <mutex>
namespace yy
{
namespace net
{

// struct Fun
// {
//     std::function<void()> Functor_;
//     std::string name_;
//     Fun()=default;
//     template<typename Callable>
//     Fun(Callable&& callable, std::string name = "NoName")
//         : Functor_(std::forward<Callable>(callable))
//         , name_(std::move(name))
//     {
//     }  
//     std::string getName(){return name_;} 
//     void operator()()
//     {
//         Functor_();
//     }
// };

class EventLoop:public noncopyable
{
public:
    
    typedef Thread::Pid_t Pid_t;
    typedef std::function<void()> Functor;
    //typedef Fun Functor;

    //typedef RingBuffer<Functor> FunctionList;
    typedef std::vector<Functor> FunctionList;
    EventLoop();
    ~EventLoop()=default;
    void loop();
    void quit();
    bool isQuit();

    void setName(const char* name)
    {
        name_=name;
    }
    // std::atomic<int> num={0};
    // std::atomic<int> num2={0};

    bool isInLoopThread();
    void submit(Functor cb);
    template<class PrecisionTag>
    void runTimer(BaseTimer::TimerCallBack cb,typename Timer<PrecisionTag>::Time_Interval interval,int execute_count);
private:
    friend class EventHandler;
    void wakeup();
    void addListen(EventHandler* handler)
    {
        assert(handler);
        if(isInLoopThread())
            poller_.add_listen(handler);
        else
            submit(std::bind(&PollerType::add_listen,&poller_,handler));

    }
    void update_listen(EventHandler* handler)
    { 
        assert(handler);    
        if(isInLoopThread())
            poller_.update_listen(handler);
        else
            submit(std::bind(&PollerType::update_listen,&poller_,handler));
    }
    void remove_listen(EventHandler* handler)
    {
        assert(handler);
        if(isInLoopThread())
            poller_.remove_listen(handler);
        else
            submit(std::bind(&PollerType::remove_listen,&poller_,handler));
    }        
    void doPendingFunctions();
    
    bool CheckeEventLoopStatus();
    
    PollerType poller_;
    PollerHandlerList activeHandlers_;
    FunctionList FunctionList_;
    Pid_t threadId_;
    int status_;
    EventHandler wakeHandler_;

    std::mutex mtx_;

    int LoopNum={0};
    std::string name_;
    
};



}    
}
#endif // _YY_NET_EVENTLOOP_H_