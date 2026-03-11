#ifndef YY_NET_POLLER_TYPE_H_
#define YY_NET_POLLER_TYPE_H_
namespace yy 
{
namespace net 
{
class Poll;
class Select;    
class Epoll;
#ifdef POLL
    typedef Poll PollerType;
#elif defined(EPOLL)
    typedef Epoll PollerType;
#elif defined(SELECT)
    typedef Select PollerType;  
#else
    typedef Epoll PollerType;
#endif     

}  
}
#endif

#ifdef _NEED_SPECIFIC_POLLER_TYPE_
#ifndef POLLER_SPECIFIC_TYPE_H_
#define POLLER_SPECIFIC_TYPE_H_
#ifdef POLL
    #include "Poller/Poll.h"
#elif defined(EPOLL)
    #include "Poller/Epoll.h"
#elif defined(SELECT)
    #include "Poller/Select.h"
#else
    #include "Poller/Epoll.h"
#endif

#endif
#endif




   
