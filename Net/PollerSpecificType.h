
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
