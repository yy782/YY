#ifndef YY_COMMON_TIMESTAMP_H
#define YY_COMMON_TIMESTAMP_H
#include <chrono>

#include "copyable.h"

namespace yy
{
   
struct LowPrecisionTag{};
struct HighPrecisionTag{};

typedef LowPrecisionTag LowPrecision;
typedef HighPrecisionTag HighPrecision;

template<typename PrecisionTag>
struct ClockTraits;

template<>
struct ClockTraits<LowPrecision>
{
    typedef std::chrono::system_clock Clock;
};


template<>
struct ClockTraits<HighPrecision>
{
    typedef std::chrono::high_resolution_clock Clock;
};

template<typename PrecisionTag=LowPrecisionTag>
class TimeStamp:copyable
{
public:    
    TimeStamp():
    time_point_(ClockTraits<PrecisionTag>::Clock::now())
    {}
    void flush(){
        time_point_=ClockTraits<PrecisionTag>::Clock::now();
    }
    auto get_time_point()const
    {
        return time_point_;
    } 
private:
    std::chrono::time_point<typename ClockTraits<PrecisionTag>::Clock> time_point_;
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
}
#endif