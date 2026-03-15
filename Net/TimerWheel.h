#ifndef _YY_NET_TIMETWHEEL_H_
#define _YY_NET_TIMETWHEEL_H_

#include "../Common/noncopyable.h"
#include "../Common/TimeStamp.h"
#include <memory>
#include <assert.h>
#include "Timer.h"
#include "EventHandler.h"
#include <vector>
namespace yy
{
namespace net
{


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
    TimerWheel(EventLoop* loop,int maxSlots,int SI);  
    ~TimerWheel();
    void insert(TimerCallBack cb,LTimer::Time_Interval interval,int execute_count)
    {
        auto timer=std::make_shared<LTimer>(std::move(cb),interval,execute_count);
        insert(timer);
    }
    void insert(LTimerPtr timer);
    void tick();
    
private:
    void ReadTimerfd();
    const int maxSlots_;
    const int SI_;
    int cur_slot_;
    std::vector<NodePtr> slots_;
    EventHandler handler_;
};
   
}    
}
#endif