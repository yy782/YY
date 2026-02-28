#ifndef _YY_NET_TIMETWHEEL_H_
#define _YY_NET_TIMETWHEEL_H_

#include "../Common/noncopyable.h"
#include "../Common/TimeStamp.h"
#include <memory>
#include <assert.h>
#include "Timer.h"
namespace yy
{
namespace net
{

class EventHandler;
class EventLoop;

class TimerWheel:public noncopyable
{
public:
    
    typedef Timer<LowPrecision> LTimer;
    typedef std::shared_ptr<LTimer> LTimerPtr;

    struct Node;
    typedef std::shared_ptr<Node> NodePtr;
    typedef std::weak_ptr<Node> WeakNodePtr;  
    TimerWheel()=delete;
    TimerWheel(EventLoop* loop);  
    ~TimerWheel();
    void add_timer(TimerCallBack cb,int interval,int execute_count)
    {
        auto timer=std::make_shared<LTimer>(std::move(cb),interval,execute_count);
        add_timer(timer);
    }
    void add_timer(LTimerPtr timer);
    void tick();
    
private:

    static const int MAX_SLOTS=4;
    static const int SI=10;
    int cur_slot_;
    NodePtr slots_[MAX_SLOTS];
    std::shared_ptr<EventHandler> handler_;
};
   
}    
}
#endif