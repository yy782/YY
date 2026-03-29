
#include "EventLoop.h"
#include "TimerQueue.h"
namespace yy
{
namespace net
{

namespace EventLoopStatus {
    constexpr int Init = 1 << 0;
    constexpr int Looping = 1 << 1;
    constexpr int EventHandling = 1 << 2;
    constexpr int PendingFunctions = 1 << 3;
    constexpr int Quiting = 1 << 4;
    constexpr int Quited = 1 << 5;
}


const int PollTimeMs=100;

    
bool EventLoop::CheckeEventLoopStatus()
{
    if((status_&EventLoopStatus::EventHandling)&&(status_&EventLoopStatus::PendingFunctions))return false;
    if((status_&EventLoopStatus::Quiting)&&(status_&EventLoopStatus::Init))return false;
    if((status_&EventLoopStatus::Init)&&status_!=EventLoopStatus::Init)return false;
    return true;
}

EventLoop::EventLoop(int id):
id_(id),
activeHandlers_(),
FunctionList_(10),
threadId_(Thread::getId()),
status_(EventLoopStatus::Init),
wakeHandler_(sockets::createEventFdOrDie(0,EFD_NONBLOCK|EFD_CLOEXEC),this,"wakeHandler",Event(LogicEvent::Read|LogicEvent::Edge))

{
    auto eventCallBack=[this]() -> void
    {
        uint64_t one = 1;
        ssize_t n = sockets::read(wakeHandler_.fd(), &one, sizeof one);
        assert(n == sizeof one);
        
        return;
    };
    wakeHandler_.setReadCallBack(eventCallBack);
}
bool EventLoop::isQuit() noexcept
{
    return status_&EventLoopStatus::Quiting;
}
void EventLoop::loop()
{

    
    assert(isInLoopThread());
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
bool EventLoop::isInLoopThread() noexcept
{
    return Thread::isSelf(threadId_);
}
void EventLoop::quit()
{
    //assert(!isInLoopThread());
    assert(CheckeEventLoopStatus());
    
    submit([this]()/////////////////////////////////////////////////loop线程提交安全吗
    {
        assert(isInLoopThread());
        status_ = EventLoopStatus::Quiting;     
    },std::string("loopID:"+std::to_string(id_)+"quit"));
    uint64_t one =1;
    ssize_t n=sockets::write(wakeHandler_.fd(), &one, sizeof one);
    assert(n==sizeof one);
}
void EventLoop::wakeup()
{
    if(!(status_&EventLoopStatus::EventHandling)&&!(status_&EventLoopStatus::PendingFunctions))
    {
        uint64_t one =1;
        ssize_t n=sockets::write(wakeHandler_.fd(), &one, sizeof one);  
        assert(n==sizeof one);        
    }
}
void EventLoop::doPendingFunctions()
{
    assert(isInLoopThread());
    size_t FinishNum=0;
    status_|=EventLoopStatus::PendingFunctions;

    if(FunctionList_.full())
    {
        LOG_LOOP_WARN("loopID:"<<id_<<" FunctionList is full");
    }

    while(!FunctionList_.empty()&&FinishNum<30) 
    {
        Functor cb;
        FunctionList_.retrieve(cb);
        LOG_LOOP_DEBUG("loopID:"<<id_<<" "<<cb.getName());
        cb(); 
        ++FinishNum;
    }
    status_&=~EventLoopStatus::PendingFunctions;
}   

}    
}