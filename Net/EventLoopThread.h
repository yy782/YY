#ifndef _YY_NET_EVENTLOOPTHREAD_H_
#define _YY_NET_EVENTLOOPTHREAD_H_

#include "../Common/locker.h"
#include "../Common/noncopyable.h"
#include "EventLoop.h"
namespace yy
{
namespace net
{
//thread_local EventLoop safe_loop;
class EventLoopThread:public  noncopyable
{
public:
    typedef EventLoop::Functor Functor;
    EventLoopThread():
    loop_()
    {}
    void run()
    {
        //assert(loop_);
        thread_.run([this]()mutable
        {
            loop_.setPid_t(thread_.getId());
            loop_.loop();
        });
    }
    void stop()
    {
        //assert(loop_);
        loop_.quit();
        thread_.join();
    }
    EventLoop* getEventLoop(){return &loop_;}
private:
    Thread thread_;
    EventLoop loop_;
};
}    
}



#endif