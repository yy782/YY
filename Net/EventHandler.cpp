#include "EventHandler.h"
#include "../Common/LogFilter.h"
#include "EventLoop.h"
namespace yy
{
namespace net
{
EventHandler::EventHandler(int fd,EventLoop* loop,const char* name,Event events):
fd_(fd),
loop_(loop),
name_(name)
{
    assert(loop_!=nullptr);
    events_=events;
    loop_->addListen(this);
    
 
} 
EventHandler::EventHandler(Event events):
events_(events)
{
    
}
void EventHandler::init(int fd,EventLoop* loop,const char* name)
{
    assert(fd_==-1);
    assert(fd!=-1);
    assert(loop);
    loop_=loop;
    fd_=fd;
    name_=name;
    loop_->addListen(this); 
}  
// void EventHandler::tie(const std::shared_ptr<void>& obj)
// {
//   tie_ = obj;
//   tied_=true;
// }
void EventHandler::handler_revent()
{
    if(revents_&LogicEvent::Hup&&!(revents_&LogicEvent::Read))
    {
        // @brief 这里判断一下ReadEvent主要是对方关闭了写端，但是可能还有可读数据未读，HupEvent是即将关闭连接
        // 但是HupEvent和RdHupEvent的事件分界比较模糊?
        EXCLUDE_BEFORE_COMPILATION( 
            LOG_EVENT_DEBUG(printName()<<" handler_revent Hup");
        )
        if(closeCallback_) closeCallback_();
    }  
    if(revents_&LogicEvent::Nval)
    {
        EXCLUDE_BEFORE_COMPILATION(
            LOG_EVENT_DEBUG(printName()<<" handler_revent Nval");
        )
        if(errorCallback_)errorCallback_();
    }
    if(revents_&LogicEvent::Error)
    {
        EXCLUDE_BEFORE_COMPILATION(
            LOG_EVENT_DEBUG(printName()<<" handler_revent Error");
        )
        if(errorCallback_)errorCallback_();
    }
    if(revents_&LogicEvent::Except)
    {
        EXCLUDE_BEFORE_COMPILATION(
            LOG_EVENT_DEBUG(printName()<<" handler_revent Except");
        )
        if(exceptCallback_)
        {
            exceptCallback_();
        }
    }
    if(revents_&LogicEvent::Read||revents_&LogicEvent::RdHup)
    {
        EXCLUDE_BEFORE_COMPILATION(
            LOG_EVENT_DEBUG(printName()<<" handler_revent Read");
        )
        if(readCallback_)
        {
            readCallback_();
        }
    }
    if(revents_&LogicEvent::Write)
    {
        EXCLUDE_BEFORE_COMPILATION(
            LOG_EVENT_DEBUG(printName()<<" handler_revent Write");
        )
        if(writeCallback_)writeCallback_();
    }
}
void EventHandler::update()
{
    assert(loop_);
    loop_->update_listen(this);
}
void EventHandler::removeListen()
{
    assert(loop_);
    
    loop_->remove_listen(this);
}
}
}    
