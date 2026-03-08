#include "Select.h"

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
}    
}