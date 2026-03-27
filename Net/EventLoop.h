#ifndef _YY_NET_EVENTLOOP_H_
#define _YY_NET_EVENTLOOP_H_
#include <vector>
#include <functional>
#include <queue>
#include <mutex>



#include "EventHandler.h"
#include "sockets.h"
#include "PollerType.h"
#include "../Common/noncopyable.h"
#include "Timer.h"
#include "../Common/locker.h"
#include "../Common/RingBuffer.h"
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
    typedef RingBuffer<Functor> FunctionList;

    EventLoop();
    ~EventLoop()
    {
        assert(isInLoopThread());
    }
    void loop();
    void quit();
    bool isQuit();


    bool isInLoopThread();

    template<typename Callable>
    void submit(Callable&& cb);
    template<typename Callable>
    void DelayedExecution(Callable&& cb);
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
            submit([this, handler]()
            {
                poller_.add_listen(handler);
            });

    }
    void update_listen(EventHandler* handler)
    { 
        assert(handler);    
        if(isInLoopThread())
            poller_.update_listen(handler);
        else
            submit([this, handler]()
            {
                poller_.update_listen(handler);
            });
    }
    void remove_listen(EventHandler* handler)
    {
        assert(handler);
        if(isInLoopThread())
            poller_.remove_listen(handler);
        else
            submit([this, handler]()
            {
                poller_.remove_listen(handler);
            });
    }        
    void doPendingFunctions();
    
    bool CheckeEventLoopStatus();
    
    PollerType poller_;
    PollerHandlerList activeHandlers_;
    FunctionList FunctionList_;
    Pid_t threadId_;
    int status_;
    EventHandler wakeHandler_;

    //std::mutex mtx_;

    //int LoopNum={0};
    //std::string name_;
    
};

template<typename Callable>
void EventLoop::submit(Callable&& cb)
{
    if(isInLoopThread())
    {
        cb();
    }
    else 
    {
        assert(!isInLoopThread());
        FunctionList_.blockappend(std::forward<Callable>(cb));
    }
}
template<typename Callable>
void EventLoop::DelayedExecution(Callable&& cb)
{
    assert(isInLoopThread());
    if(!FunctionList_.append(cb))
    {
        runTimer<LowPrecision>([Fun=std::forward<Callable>(cb),this]()mutable
        {
            DelayedExecution(std::forward<Callable>(Fun));
        },5s,1);
    }
}
}    
}
#endif // _YY_NET_EVENTLOOP_H_