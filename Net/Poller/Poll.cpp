#include "Poll.h"


namespace yy 
{
namespace net 
{
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
//     pfd.fd=handler.fd();
//     pfd.revents=0;
//     pollfds_.push_back(pfd);
//     assert(idx==pollfds_.size()-1);
//     handler.set_status(safe_static_cast<int>(idx));

//     assert(handlers_.find(handler.fd())==handlers_.end());       
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
}    
}