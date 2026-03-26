#ifndef YY_NET_POLLER_EPOLL_H_
#define YY_NET_POLLER_EPOLL_H_
#include "../Poller.h"
#include <sys/epoll.h>
#include <vector>

namespace yy 
{
namespace net 
{
class Epoll:public Poller<Epoll>
{
public:
    Epoll();
    ~Epoll();
    void poll(int timeout,HandlerList& event_handlers);
    void add_listen(EventHandler* handler);
    void update_listen(EventHandler* handler);
    void remove_listen(EventHandler* handler);
private:
    void operator_epoll(int operation,EventHandler* handler);

    typedef std::vector<struct ::epoll_event> EventList;
    EventList events_;
    int epollfd_;
};     
}    
}
#endif