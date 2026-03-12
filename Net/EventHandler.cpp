#include "EventHandler.h"
#include "../Common/LogFilter.h"
#include "EventLoop.h"
namespace yy
{
namespace net
{
EventHandler::EventHandler(int fd,EventLoop* loop):
status_(-1),
fd_(fd),
events_(EventType::NoneEvent),
revents_(EventType::NoneEvent),
loop_(loop)
{
    assert(loop_!=nullptr);
} 
  
void EventHandler::handler_revent()
{

    if(revents_&EventType::HupEvent&&!(revents_&EventType::ReadEvent))
    {
        // @brief 这里判断一下ReadEvent主要是对方关闭了写端，但是可能还有可读数据未读，HupEvent是即将关闭连接
        // 但是HupEvent和RdHupEvent的事件分界比较模糊?
        IGNORE( 
            LOG_EVENT_DEBUG(printName()<<" handler_revent HupEvent");
        )
        if(closeCallback_) closeCallback_();
    }  
    if(revents_&EventType::NvalEvent)
    {
        IGNORE(
            LOG_EVENT_DEBUG(printName()<<" handler_revent NvalEvent");
        )
        if(errorCallback_)errorCallback_();
    }
    if(revents_&EventType::ErrorEvent)
    {
        IGNORE(
            LOG_EVENT_DEBUG(printName()<<" handler_revent ErrorEvent");
        )
        if(errorCallback_)errorCallback_();
    }
    if(revents_&EventType::ExceptEvent)
    {
        IGNORE(
            LOG_EVENT_DEBUG(printName()<<" handler_revent ExceptEvent");
        )
        if(exceptCallback_)
        {
            exceptCallback_();
        }
    }
    if(revents_&EventType::ReadEvent||revents_&EventType::RdHupEvent)
    {
        IGNORE(
            LOG_EVENT_DEBUG(printName()<<" handler_revent ReadEvent");
        )
        if(readCallback_)
        {
            readCallback_();
        }
    }
    if(revents_&EventType::WriteEvent)
    {
        IGNORE(
            LOG_EVENT_DEBUG(printName()<<" handler_revent WriteEvent");
        )
        if(writeCallback_)writeCallback_();
    }
}
void EventHandler::removeListen()
{
    assert(loop_);
    loop_->remove_listen(this);
}
}
}    
