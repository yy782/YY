#ifndef YY_NET_POLLER_TYPE_H_
#define YY_NET_POLLER_TYPE_H_
namespace yy 
{
namespace net 
{

class Poll;
class Select;    
class Epoll;

}  
}
#if defined(POLL)
    #include "../Poller/Poll.h"
    typedef yy::net::Poll PollerType;
#elif defined(SELECT)
    #include "../Poller/Select.h"
    typedef yy::net::Select PollerType;
#elif defined(EPOLL)
    #include "../Poller/Epoll.h"
    typedef yy::net::Epoll PollerType;    
#else 
    #include "../Poller/Epoll.h"
    typedef yy::net::Epoll PollerType;
#endif

#endif






   
