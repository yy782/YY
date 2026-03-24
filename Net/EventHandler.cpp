#include "EventHandler.h"
#include "../Common/LogFilter.h"
#include "EventLoop.h"
namespace yy
{
namespace net
{
EventHandler::EventHandler(int fd,EventLoop* loop,const char* name):
status_(-1),
fd_(fd),
events_(EventType::NoneEvent),
revents_(EventType::NoneEvent),
loop_(loop),
name_(name)
{
    assert(loop_!=nullptr);
    loop_->addListen(this);
    
 
} 
void EventHandler::init(int fd,EventLoop* loop,const char* name)
{
    assert(fd_==-1);
    assert(fd!=-1);
    assert(loop);
    loop_=loop;
    fd_=fd;
    loop_->addListen(this); 
    name_=name;
}  
void EventHandler::tie(const std::shared_ptr<void>& obj)
{
  tie_ = obj;
  tied_=true;
}
void EventHandler::handler_revent()
{
    std::shared_ptr<void> guard;
    if(tied_)
    {
        guard=tie_.lock();
        if(!guard)return;        
    }
    if(revents_&EventType::HupEvent&&!(revents_&EventType::ReadEvent))
    {
        // @brief 这里判断一下ReadEvent主要是对方关闭了写端，但是可能还有可读数据未读，HupEvent是即将关闭连接
        // 但是HupEvent和RdHupEvent的事件分界比较模糊?
        EXCLUDE_BEFORE_COMPILATION( 
            LOG_EVENT_DEBUG(printName()<<" handler_revent HupEvent");
        )
        if(closeCallback_) closeCallback_();
    }  
    if(revents_&EventType::NvalEvent)
    {
        EXCLUDE_BEFORE_COMPILATION(
            LOG_EVENT_DEBUG(printName()<<" handler_revent NvalEvent");
        )
        if(errorCallback_)errorCallback_();
    }
    if(revents_&EventType::ErrorEvent)
    {
        EXCLUDE_BEFORE_COMPILATION(
            LOG_EVENT_DEBUG(printName()<<" handler_revent ErrorEvent");
        )
        if(errorCallback_)errorCallback_();
    }
    if(revents_&EventType::ExceptEvent)
    {
        EXCLUDE_BEFORE_COMPILATION(
            LOG_EVENT_DEBUG(printName()<<" handler_revent ExceptEvent");
        )
        if(exceptCallback_)
        {
            exceptCallback_();
        }
    }
    if(revents_&EventType::ReadEvent||revents_&EventType::RdHupEvent)
    {
        EXCLUDE_BEFORE_COMPILATION(
            LOG_EVENT_DEBUG(printName()<<" handler_revent ReadEvent");
        )
        if(readCallback_)
        {
            readCallback_();
        }
    }
    if(revents_&EventType::WriteEvent)
    {
        EXCLUDE_BEFORE_COMPILATION(
            LOG_EVENT_DEBUG(printName()<<" handler_revent WriteEvent");
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
