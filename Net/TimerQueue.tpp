#include <assert.h>
#include <vector>
#include "Timer.h"
#include "sockets.h"
#include "EventHandler.h"
#include "TimerQueue.h"
namespace yy
{
namespace net
{
template<typename PrecisionTag>
TimerQueue<PrecisionTag>::TimerQueue(EventLoop* loop):
handler_(EventHandler::create(sockets::create_timerfd(CLOCK_MONOTONIC,TFD_CLOEXEC|TFD_NONBLOCK),loop))
{
    assert(handler_);
    handler_->setReadCallBack(std::bind(&TimerQueue::handlerRead,this));
    handler_->setReading();
}
template<typename PrecisionTag>
TimerQueue<PrecisionTag>::~TimerQueue()
{
    
}    
template<typename PrecisionTag>
void TimerQueue<PrecisionTag>::insert(TimerCallBack cb,int interval,int execute_count)
{
    auto timer=std::make_shared<PTimer>(std::move(cb),interval,execute_count);
    assert(timer==nullptr);
    auto when=timer->getTimerStamp();
    assert(when<Time_Stamp::now());
    auto it=timers_.begin();
    
    if(it==timers_.end()||when<it->first)
    {
        modifyTimerfd(timer->getTimerStamp());
    }
#ifdef NDEBUG    
    timers_.insert(Entry(timer->getTimerStamp(),timer)); 
#else
   auto result=timers_.insert(Entry(timer->getTimerStamp(),timer)); 
   assert(result.second);
#endif    
}
template<typename PrecisionTag>
void TimerQueue<PrecisionTag>::cancelTimer(PTimer* timer)
{
    assert(timer);
#ifdef NDEBUG
    timers_.erase(Entry(timer->getTimerStamp(),timer));
#else
    size_t n1=timers_.erase(Entry(timer->getTimerStamp(),timer));
    assert(n1==1);
#endif
}  
template<typename PrecisionTag>
void TimerQueue<PrecisionTag>::handlerRead()
{
    std::vector<Entry> expired=getDueTasks(Time_Stamp::now());
    std::vector<Entry> NewTasks;
    for(auto& it:expired)
    {
        if(it.second->remain_count()==0)continue;
        it.second->execute();
        if(it.second->remain_count())NewTasks.push_back(it);
    }
    
    timers_.insert(NewTasks.begin(),NewTasks.end());

    modifyTimerfd(timers_.begin()->first);
}
template<typename PrecisionTag>
void TimerQueue<PrecisionTag>::modifyTimerfd(Time_Stamp timestamp)
{
/*
    struct timespec {
        time_t tv_sec;  // 秒（seconds）
        long   tv_nsec; // 纳秒（nanoseconds），范围 0 ~ 999,999,999
    };

    struct itimerspec {
        struct timespec it_interval; // 重复间隔（周期性定时器用）
        struct timespec it_value;    // 首次超时时间（一次性/周期性定时器都需要）
    };        
*/    
    assert(timestamp<Time_Stamp::now());
    int fd=handler_->get_fd();
    struct timespec value=convert(timestamp); 
    sockets::timerfd_settime(fd,0,NULL,&value);
}
template<typename PrecisionTag>
std::vector<typename TimerQueue<PrecisionTag>::Entry> TimerQueue<PrecisionTag>::getDueTasks(Time_Stamp now)
{
    std::vector<Entry> expired;
    Entry sentry(now,TimerPtr());
    typename TimerList::iterator end=timers_.upper_bound(sentry);
    assert(end==timers_.end()||now<end->first);
    std::move(timers_.begin(),end,back_inserter(expired));
#ifdef NDEBUG  
    timers_.erase(timers_.begin(),end);
#else
    auto n=timers_.erase(timers_.begin(),end);
    assert(n->second);
#endif
  return expired;
}   
}    
}

