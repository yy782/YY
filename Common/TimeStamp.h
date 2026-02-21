#ifndef YY_COMMON_TIMESTAMP_H
#define YY_COMMON_TIMESTAMP_H
#include <chrono>
#include <sys/timerfd.h>
#include "copyable.h"
#include "Types.h"
namespace yy
{
   
struct LowPrecisionTag;
struct HighPrecisionTag;

typedef LowPrecisionTag LowPrecision;
typedef HighPrecisionTag HighPrecision;

template<typename PrecisionTag>
struct ClockTraits;

template<>
struct ClockTraits<LowPrecision>
{
    typedef std::chrono::system_clock Clock;
    typedef std::chrono::seconds S;
};


template<>
struct ClockTraits<HighPrecision>
{
    typedef std::chrono::high_resolution_clock Clock;
    typedef std::chrono::microseconds S;
};

template<typename PrecisionTag=LowPrecisionTag>
class TimeStamp:copyable
{
public:    
    typedef std::chrono::time_point<typename ClockTraits<PrecisionTag>::Clock> TimePoint;
    TimeStamp():
    time_point_()
    {}
    TimeStamp(TimePoint time_point):
    time_point_(time_point)
    {}
    void flush(){
        time_point_=now();
    }
    auto get_time_point()const
    {
        return time_point_;
    }
    static TimePoint now(){return ClockTraits<PrecisionTag>::Clock::now();} 
private:
    TimePoint time_point_;
};
template<typename PrecisionTag=LowPrecisionTag>
class TimeInterval:copyable
{
public:
    typedef typename ClockTraits<PrecisionTag>::S TimePeriod;
    TimeInterval(TimePeriod timePeriod):
    timePeriod_(timePeriod)
    {}
    TimePeriod getTimePeriod()const{return timePeriod_;}
    long getTimes()const{return timePeriod_.count();}
private:
    TimePeriod timePeriod_;    
};


template<typename PrecisionTag>
bool operator==(const TimeStamp<PrecisionTag>& lhs,const TimeStamp<PrecisionTag>& rhs)
{
    return lhs.get_time_point()==rhs.get_time_point();
}
template<typename PrecisionTag>
bool operator!=(const TimeStamp<PrecisionTag>& lhs,const TimeStamp<PrecisionTag>& rhs)
{
    return !(lhs==rhs);
}
template<typename PrecisionTag>
bool operator<(const TimeStamp<PrecisionTag>& lhs,const TimeStamp<PrecisionTag>& rhs)
{
    return lhs.get_time_point()<rhs.get_time_point();

}
template<typename PrecisionTag>
bool operator>(const TimeStamp<PrecisionTag>& lhs,const TimeStamp<PrecisionTag>& rhs)
{
    return rhs<lhs;
}
template<typename PrecisionTag>
bool operator<=(const TimeStamp<PrecisionTag>& lhs,const TimeStamp<PrecisionTag>& rhs)
{
    return !(lhs>rhs);

}
template<typename PrecisionTag> 
bool operator>=(const TimeStamp<PrecisionTag>& lhs,const TimeStamp<PrecisionTag>& rhs)
{
    return !(lhs<rhs);
}

template<typename PrecisionTag>
size_t operator-(const TimeStamp<PrecisionTag>& lhs,const TimeStamp<PrecisionTag>& rhs)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(lhs.get_time_point()-rhs.get_time_point()).count();
}



template<class PrecisionTag>
struct timespec convert(TimeStamp<PrecisionTag> timer)
{
    using namespace std::chrono;
    auto duration=timer.get_time_point().time_since_epoch();
    auto sec=duration_cast<seconds>(duration);
    auto nsec=duration_cast<nanoseconds>(duration-sec);

    timespec ts;
    memZero(&ts,sizeof ts);
    ts.tv_sec=sec.count();
    ts.tv_nsec=nsec.count();
    return ts;
}

}
#endif