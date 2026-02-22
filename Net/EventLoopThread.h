#ifndef _YY_NET_EVENTLOOPTHREAD_H_
#define _YY_NET_EVENTLOOPTHREAD_H_

#include "../Common/locker.h"
#include "../Common/noncopyable.h"
#include "EventLoop.h"
namespace yy
{
namespace net
{
thread_local EventLoop* safe_loop;
class EventLoopThread:public  noncopyable
{
public:
    EventLoopThread():
    loop_(safe_loop)
    {}
    void run()
    {
        assert(loop_);
        thread_.run(std::bind(&EventLoop::loop,loop_));
    }
    void stop()
    {
        assert(loop_);
        loop_->quit();
    }
    EventLoop* getEventLoop()const{return loop_;}
private:
    Thread thread_;
    EventLoop* loop_;
};
}    
}



#endif