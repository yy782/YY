
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
// void EventLoop::loop()
// {

//     static int LoopNum=0;

//     status_&=~EventLoopStatus::Init;
//     status_|=EventLoopStatus::Looping;
//     assert(CheckeEventLoopStatus());
//     while(status_&EventLoopStatus::Looping)
//     {
//         activeHandlers_.clear();
//         poller_.poll(PollTimeMs,activeHandlers_);
//         TimeStamp<HighPrecision> Now=TimeStamp<HighPrecision>::now();
//         LOG_SYSTEM_DEBUG()
//         status_|=EventLoopStatus::EventHandling;
//         assert(CheckeEventLoopStatus());
//         for(auto& handler:activeHandlers_)
//         {
//             assert(handler!=nullptr);
//             handler->handler_revent();
//         }
//         assert(status_&EventLoopStatus::EventHandling);
//         status_&=~EventLoopStatus::EventHandling;
//         doPendingFunctions();
//         if(status_&EventLoopStatus::Quiting)break;
//     }
//     while(!FunctionList_.empty())
//         doPendingFunctions();
//     status_=EventLoopStatus::Quited;
// } 
void EventLoop::loop()
{

    

    status_&=~EventLoopStatus::Init;
    status_|=EventLoopStatus::Looping;
    assert(CheckeEventLoopStatus());
    while(status_&EventLoopStatus::Looping)
    {
        activeHandlers_.clear();
        poller_.poll(PollTimeMs,activeHandlers_);
        // TimeStamp<HighPrecision> Now=TimeStamp<HighPrecision>::now();
        // LOG_SYSTEM_DEBUG(name_<<" 第"<<(++LoopNum)<<"次事件循环"<<" Timer:"<<Now.nowToString());

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
    });
    uint64_t one =1;
    ssize_t n=sockets::write(wakeHandler_.get_fd(), &one, sizeof one);
    assert(n==sizeof one);
}
// void EventLoop::submit(Functor cb)
// {
//     if(isInLoopThread())
//     {
//         cb();
//     }
//     else 
//     {
//         assert(cb);
//         if(cb.getName()!="NoName")
//         {
//             ++num;
//             LOG_SYSTEM_DEBUG("loopAddr:"<<this<<" submit "<<cb.getName()<<" had push:"<<num);
//         }

//         // FunctionList_.blockappend(std::move(cb));    
//         std::lock_guard<std::mutex> l(mtx_);    
//         FunctionList_.push_back(std::move(cb));
//         wakeup();
//     }
// }
void EventLoop::submit(Functor cb)
{
    if(isInLoopThread())
    {
        cb();
    }
    else 
    {
        // TimeStamp<HighPrecision> Now=TimeStamp<HighPrecision>::now();
        // LOG_SYSTEM_DEBUG(name_<<"be submited "<<cb.getName()<<" Timer:"<<Now.nowToString()); 
        // FunctionList_.blockappend(std::move(cb));    
        std::lock_guard<std::mutex> l(mtx_);    
        FunctionList_.push_back(std::move(cb));
        wakeup();
    }
}
// void EventLoop::wakeup()
// {

//     uint64_t one =1;
//     ssize_t n=sockets::write(wakeHandler_.get_fd(), &one, sizeof one);
//     assert(n==sizeof one);          

// }
// void EventLoop::wakeup()
// {
//     if(!(status_&EventLoopStatus::EventHandling)&&!(status_&EventLoopStatus::PendingFunctions))
//     {
//         uint64_t one =1;
//         ssize_t n=sockets::write(wakeHandler_.get_fd(), &one, sizeof one);
//         assert(n==sizeof one);        
//     }
// }
void EventLoop::wakeup()
{

    uint64_t one =1;
    ssize_t n=sockets::write(wakeHandler_.get_fd(), &one, sizeof one);
    assert(n==sizeof one);        
    
}
// void EventLoop::doPendingFunctions()
// {
//     size_t FinishNum=0;
//     status_|=EventLoopStatus::PendingFunctions;
//     while(!FunctionList_.empty()&&FinishNum<30) 
//     {
//         Functor cb;
//         FunctionList_.retrieve(cb);

//         // if(cb.getName()!="NoName")
//         // {
//         //     LOG_SYSTEM_DEBUG("loopAddr:"<<this<<" handle "<<cb.getName()<<" had pop:"<<(++num2));
    
//         // }
        
//         cb(); 
//         ++FinishNum;
//     }
//     status_&=~EventLoopStatus::PendingFunctions;
// }   
// void EventLoop::doPendingFunctions()
// {
//   FunctionList functors;
//   status_|=EventLoopStatus::PendingFunctions;

//   {
//     std::lock_guard<std::mutex> l(mtx_);
//     functors.swap(FunctionList_);
//   }

//   for (const Functor& functor : functors)
//   {
//         functor();
//   }
// status_&=~EventLoopStatus::PendingFunctions;
// }
void EventLoop::doPendingFunctions()
{
  FunctionList functors;
  status_|=EventLoopStatus::PendingFunctions;

  {
    std::lock_guard<std::mutex> l(mtx_);
    functors.swap(FunctionList_);
  }

  for (Functor& functor : functors)
  {
        // TimeStamp<HighPrecision> Now=TimeStamp<HighPrecision>::now();
        // LOG_SYSTEM_DEBUG(name_<<"handle "<<functor.getName()<<" Timer:"<<Now.nowToString());        
        functor();
  }
status_&=~EventLoopStatus::PendingFunctions;
}
}    
}