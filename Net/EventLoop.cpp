
#include "EventLoop.h"
#include "TimerQueue.h"
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

    Quiting=1<<4,
    
    Quited=1<<5
};

const int PollTimeMs=100;
}
    
bool EventLoop::CheckeEventLoopStatus()
{
    if((status_&EventLoopStatus::EventHandling)&&(status_&EventLoopStatus::PendingFunctions))return false;
    if((status_&EventLoopStatus::Quiting)&&(status_&EventLoopStatus::Init))return false;
    if((status_&EventLoopStatus::Init)&&status_!=EventLoopStatus::Init)return false;
    return true;
}

EventLoop::EventLoop():
poller_(),
activeHandlers_(),
FunctionList_(),
threadId_(Thread::getId()),
status_(EventLoopStatus::Init),
wakeHandler_(sockets::createEventFdOrDie(0,EFD_NONBLOCK|EFD_CLOEXEC),this,"wakeupHandler")
{
    auto eventCallBack=[this]()->void
    {
        uint64_t one=1;
        ssize_t n=sockets::read(wakeHandler_.get_fd(),&one,sizeof one);
        assert(n==sizeof one);
        
        
        return;
    };
    wakeHandler_.setReadCallBack(eventCallBack);
    wakeHandler_.setReading();
}
bool EventLoop::isQuit()
{
    return status_&EventLoopStatus::Quiting;
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
        assert(status_&EventLoopStatus::EventHandling);
        status_&=~EventLoopStatus::EventHandling;
        doPendingFunctions();
        if(status_&EventLoopStatus::Quiting)break;
    }
    while(!FunctionList_.empty())
        doPendingFunctions();
    status_=EventLoopStatus::Quited;
} 
bool EventLoop::isInLoopThread()
{
    return Thread::isSelf(threadId_);
}
void EventLoop::quit()
{

    assert(CheckeEventLoopStatus());
    submit([this](){
        status_=EventLoopStatus::Quiting;
        uint64_t one =1;
        ssize_t n=sockets::writeAuto(wakeHandler_.get_fd(), &one, sizeof one);
        assert(n==sizeof one);        
    });
}
void EventLoop::submit(Functor cb)
{
    if(isInLoopThread())
    {
        cb();
    }
    else 
    {
        assert(cb);
        FunctionList_.blockappend(std::move(cb));        
    }
}
void EventLoop::wakeup()
{
    if(!(status_&EventHandling)&&!(status_&EventLoopStatus::PendingFunctions))
    {
        uint64_t one =1;
        ssize_t n=sockets::writeAuto(wakeHandler_.get_fd(), &one, sizeof one);
        assert(n==sizeof one);          
    }
}
void EventLoop::doPendingFunctions()
{
    thread_local size_t FinishNum=0;
    status_|=EventLoopStatus::PendingFunctions;
    while(!FunctionList_.empty()&&FinishNum<30) // task持续不断，饥饿?
    {
        Functor cb;
        FunctionList_.retrieve(cb); 
        cb(); //调用链过深 ?
        ++FinishNum;
    }
    status_&=~EventLoopStatus::PendingFunctions;
}   
}    
}