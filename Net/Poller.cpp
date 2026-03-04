#include "Poller.h"
#include "sockets.h"
#include "Event.h"
#include "EventHandler.h"
#include "../Common/LogFilter.h"
namespace yy
{
namespace net
{




// TimeStamp<LowPrecision> Select::poll(int timeout,HandlerList& event_handlers)
// {
//     struct timeval block_time={timeout/1000,(timeout%1000)*1000};
//     fd_set read_fds=read_fds_;
//     fd_set write_fds=write_fds_;
//     fd_set except_fds=except_fds_;
//     auto ready_fds=::select(max_fd_+1,&read_fds,&write_fds,&except_fds,&block_time);
//     int save_errno=errno;
//     TimeStamp<LowPrecision> timer(TimeStamp<LowPrecision>::now());
//     if(ready_fds<0)
//     {
//         LOG_PRINT_ERRNO(save_errno);
//         return timer;
//     }
//     else if(ready_fds==0)
//     {
//         return timer;
//     }
//     else
//     {
//         for(int fd=0;fd<=max_fd_&&ready_fds;++fd)
//         {
//             auto handler=get_event_handler(this,fd);
//             assert(handler!=nullptr&&"关联的事件为空");
//         // @note 异常文件描述符集合里会有带外数据，错误事件，可能有挂断事件，，读文件描述符集合里可能有挂断事件，需要谨慎判断    
//             Event event;
//             if(check_fd(fd,read_fds_))
//             {
//                 char buf[1];
//                 auto n=sockets::recv(handler->get_fd(),buf,1,MSG_PEEK|MSG_DONTWAIT);
//                 int save_errno1=errno;
//                 if(n==0)
//                 {
//                     event.add_event(EventType::HupEvent);
//                 }
//                 else if(n>0)
//                 {
//                     event.add_event(EventType::ReadEvent);  
//                 }
//                 else if(n<0&&errno!=EAGAIN&&errno!=EWOULDBLOCK)
//                 {
//                     LOG_PRINT_ERRNO(save_errno1);
//                 }  
//             }
//             if(check_fd(fd,write_fds_))
//             {
//                 event.add_event(EventType::WriteEvent);
//             }
//             if(check_fd(fd,except_fds_))
//             {
//                 if(sockets::sockfd_has_error(handler->get_fd()))
//                 {
//                     event.add_event(EventType::ErrorEvent);
//                 }
//                 else
//                 {
//                     event.add_event(EventType::ExceptEvent);
//                 }
//             }
//             if(event==EventType::NoneEvent)continue;
//             --ready_fds;
//             //assert(handler->get_status()==Added);
//             //event_handlers.push_back(handler);
//             handler->set_revent(event);            
//         }
//         return timer;
//     }
//         return timer;
// }
// void Select::add_listen(EventHandler& handler)
// {
//     //assert(handler.get_status()==New||
//     //        handler.get_status()==Delete);
//     //add_handler(this,handler);
//     update_max_fd(handler.get_fd());
// #ifndef NDEBUG
//     int event_nums=0;
//     if(handler.isReading())
//     {
//         set_fd(handler.get_fd(),read_fds_);
//         event_nums++;
//     }
//     if(handler.isWriting())
//     {
//         set_fd(handler.get_fd(),write_fds_);
//         event_nums++;
//     }
//     if(handler.isExcept())
//     {
//         set_fd(handler.get_fd(),except_fds_);
//         event_nums++;
//     }  
//     assert(event_nums!=0&&"需要监听的事件是?");
// #else
//     if(handler.isReading())set_fd(handler.get_fd(),read_fds_);
//     if(handler.isWriting())set_fd(handler.get_fd(),write_fds_);
//     if(handler.isExcept())set_fd(handler.get_fd(),except_fds_);        
// #endif                
// }
// void Select::update_listen(EventHandler& handler)
// {
//     assert(has_handler(this,&handler));
//     //assert(handler.get_status()==Added);
// #ifndef NDEBUG
//     int event_nums=0;
//     auto fd=handler.get_fd();
//     if(handler.isReading()&&!check_fd(fd,read_fds_))
//     {
//         set_fd(handler.get_fd(),read_fds_);
//         event_nums++;
//     }
//     if(handler.isWriting()&&!check_fd(fd,write_fds_))
//     {
//         set_fd(handler.get_fd(),write_fds_);
//         event_nums++;
//     }
//     if(handler.isExcept()&&!check_fd(fd,except_fds_))
//     {
//         set_fd(handler.get_fd(),except_fds_);
//         event_nums++;
//     }  
//     assert(event_nums!=0&&"需要更新的事件是?");
// #else
//     if(handler.isReading())set_fd(handler.get_fd(),read_fds_);
//     if(handler.isWriting())set_fd(handler.get_fd(),write_fds_);
//     if(handler.isExcept())set_fd(handler.get_fd(),except_fds_);        
// #endif        
// }
// void Select::remove_listen(EventHandler& handler)
// {
//     //assert(handler.get_status()==Added);
//     //handler.set_status(Delete);
//     remove_handler(this,handler);
//     update_max_fd(handler.get_fd());      
// #ifndef NDEBUG    
//     int event_nums=0;
//     if(handler.isReading())
//     {
//         remove_fd(handler.get_fd(),read_fds_);
//         event_nums++;
//     }
//     if(handler.isWriting())
//     {
//         remove_fd(handler.get_fd(),write_fds_);
//         event_nums++;
//     }
//     if(handler.isExcept())
//     {
//         remove_fd(handler.get_fd(),except_fds_);
//         event_nums++;
//     }  
//     assert(event_nums!=0&&"事件未注册,却要删除?");        
// #else
//     if(handler.isReading())remove_fd(handler.get_fd(),read_fds_);
//     if(handler.isWriting())remove_fd(handler.get_fd(),write_fds_);
//     if(handler.isExcept())remove_fd(handler.get_fd(),except_fds_);
// #endif    
// }
// void Select::update_max_fd(int fd)
// {    
//     if(safe_static_cast<size_t>(fd)<is_listening_.size())
//     {
//         if(is_listening_[fd])//已注册到select,是删除fd
//         {
//             is_listening_[fd]=false;
//             if(fd==max_fd_)
//             {
//                 for(auto it=is_listening_.rbegin();it!=is_listening_.rend();++it)
//                 {
//                     if(*it)
//                     {
//                         max_fd_=*it;
//                         break;
//                     }
//                 }
//             }
//         }
//         else//没有注册到select,是添加fd
//         {
//             is_listening_[fd]=true;
//             max_fd_=(fd>max_fd_)?fd:max_fd_;
//         }
//     }
//     else//没有注册到select,是添加fd
//     {
//         is_listening_.resize(fd+1);
//         is_listening_[fd]=true;
//         max_fd_=fd;
//     }
// }

/*
    struct pollfd
    {
    int fd;
    short events;
    short revents;
    }
*/ 
// Poll::Poll():
// pollfds_(1024)
// {
//     for(size_t i=0;i<pollfds_.size();++i)
//     {
//         pollfds_[i].fd=-1;
//     }
// }

// TimeStamp<LowPrecision> Poll::poll(int timeout,HandlerList& event_handlers)
// {
//     int ready_fds=::poll(&*pollfds_.begin(),pollfds_.size(),timeout);
//     int save_errno=errno;
//     auto timer=TimeStamp<LowPrecision>::now();
//     if(ready_fds<0)
//     {
//         LOG_PRINT_ERRNO(save_errno);
//         return timer;
//     }
//     else if(ready_fds==0)
//     {
//         return timer;
//     }
//     else
//     {
//         for(size_t i=0;i<pollfds_.size()&&ready_fds;++i)
//         {
//             auto revents=pollfds_[i].revents;
//             if(revents==0)continue;
//             auto fd=pollfds_[i].fd;
//             --ready_fds;
            
//             auto handler=get_event_handler(this,fd);
//             assert(handler->get_status()!=-1);
//             assert(safe_static_cast<size_t>(handler->get_status())==i);
//             assert(handler->get_event()==EventType::NoneEvent);
//             handler->set_revent(Event(safe_static_cast<int>(revents)));
//             event_handlers.push_back(handler);
//         }
//     }
//     return timer;
// }
// void Poll::add_listen(EventHandler& handler)
// {
//     assert(handler.get_status()==-1&&"状态不对");
//     size_t idx=pollfds_.size();
//     struct pollfd pfd;
//     pfd.events=safe_static_cast<short>(handler.get_event().get_event());
//     pfd.fd=handler.get_fd();
//     pfd.revents=0;
//     pollfds_.push_back(pfd);
//     assert(idx==pollfds_.size()-1);
//     handler.set_status(safe_static_cast<int>(idx));

//     assert(handlers_.find(handler.get_fd())==handlers_.end());
//     handlers_[handler.get_fd()]=handler;
// }
// void Poll::update_listen(EventHandler& handler)
// {
//     assert(handler.get_event()!=EventType::NoneEvent);
//     int idx=handler.get_status();
//     assert(static_cast<size_t>(idx)<pollfds_.size());
//     assert(handlers_.find(handler.get_fd())!=handlers_.end());
//     assert(pollfds_[idx].fd==handler.get_fd()||
//             pollfds_[idx].fd==-handler.get_fd()-1);
//     pollfds_[idx].events=safe_static_cast<short>(handler.get_event().get_event());

//     pollfds_[idx].fd=handler.get_fd();//这是取消忽视
//     if(handler.has_ignore())
//     {
//         pollfds_[idx].fd=-handler.get_fd()-1;
//     }
//     //  @brief 当pollfd的fd为负数时，内核会省略对该fd的监听，如果不想将pollfd移出监听集合时，可以设置成-fd-1
    
// }
// void Poll::remove_listen(EventHandler& handler)
// {
//     size_t idx=safe_static_cast<size_t>(handler.get_status());
//     assert(idx<pollfds_.size());
//     assert(handlers_.find(handler.get_fd())!=handlers_.end());
//     assert(pollfds_[idx].fd==handler.get_fd());
//     size_t n=handlers_.erase(handler.get_fd());
//     assert(n==1);

//     if(idx==pollfds_.size()-1)
//     {
//         pollfds_.pop_back();
//     }
//     else
//     {
//         int back_fd=pollfds_.back().fd;
//         iter_swap(pollfds_.begin()+idx, pollfds_.end()-1);
//         if(back_fd<0)
//         {
//         back_fd=-back_fd-1;
//         }
        
//         pollfds_.pop_back();
//     }
//     handler.set_status(-1);
// }
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
epollfd_(sockets::create_epollfd(EPOLL_CLOEXEC))
{
    assert(epollfd_>0);
}
Epoll::~Epoll()
{
    if(::close(epollfd_)<0)
    {
        LOG_PRINT_ERRNO(errno);
    }
}
TimeStamp<LowPrecision> Epoll::poll(int timeout,HandlerList& event_handlers)
{
    auto ready_fds=::epoll_wait(epollfd_,&*events_.begin(),
                    static_cast<int>(events_.size()),timeout);
    int save_errno=errno;
    auto timer=TimeStamp<LowPrecision>::now();
    if(ready_fds<0)
    {
        if((save_errno==EAGAIN)||(save_errno==EWOULDBLOCK))
        {
            LOG_PRINT_ERRNO(save_errno);
        }
        return timer;
    }
    else if(ready_fds==0)
    {
        return timer;
    }                
    else
    {

        for(int i=0;i<ready_fds;++i)
        {
           
            
            uint32_t revents=events_[i].events;
            
            auto fd=events_[i].data.fd;    

#ifndef NDEBUG
            auto it=handlers_.find(fd);
            assert(it!=handlers_.end());
            auto handler=it->second;
#else
            auto handler=handlers_[fd];
#endif            
  

            handler->set_revent(Event(revents));
            // @note 这里是uint32_t到int的隐形转换，但是epollfd_返回的就绪事件是在0X00000000-0x0000001F,不会溢出

            event_handlers.push_back(handler);
        }
    }
    if(static_cast<size_t>(ready_fds)==events_.size())
    {
        events_.resize(events_.size()*2);
    }
    return timer;
}
void Epoll::add_listen(EventHandler* handler)
{
    assert(handler);
    LOG_SYSTEM_DEBUG("添加监听 "<<handler->printName());

    operator_epoll(EPOLL_CTL_ADD,handler);

    assert((handler->get_status()==New||
            handler->get_status()==Delete));

#ifndef NDEBUG
    auto it=handlers_.find(handler->get_fd());
    assert(it==handlers_.end());
#endif   

    assert(handlers_.find(handler->get_fd())==handlers_.end());
    handlers_[handler->get_fd()]=handler;

    handler->set_status(Added);
}
void Epoll::update_listen(EventHandler* handler)
{
    assert(handler);
    assert(handler->get_status()==Added);
    
#ifndef NDEBUG
    auto it=handlers_.find(handler->get_fd());
    assert(it!=handlers_.end());
#endif

    operator_epoll(EPOLL_CTL_MOD,handler);
}
void Epoll::remove_listen(EventHandler* handler)
{
    assert(handler);




    assert(handler->get_status()==Added);

#ifndef NDEBUG
    auto it=handlers_.find(handler->get_fd());
    assert(it!=handlers_.end());
#endif

    operator_epoll(EPOLL_CTL_DEL,handler);
    
    assert(handlers_.find(handler->get_fd())!=handlers_.end());
#ifndef NDEBUG
    size_t n=handlers_.erase(handler->get_fd());
    assert(n==1);        
#else
    handlers_.erase(handler->get_fd()); 
#endif

    handler->set_status(Delete);
}
void Epoll::operator_epoll(int operation,EventHandler* handler)
{
    assert(handler);
    struct epoll_event ev;
    memZero(&ev,sizeof ev);
    ev.events=handler->get_event().get_event();
    ev.data.fd=handler->get_fd();
    if(::epoll_ctl(epollfd_,operation,handler->get_fd(),&ev)==-1)
    {
            if((errno!=EAGAIN)&&(errno!=EWOULDBLOCK))
            {
                LOG_PRINT_ERRNO(errno);
            }
    }    
}
}    
}