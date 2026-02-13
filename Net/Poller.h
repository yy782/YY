#ifndef _YY_NET_POLLER_ 
#define _YY_NET_POLLER_

#include <vector>
#include <type_traits>
#include <map>

#include <sys/select.h>

#include "../Common/noncopyable.h"
#include "../Common/TimeStamp.h"
#include "EventHandler.h"
#include "Event.h"
namespace yy
{
namespace net
{
template<class PollTag>
class Poller:noncopyable
{
public:
    Poller()
    {
        
    }

    TimeStamp<HighPrecision> poll(int timeout,std::vector<EventHandler>& event_handler)
    {
        static_assert(has_poll_v<PollTag>,"成员不存在");
        return static_cast<PollTag*>(this).poll(timeout,event_handler);
        // @note 这里不能值转换，会创建新的对象
    }
    ~Poller()
    {
        return static_cast<PollTag*>(this).~PollTag();
    }
    void remove_listen(EventHandler& event_handler)
    {
        static_assert(has_remove_event_v<PollTag>,"成员不存在");
        return static_cast<PollTag*>(this).remove_listen(event_handler);
    }

    template<class T>
    constexpr static bool has_poll_v=decltype(has_poll<T>(0))::value;  
    template<class T>
    constexpr static bool has_remove_event_v=decltype(has_remove_listen<T>(0))::value;  
protected:    
    std::map<int,EventHandler*> event_handler_map_;
    // @brief 选择指针存储，1.避免拷贝开销，2.EventHandler拒绝拷贝

    EventHandler* get_event_handler(int fd)
    {
        auto it=event_handler_map_.find(fd);
        assert(it!=event_handler_map_.end()&&"监听的套接字不存在");
        auto handler=it->second;
        assert(handler->get_fd()==fd&&"奇怪的套接字");
        return handler;
    }
private:
    template<class T>
    static auto has_poll(int)->decltype
    (
        std::declval<T>().poll(
            std::declval<int>(),
            std::declval<std::vector<EventHandler>&>()
        ),
        std::true_type{}
    );
    template<class T>
    static auto has_poll(...)->decltype
    (
        std::false_type{}
    );

    template<class T>
    static auto has_remove_listen(int)->decltype
    (
        std::declval<T>().remove_listen(
            std::declval<EventHandler&>()
        ),
        std::true_type{}
    );
    template<class T>
    static auto has_remove_listen(...)->decltype
    (
        std::false_type{}
    );    


};


class Select:public Poller<Select>
{
public:
    TimeStamp<HighPrecision> poll(int timeout,std::vector<EventHandler*>& event_handlers)
    {
        struct timeval block_time={timeout/1000,(timeout%1000)*1000};

        fd_set read_fds=read_fds_;
        fd_set write_fds=write_fds_;
        fd_set except_fds=except_fds_;
        

        auto ready_fd=::select(max_fd_+1,&read_fds,&write_fds,&except_fds,&block_time);
        TimeStamp<HighPrecision> timer(TimeStamp<HighPrecision>::now());
        if(ready_fd<0)
        {
            LOG_PRINT_ERRNO;
            return timer;
        }
        for(int fd=0;fd<=max_fd_;++fd)
        {
            if(!check_fd(fd,all_fds_))continue;
            auto handler=get_event_handler(fd);
            assert(handler!=nullptr&&"关联的事件为空");
            event_handlers.push_back(handler);
            //Event event;
            if(check_fd(fd,read_fds_))
            {
                //event|=READ;
            }
            if(check_fd(fd,write_fds_))
            {
        //
            }
            if(check_fd(fd,except_fds_))
            {
        //
            }
            //assert(event is not null);
            //handler->set_event(event);            
        }
        return timer;
    }
    void remove_listen(EventHandler& event_handler)
    {

    }
    ~Select()
    {
        clear();
    }
private:
    void clear()
    {
        FD_ZERO(&all_fds_);
        max_fd_=-1;
    }
    void clear_set(fd_set& set)
    {
        FD_ZERO(&set);
        // @brief 由于FD_ZERO是宏展开，不能直接用return FD_ZERO(&set)
    }

    void set_fd(int fd,fd_set& set)
    {
        return  FD_SET(fd,&set);
    }
    void remove_fd(int fd,fd_set& set)
    {
        return FD_CLR(fd,&set);
    }
    bool check_fd(int fd,fd_set& set)
    {
        return FD_ISSET(fd,&set);
    }
    fd_set read_fds_;
    fd_set write_fds_;
    fd_set except_fds_;

    fd_set all_fds_;
    int max_fd_;
};


}    
}

#endif