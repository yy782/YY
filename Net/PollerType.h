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






   
