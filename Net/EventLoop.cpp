
#include "EventLoop.h"
namespace yy
{
namespace net
{
namespace 
{
enum EventLoopStatus
{
    Init=1<<0,

    Looping=1<<1,
    EventHandling=1<<2,
    PendingFunctions=1<<3,

    Quit=1<<4    
};

const int PollTimeMs=100;
}
    


EventLoop::EventLoop():
poller_(),
activeHandlers_(),
status_(EventLoopStatus::Init),
wakeupHandler_(sockets::create_eventfd(0,EFD_NONBLOCK|EFD_CLOEXEC),this)
{
    auto eventCallBack=[this](EventHandler::ReadCallBack)->void
    {
        uint64_t one=1;
        ssize_t n=sockets::YYread(wakeupHandler_.get_fd(),&one,sizeof one);
        assert(n==sizeof one);
        return;
    };
    wakeupHandler_.setReadCallBack(std::bind(eventCallBack));
    wakeupHandler_.setReading();
    poller_.add_listen(wakeupHandler_);
}
void EventLoop::loop()
{
    status_|=EventLoopStatus::Looping;
    assert(CheckeEventLoopStatus());
    while(status_&EventLoopStatus::Looping)
    {
        activeHandlers_.clear();
        pollReturnTime_=poller_.poll(PollTimeMs,activeHandlers_);
        ++iteration_;
        status_|=EventLoopStatus::EventHandling;
        assert(CheckeEventLoopStatus());
        for(auto& handler:activeHandlers_)
        {
            assert(handler!=nullptr);
            handler->handler_revent();
        }
        assert(status_&EventLoopStatus::EventHandling);
        status_&=~EventLoopStatus::EventHandling;
        doPendingFunctions();
    }
} 
void EventLoop::quit()
{
    status_|=EventLoopStatus::Quit;
    assert(CheckeEventLoopStatus());
    wakeup();
}
void EventLoop::wakeup()
{
    uint64_t one =1;
    ssize_t n=sockets::YYwrite(wakeupHandler_.get_fd(), &one, sizeof one);
    assert(n==sizeof one);
}
void EventLoop::doPendingFunctions()
{
    status_|=EventLoopStatus::PendingFunctions;
    assert(CheckeEventLoopStatus());
    for(const auto& functor:FunctionList_)
    {
        functor();
    }
    status_&=~EventLoopStatus::PendingFunctions;
    assert(CheckeEventLoopStatus());
}   
}    
}