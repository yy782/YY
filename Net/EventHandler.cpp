#include "EventHandler.h"

namespace yy
{
namespace net
{
  
void EventHandler::handler_revent(Time_Stamp receiveTime)
{

if(revents_&EventType::HupEvent&&!(revents_&EventType::ReadEvent))
{
    // @brief 这里判断一下ReadEvent主要是对方关闭了写端，但是可能还有可读数据未读，HupEvent是即将关闭连接
    // 但是HupEvent和RdHupEvent的事件分界比较模糊?
    if(closeCallback_) closeCallback_();
}  
if(revents_&EventType::NvalEvent)
{
    if(errorCallback_)errorCallback_();
}
if(revents_&EventType::ErrorEvent)
{
    if(errorCallback_)errorCallback_();
}
if(revents_&EventType::ReadEvent||revents_&EventType::RdHupEvent||revents_&EventType::ExceptEvent)
{
    if(readCallback_)readCallback_(receiveTime);
}
if(revents_&EventType::WriteEvent)
{
    if(writeCallback_)writeCallback_();
}

}
}    
}