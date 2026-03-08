
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
    
bool EventLoop::CheckeEventLoopStatus()
{
    if((status_&EventLoopStatus::EventHandling)&&(status_&EventLoopStatus::PendingFunctions))return false;
    if((status_&EventLoopStatus::Quit)&&status_!=EventLoopStatus::Quit)return false;
    if((status_&EventLoopStatus::Init)&&status_!=EventLoopStatus::Init)return false;
    return true;
}

EventLoop::EventLoop():
poller_(),
activeHandlers_(),
status_(EventLoopStatus::Init),
wakeupHandler_(sockets::create_eventfd(0,EFD_NONBLOCK|EFD_CLOEXEC),this)
{
    auto eventCallBack=[this]()->void
    {
        uint64_t one=1;
        ssize_t n=sockets::read(wakeupHandler_.get_fd(),&one,sizeof one);
        assert(n==sizeof one);
        status_=EventLoopStatus::Quit;
        
        return;
    };
    wakeupHandler_.setReadCallBack(eventCallBack);
    wakeupHandler_.setReading();
    wakeupHandler_.set_name("wakeupHandler");
    poller_.add_listen(&wakeupHandler_);
}
void EventLoop::loop()
{
    status_&=~EventLoopStatus::Init;
    status_|=EventLoopStatus::Looping;
    assert(CheckeEventLoopStatus());
    while(status_&EventLoopStatus::Looping)
    {
        activeHandlers_.clear();
        poller_.poll(PollTimeMs,activeHandlers_);
        status_|=EventLoopStatus::EventHandling;
        assert(CheckeEventLoopStatus());
        for(auto& handler:activeHandlers_)
        {
            assert(handler!=nullptr);
            handler->handler_revent();
        }
        if(status_==EventLoopStatus::Quit)break;
        assert(status_&EventLoopStatus::EventHandling);
        status_&=~EventLoopStatus::EventHandling;
        doPendingFunctions();
    }

    assert(CheckeEventLoopStatus());
} 
bool EventLoop::isInLoopThread()
{
    return Thread::isSelf(threadId_);
}
void EventLoop::quit()
{

    assert(CheckeEventLoopStatus());
    wakeup();
}
void EventLoop::submit(Functor cb)
{
    FunctionList_.blockappend(std::move(cb));
}
void EventLoop::wakeup()
{
    uint64_t one =1;
    ssize_t n=sockets::write(wakeupHandler_.get_fd(), &one, sizeof one);
    assert(n==sizeof one);
}
void EventLoop::doPendingFunctions()
{
    status_|=EventLoopStatus::PendingFunctions;
    assert(CheckeEventLoopStatus());
    while(!FunctionList_.empty())
    {
        Functor cb;
        FunctionList_.retrieve(cb);
        cb();
    }
    status_&=~EventLoopStatus::PendingFunctions;
    assert(CheckeEventLoopStatus());
}   
}    
}