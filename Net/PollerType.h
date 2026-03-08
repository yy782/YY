

#ifdef POLL
    #include "Poller/Poll.h"
    typedef Poll PollerType;
#elif defined(EPOLL)
    #include "Poller/Epoll.h"
    typedef Epoll PollerType;
#elif defined(SELECT)
    #include "Poller/Select.h"
    typedef Select PollerType;  
#else
    #include "Poller/Epoll.h"
    typedef yy::net::Epoll PollerType;
#endif
   
