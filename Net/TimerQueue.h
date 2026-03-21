#ifndef _YY_NET_TIMERQUEUE_H_
#define _YY_NET_TIMERQUEUE_H_

#include "EventLoop.h"
#include <set>
#include <memory>
#include "../Common/noncopyable.h"
#include "../Common/TimeStamp.h"
#include <sys/timerfd.h>
#include <vector>
#include <memory>
#include "Timer.h"
#include "EventHandler.h"
#include <utility>
namespace yy
{
namespace net
{






template<class PrecisionTag>
class TimerQueue:public noncopyable
{// @brief TimerQueue主要负责高精度的定时器任务
public:
    typedef Timer<PrecisionTag> PTimer;
    typedef TimeStamp<PrecisionTag> Time_Stamp;
    typedef std::shared_ptr<PTimer> TimerPtr;
    typedef std::shared_ptr<EventHandler> EventHandlerPtr;
    typedef std::pair<Time_Stamp,TimerPtr> Entry;
    typedef std::set<Entry> TimerList;
    //typedef std::set<std::pair<TimerPtr,int64_t>> QuerySet;// @brief int64_t是定时器的身份ID,靠TimerPtr(通过原始元素地址)比较，无法保证键唯一性
    // @note muduo库由于受限于C++11标准，std::set不支持异构查找,选择用裸指针存储，我们使用的是C++17标准，选择完善他

    TimerQueue()=delete;
    TimerQueue(EventLoop* loop);
    ~TimerQueue();
    void insert(TimerCallBack cb,typename PTimer::Time_Interval interval,int execute_count);
    void insert(TimerPtr timer);
    void cancelTimer(TimerPtr timer);
    void handlerRead();
    void insertInLoop(TimerPtr timer);
    void cancelTimerInLoop(TimerPtr timer);

private:
    void ReadTimerfd();
    void modifyTimerfd();
    std::vector<Entry> getDueTasks(const Time_Stamp& now); 


    EventHandler handler_;
    TimerList timers_;
    //QuerySet querySet_;
};

template<class PrecisionTag>
void EventLoop::runTimer(TimerCallBack cb,typename Timer<PrecisionTag>::Time_Interval interval,int execute_count)
{
    submit([this,cb=std::move(cb),interval=std::move(interval),execute_count]()mutable
    {///////////////////////////////////////////////
        assert(isInLoopThread());
        thread_local static TimerQueue<PrecisionTag> queue(this);
        auto timer=std::make_shared<Timer<PrecisionTag>>(std::move(cb),std::move(interval),execute_count);
        queue.insertInLoop(timer);
    });
}
}    
}

#include "TimerQueue.tpp"

#endif