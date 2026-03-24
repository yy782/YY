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
    struct Node;
    typedef std::shared_ptr<Node> NodePtr;
    typedef std::weak_ptr<Node> WeakNodePtr;  
    TimerWheel()=delete;
    TimerWheel(EventLoop* loop,int maxSlots,int SI);  
    ~TimerWheel();
    void insert(Timer<Base>::TimerCallBack cb,LTimer::Time_Interval interval,int execute_count)
    {
        auto timer=std::make_shared<LTimer>(std::move(cb),interval,execute_count);
        insert(timer);
    }
    void insert(LTimerPtr timer);
    void tick();
    
private:
    void ReadTimerfd();
    const int fd_;
    const int maxSlots_;
    const int SI_;
    int cur_slot_;
    std::vector<NodePtr> slots_;
    EventLoop* loop_;
    EventHandler handler_;
};
   
}    
}
#endif