#ifndef _YY_NET_TIMER_H_
#define _YY_NET_TIMER_H_

#include <functional>
#include <assert.h>
#include "../Common/noncopyable.h"
#include "../Common/TimeStamp.h"
#include <atomic>

namespace yy
{
namespace net
{

#define FOREVER -1


template<typename PrecisionTag>  
class Timer;
typedef std::function<void()> TimerCallBack; 


template<typename PrecisionTag>  
class Timer:public noncopyable
{
public:
    typedef Timer<PrecisionTag> PTimer;
    typedef TimeStamp<PrecisionTag> Time_Stamp;
    typedef TimeInterval<PrecisionTag> Time_Interval;
    
    
    Timer(TimerCallBack cb,Time_Interval interval,int execute_count):
    callback_(std::move(cb)),
    interval_(interval),
    execute_count_(execute_count),
    expiration_(Time_Stamp::now()+interval)
    {
        assert((execute_count_.load()<0&&execute_count_.load()==FOREVER)||execute_count_.load()>=0);
    }
    int remain_count()const{return execute_count_.load();}
    void modifyExecuteCount(size_t count){execute_count_.store(count);}
    Time_Stamp& getTimerStamp(){return expiration_;}
    const Time_Stamp& getTimerStamp()const {return expiration_;}
    Time_Interval& getTimeInterval(){return interval_;}
    const Time_Interval& getTimeInterval()const{return interval_;}
    void execute()
    {
        assert(execute_count_.load()!=0);
        
        if(execute_count_.load()!=FOREVER)
        {
            --execute_count_;
            
        }
        callback_();
        expiration_=Time_Stamp::now()+interval_;
    }
    void setRemainCount(int c){execute_count_=c;}
    void cancel(){execute_count_=0;}
private:
    const TimerCallBack callback_;
    Time_Interval interval_;
    std::atomic<int> execute_count_;
    Time_Stamp expiration_;
};

typedef Timer<LowPrecision> LTimer;
typedef Timer<HighPrecision> HTimer;
typedef std::shared_ptr<LTimer> LTimerPtr;
typedef std::shared_ptr<HTimer> HTimerPtr;
}    
}

#endif