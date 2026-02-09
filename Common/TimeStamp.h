#ifndef YY_COMMON_TIMESTAMP_H
#define YY_COMMON_TIMESTAMP_H
#include <chrono>

#include "copyable.h"

namespace yy
{
class TimeStamp:copyable
{
public:    
    TimeStamp():
    time_point_(std::chrono::high_resolution_clock::now())
    {}
    auto get_time_point()const
    {
        return time_point_;
    } 
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> time_point_;
};
bool operator==(const TimeStamp& lhs,const TimeStamp& rhs)
{
    return lhs.get_time_point()==rhs.get_time_point();
}
bool operator!=(const TimeStamp& lhs,const TimeStamp& rhs)
{
    return !(lhs==rhs);
}
bool operator<(const TimeStamp& lhs,const TimeStamp& rhs)
{
    return lhs.get_time_point()<rhs.get_time_point();

}
bool operator>(const TimeStamp& lhs,const TimeStamp& rhs)
{
    return rhs<lhs;
}
bool operator<=(const TimeStamp& lhs,const TimeStamp& rhs)
{
    return !(lhs>rhs);

}
bool operator>=(const TimeStamp& lhs,const TimeStamp& rhs)
{
    return !(lhs<rhs);
}
}

#endif