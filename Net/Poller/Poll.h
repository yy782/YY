

#include "../Poller.h"

namespace yy 
{
namespace net 
{
class Poll:public Poller<Poll>
{
public:
    Poll();
    ~Poll()=default;
    TimeStamp<LowPrecision> poll(int timeout,HandlerList& event_handlers);
    void add_listen(EventHandler& handler);
    void update_listen(EventHandler& handler);
    void remove_listen(EventHandler& handler);
private:
    typedef std::vector<pollfd> PollFdList;
    PollFdList pollfds_;
};
}
}