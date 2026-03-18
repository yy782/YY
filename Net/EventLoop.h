#ifndef _YY_NET_EVENTLOOP_H_
#define _YY_NET_EVENTLOOP_H_
#include <vector>
#include "EventHandler.h"
#include "sockets.h"
#include "Poller.h"
#include "PollerSpecificType.h"
#include "../Common/noncopyable.h"
#include "../Common/TimeStamp.h"
#include "../Common/locker.h"

#include <queue>
#include "../Common/RingBuffer.h"
namespace yy
{
namespace net
{



class EventLoop:public noncopyable
{
public:
    typedef Thread::Pid_t Pid_t;
    typedef std::function<void()> Functor;
    typedef RingBuffer<Functor> FunctionList;
    EventLoop();
    ~EventLoop()=default;
    void loop();
    void quit();
    bool isQuit();
    

    
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

    void setPid_t(const Pid_t& pid)
    {
        threadId_=pid;
    }
    bool isInLoopThread();
    void submit(Functor cb);
private:
  
    
    void doPendingFunctions();
    
    bool CheckeEventLoopStatus();
    
    PollerType poller_;
    PollerHandlerList activeHandlers_;
    FunctionList FunctionList_;
    
    Pid_t threadId_;

    int status_;
    EventHandler QuitHandler_;
    
};
}    
}
#endif // _YY_NET_EVENTLOOP_H_