#ifndef _YY_NET_TIMERQUEUE_H_
#define _YY_NET_TIMERQUEUE_H_


#include <set>
#include <memory>
#include "../Common/noncopyable.h"
#include "../Common/TimeStamp.h"
#include <sys/timerfd.h>
#include <vector>
namespace yy
{
namespace net
{

template<class PrecisionTag>
class Timer;

class EventLoop;
class EventHandler;


class TimerQueue:public noncopyable
{// @brief TimerQueue主要负责高精度的定时器任务
public:
    typedef Timer<HighPrecision> HTimer;
    typedef TimeStamp<HighPrecision> Time_Stamp;
    typedef std::shared_ptr<HTimer> HTimerPtr;
    typedef std::pair<Time_Stamp,HTimerPtr> Entry;
    typedef std::set<Entry> TimerList;
    //typedef std::set<std::pair<TimerPtr,int64_t>> QuerySet;// @brief int64_t是定时器的身份ID,靠TimerPtr(通过原始元素地址)比较，无法保证键唯一性
    // @note muduo库由于受限于C++11标准，std::set不支持异构查找,选择用裸指针存储，我们使用的是C++17标准，选择完善他

    TimerQueue()=delete;
    TimerQueue(EventLoop* loop);
    ~TimerQueue();
    void insert(HTimer* timer);
    void cancelTimer(HTimer* timer);
    void handlerRead();


private:
    void modifyTimerfd(Time_Stamp timestamp);
    std::vector<Entry> getDueTasks(Time_Stamp now); 

    EventHandler* const handler_;
    TimerList timers_;
    //QuerySet querySet_;
};

}    
}

#endif