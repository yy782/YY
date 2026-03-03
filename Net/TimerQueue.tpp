#include <assert.h>
#include <vector>
#include "Timer.h"
#include "sockets.h"

#include "TimerQueue.h"
#include "EventLoop.h"
namespace yy
{
namespace net
{
template<typename PrecisionTag>
TimerQueue<PrecisionTag>::TimerQueue(EventLoop* loop):
handler_(sockets::create_timerfd(CLOCK_MONOTONIC,TFD_CLOEXEC|TFD_NONBLOCK),loop)
{
    
    handler_.setReadCallBack(std::bind(&TimerQueue::handlerRead,this));
    handler_.setReading();


    if constexpr (std::is_same_v<PrecisionTag,LowPrecision>)
    {
        handler_.set_name("LTimerQueue");
    }
    else
    {
        handler_.set_name("HTimerQueue");
    }

    
    loop->addListen(&handler_);
}
template<typename PrecisionTag>
TimerQueue<PrecisionTag>::~TimerQueue()
{
    
}    
template<typename PrecisionTag>
void TimerQueue<PrecisionTag>::insert(TimerCallBack cb,int interval,int execute_count)
{
    auto timer=std::make_shared<PTimer>(std::move(cb),interval,execute_count);
   insert(timer);
}
template<typename PrecisionTag>
void TimerQueue<PrecisionTag>::insert(TimerPtr timer)
{
    assert(timer!=nullptr);
    auto& when=timer->getTimerStamp();
    assert(when>Time_Stamp::now());
    auto it=timers_.begin();
    
    if(it==timers_.end()||when<it->first)
    {
        modifyTimerfd(timer);
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
    ReadTimerfd();

    LOG_TIME_DEBUG("TimerQueue handlerRead");

    std::vector<Entry> expired=getDueTasks(Time_Stamp::now());
    std::vector<Entry> NewTasks;
    for(auto& it:expired)
    {
        if(it.second->remain_count()<=0)continue;
        it.second->execute();
        if(it.second->remain_count())NewTasks.push_back(it);
    }
    
    timers_.insert(NewTasks.begin(),NewTasks.end());

    if(timers_.size()!=0)modifyTimerfd(timers_.begin()->second);
}
template<typename PrecisionTag>
void TimerQueue<PrecisionTag>::modifyTimerfd(const TimerPtr& timer)
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
    
    int fd=handler_.get_fd();
    auto diff_us=timer->getTimeInterval().getTimePeriod().count();
    struct itimerspec new_ts{};
    new_ts.it_value.tv_sec=diff_us/1000000;            
    new_ts.it_value.tv_nsec=(diff_us%1000000)*1000; 
    sockets::timerfd_settime(fd,0,new_ts); 
}
template<typename PrecisionTag>
std::vector<typename TimerQueue<PrecisionTag>::Entry> TimerQueue<PrecisionTag>::getDueTasks(const Time_Stamp& now)
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
template<typename PrecisionTag>
void TimerQueue<PrecisionTag>::ReadTimerfd()
{
    uint64_t howmany;
    ssize_t n=sockets::read(handler_.get_fd(),&howmany,sizeof howmany);
    if(n!=sizeof howmany){
        LOG_TIME_ERROR("TimerQueue::ReadTimerfd() read error");
    }
} 
}    
}

