#include "EventHandler.h"
#include "../Common/Log.h"
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
  
void EventHandler::handler_revent(Time_Stamp receiveTime)
{

if(revents_&EventType::HupEvent&&!(revents_&EventType::ReadEvent))
{
    // @brief 这里判断一下ReadEvent主要是对方关闭了写端，但是可能还有可读数据未读，HupEvent是即将关闭连接
    // 但是HupEvent和RdHupEvent的事件分界比较模糊?
    LOG_CLIENT_DEBUG(printName()<<" handler_revent HupEvent");
    if(closeCallback_) closeCallback_();
}  
if(revents_&EventType::NvalEvent)
{
    LOG_CLIENT_DEBUG(printName()<<" handler_revent NvalEvent");
    if(errorCallback_)errorCallback_();
}
if(revents_&EventType::ErrorEvent)
{
    LOG_CLIENT_DEBUG(printName()<<" handler_revent ErrorEvent");
    if(errorCallback_)errorCallback_();
}
if(revents_&EventType::ReadEvent||revents_&EventType::RdHupEvent||revents_&EventType::ExceptEvent)
{
     LOG_CLIENT_DEBUG(printName()<<" handler_revent ReadEvent");
    if(readCallback_)readCallback_(receiveTime);
}
if(revents_&EventType::WriteEvent)
{
     LOG_CLIENT_DEBUG(printName()<<" handler_revent WriteEvent");
    if(writeCallback_)writeCallback_();
}
}

}
}    
