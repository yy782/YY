#include "EventHandler.h"
#include "../Common/LogFilter.h"
#include "EventLoop.h"
namespace yy
{
namespace net
{
EventHandler::EventHandler(int fd,EventLoop* loop,const std::string& addInformation,Event events):
status_(-1),
fd_(fd),
events_(events),
revents_(Event(LogicEvent::None)),
loop_(loop)
{
    assert(loop_!=nullptr);
    loop_->addListen(this,addInformation);
} 
EventHandler::EventHandler(Event events):
status_(-1),
events_(events),
revents_(Event(LogicEvent::None))
{

}
void EventHandler::init(int fd,EventLoop* loop,const std::string& addInformation)
{
    assert(fd_==-1);
    assert(fd!=-1);
    assert(loop);
    loop_=loop;
    fd_=fd;
    loop_->addListen(this,addInformation);  
}  
// void EventHandler::tie(const std::shared_ptr<void>& obj)
// {
//   tie_ = obj;
//   tied_=true;
// }
void EventHandler::handler_revent()
{
    // std::shared_ptr<void> guard;
    // if(tied_)
    // {
    //     guard=tie_.lock();
    //     if(!guard)return;        
    // }
    if(revents_&LogicEvent::Hup&&!(revents_&LogicEvent::Read))
    {
        // @brief 这里判断一下ReadEvent主要是对方关闭了写端，但是可能还有可读数据未读，HupEvent是即将关闭连接
        // 但是HupEvent和RdHupEvent的事件分界比较模糊?
        if(closeCallback_) closeCallback_();
    }  
    if(revents_&LogicEvent::Nval)
    {
        if(errorCallback_)errorCallback_();
    }
    if(revents_&LogicEvent::Error)
    {
        if(errorCallback_)errorCallback_();
    }
    if(revents_&LogicEvent::Except)
    {
        if(exceptCallback_)
        {
            exceptCallback_();
        }
    }
    if(revents_&LogicEvent::Read||revents_&LogicEvent::RdHup)
    {
        if(readCallback_)
        {
            readCallback_();
        }
    }
    if(revents_&LogicEvent::Write)
    {
        if(writeCallback_)writeCallback_();
    }
}
void EventHandler::update()
{
    assert(loop_);
    loop_->update_listen(this);
}
void EventHandler::removeListen(const std::string& removeInformation)
{
    assert(loop_);
    
    loop_->remove_listen(this,removeInformation);
}
}
}    
