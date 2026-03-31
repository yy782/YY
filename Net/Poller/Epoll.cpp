#include "Epoll.h"
#include "../sockets.h"

#include "../../Common/LogFilter.h"

namespace yy 
{
namespace net 
{
/*
struct epoll_event {
    uint32_t events;  // 要监听的事件（如 EPOLLIN/EPOLLOUT）
    epoll_data_t data;// 自定义数据（通常存 fd，方便后续处理）
};
typedef union epoll_data {
    void *ptr;
    int fd;          // 最常用：存要监听的 fd
    uint32_t u32;
    uint64_t u64;
} epoll_data_t;
*/
namespace
{
const int New=-1;
const int Added=1;
const int Delete=2;
}
Epoll::Epoll():
events_(1024),
epollfd_(sockets::createEpollFdOrDie(EPOLL_CLOEXEC))
{
    assert(epollfd_>0);
}
Epoll::~Epoll()
{
    if(::close(epollfd_)<0)
    {
        LOG_ERRNO(errno);
    }
}
void Epoll::poll(int timeout,HandlerList& event_handlers)
{

    auto ready_fds=::epoll_wait(epollfd_,&*events_.begin(),
                    static_cast<int>(events_.size()),timeout);
    
    if(ready_fds<0)
    {
        if((errno!=EAGAIN)&&(errno!=EWOULDBLOCK)&&(errno!=EINTR))

        {
            LOG_ERRNO(errno);
        }
        return;
    }
    else if(ready_fds==0)
    {
        return;
    }                
    else
    {
        for(int i=0;i<ready_fds;++i)
        { 
            uint32_t revents=events_[i].events;
            EventHandler* handler=static_cast<EventHandler*>(events_[i].data.ptr);
            assert(handler);
            handler->set_revent(Event(revents));
            event_handlers.push_back(handler);
        }
    }
    if(static_cast<size_t>(ready_fds)==events_.size())
    {
        events_.resize(events_.size()*2);
    }
    return;
}
void Epoll::add_listen(EventHandler* handler)
{
    assert(handler);
    assert((handler->status()==New||
            handler->status()==Delete));
    assert(handlers_.find(handler->fd())==handlers_.end());
    handlers_[handler->fd()]=handler;       
    handler->set_status(Added);
    operator_epoll(EPOLL_CTL_ADD,handler);
  
}
void Epoll::update_listen(EventHandler* handler)
{
    assert(handler);
    assert(handler->status()==Added);

    assert(has_handler(handler));

    operator_epoll(EPOLL_CTL_MOD,handler);

}
void Epoll::remove_listen(EventHandler* handler)
{
    assert(handler);
    assert(handler->status()==Added);
    assert(has_handler(handler));
    operator_epoll(EPOLL_CTL_DEL,handler);
    remove_handler(handler);
    handler->set_status(Delete);
}
void Epoll::operator_epoll(int operation,EventHandler* handler)
{
    assert(handler);
    struct epoll_event ev;
    memZero(&ev,sizeof ev);
    ev.events=handler->event().value();
    ev.data.ptr=handler;
    if(::epoll_ctl(epollfd_,operation,handler->fd(),&ev)==-1)   
    {
            if((errno!=EAGAIN)&&(errno!=EWOULDBLOCK))
            {
                LOG_ERRNO(errno);
            }
    }    
}    
}    
}
