#include <assert.h>
#include <vector>
#include "Timer.h"
#include "sockets.h"

#include "TimerQueue.h"

namespace yy
{
namespace net
{
template<typename PrecisionTag>
TimerQueue<PrecisionTag>::TimerQueue(EventLoop* loop):
handler_()
{
    int fd=sockets::createTimerFdOrDie(CLOCK_MONOTONIC,TFD_CLOEXEC|TFD_NONBLOCK);
    handler_.setReadCallBack(std::bind(&TimerQueue::handlerRead,this));
    if constexpr (std::is_same_v<PrecisionTag,LowPrecision>)
    {
        handler_.init(fd,loop,"LTimerQueue");
    }
    else
    {
        handler_.init(fd,loop,"HTimerQueue");
    }

    handler_.setReading();

}
template<typename PrecisionTag>
TimerQueue<PrecisionTag>::~TimerQueue()
{
    
}    
template<typename PrecisionTag>
void TimerQueue<PrecisionTag>::insert(BaseTimer::TimerCallBack cb,typename PTimer::Time_Interval interval,int execute_count)
{
    auto timer=std::make_shared<PTimer>(std::move(cb),interval,execute_count);
    insertInLoop(timer);
}
template<typename PrecisionTag>
void TimerQueue<PrecisionTag>::insert(TimerPtr timer)
{
    insertInLoop(timer);
}
template<typename PrecisionTag>
void TimerQueue<PrecisionTag>::insertInLoop(TimerPtr timer)
{
    assert(handler_.get_loop()->isInLoopThread());
    //LOG_TIME_DEBUG(timers_.size());
    
    assert(timer!=nullptr);
    
#ifdef NDEBUG    
    timers_.insert(Entry(timer->getTimerStamp(),timer)); 
#else
   auto result=timers_.insert(Entry(timer->getTimerStamp(),timer)); 
   assert(result.second);
#endif 
    modifyTimerfd();
}
template<typename PrecisionTag>
void TimerQueue<PrecisionTag>::cancelTimer(TimerPtr timer)
{
    cancelTimerInLoop(timer);
}  
template<typename PrecisionTag>
void TimerQueue<PrecisionTag>::cancelTimerInLoop(TimerPtr timer)
{
    assert(handler_.get_loop()->isInLoopThread());
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
    assert(handler_.get_loop()->isInLoopThread());
    ReadTimerfd();

    //LOG_TIME_DEBUG("TimerQueue handlerRead");

    std::vector<Entry> expired=getDueTasks(Time_Stamp::now());
    std::vector<Entry> NewTasks;
    for(auto& it:expired)
    {
        if(it.second->remain_count()<=0)continue;
        it.second->execute();
        if(it.second->remain_count())NewTasks.push_back(it);
    }
    
    timers_.insert(NewTasks.begin(),NewTasks.end());

    modifyTimerfd();
}
template<typename PrecisionTag>
void TimerQueue<PrecisionTag>::modifyTimerfd()
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
    assert(handler_.get_loop()->isInLoopThread());
    if(timers_.empty())
    {
        return;
    }
    else 
    {
        auto timer=timers_.begin()->second;
        int ret=sockets::timerfd_settime(handler_.get_fd(),0,*timer.get()); 
        if (ret<0) // 简单的忽略
        {
            timers_.erase(Entry(timer->getTimerStamp(),timer));
            modifyTimerfd();
        }      
    }

}
template<typename PrecisionTag>
std::vector<typename TimerQueue<PrecisionTag>::Entry> TimerQueue<PrecisionTag>::getDueTasks(const Time_Stamp& now)
{
    assert(handler_.get_loop()->isInLoopThread());
    std::vector<Entry> expired;
    Entry sentry(now,TimerPtr());


    typename TimerList::iterator end=timers_.upper_bound(sentry);
    assert(end==timers_.end()||now<end->first);
    std::move(timers_.begin(),end,back_inserter(expired));

    timers_.erase(timers_.begin(),end);

  return expired;
}  
template<typename PrecisionTag>
void TimerQueue<PrecisionTag>::ReadTimerfd()
{
    assert(handler_.get_loop()->isInLoopThread());
    uint64_t howmany;
    ssize_t n=sockets::read(handler_.get_fd(),&howmany,sizeof howmany);
    if(n!=sizeof howmany){
        EXCLUDE_BEFORE_COMPILATION(
            LOG_TIME_ERROR("TimerQueue::ReadTimerfd() read error:"<<errno);
        )
    }
} 
}    
}
