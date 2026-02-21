#ifndef _YY_NET_TIMER_H_
#define _YY_NET_TIMER_H_

#include <functional>
#include <assert.h>
#include "../Common/noncopyable.h"
#include "../Common/TimeStamp.h"

namespace yy
{
namespace net
{

#define FOREVER -1
template<typename PrecisionTag>  
class Timer:public noncopyable
{
public:
    typedef std::function<void()> TimerCallBack;
    typedef TimeStamp<PrecisionTag> Time_Stamp;
    typedef TimeInterval<PrecisionTag> Time_Interval;

    
    Timer(TimerCallBack cb,int interval,int execute_count):
    callback_(std::move(cb)),
    interval_(interval),
    execute_count_(execute_count),
    expiration_(Time_Stamp::now()+interval)
    {
        assert(execute_count_!=FOREVER&&execute_count_<=0);

    }
    int remain_count()const{return execute_count_;}
    void modifyExecuteCount(int count){execute_count_=count;}
    Time_Stamp getTimerStamp()const{return expiration_;}
    Time_Interval getTimeInterval()const{return interval_;}
    void execute()
    {
        assert(execute_count_!=0);
        callback_();
        if(execute_count_!=FOREVER)
        {
            --execute_count_;
        }
        expiration_=Time_Stamp::now()+interval_.getTimePeriod();
    }

private:
    const TimerCallBack callback_;
    const Time_Interval interval_;
    int execute_count_;
    Time_Stamp expiration_;
};

}    
}

#endif