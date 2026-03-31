#ifndef YY_NET_POLLER_SELECT_H_
#define YY_NET_POLLER_SELECT_H_
#include "../include/Poller.h"

namespace yy 
{
namespace net 
{
// class Select:public Poller<Select>
// {
// public:
//     Select():
//     max_fd_(-1),
//     is_listening_(1024,false)
//     {
//         clear_set(read_fds_);
//         clear_set(write_fds_);
//         clear_set(except_fds_);
//     }
//     ~Select()
//     {
//         clear();
//     }    
//     TimeStamp<LowPrecision> poll(int timeout,HandlerList& event_handlers);

//     void add_listen(EventHandler& handler);
//     void update_listen(EventHandler& handler);
//     void remove_listen(EventHandler& handler);

// private:
//     void update_max_fd(int fd);
//     void clear()
//     {
//         clear_set(read_fds_);
//         clear_set(write_fds_);
//         clear_set(except_fds_);
//         max_fd_=-1;
//     }
//     void clear_set(fd_set& set)
//     {
//         FD_ZERO(&set);
//         // @brief 由于FD_ZERO是宏展开，不能直接用return FD_ZERO(&set)
//     }
//     void set_fd(int fd,fd_set& set)
//     {
//         return  FD_SET(fd,&set);
//     }
//     void remove_fd(int fd,fd_set& set)
//     {
//         return FD_CLR(fd,&set);
//     }
//     bool check_fd(int fd,fd_set& set)
//     {
//         return FD_ISSET(fd,&set);
//     }
//     fd_set read_fds_;
//     fd_set write_fds_;
//     fd_set except_fds_;

//     int max_fd_;

//     std::vector<bool> is_listening_;
//     // @brief 主要是找最大的文件描述符，，，选择用vector<bool>因为fd分配原则下不太需要erase,性能更好
// };    
}
}
#endif