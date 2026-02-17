#ifndef _YY_NET_POLLER_ 
#define _YY_NET_POLLER_

#include <vector>
#include <type_traits>
#include <map>

#include <sys/select.h>
#include <sys/epoll.h>
#include <poll.h>

#include "../Common/noncopyable.h"
#include "../Common/TimeStamp.h"



namespace yy
{
namespace net
{

class EventHandler;

template<class PollerTag>
class Poller:noncopyable
{
public:
    typedef std::vector<EventHandler*> HandlerList;
    
    
    TimeStamp<LowPrecision> poll(int timeout,HandlerList& event_handler)// @param timeout是毫秒为单位
    {
        static_assert(has_poll_v<PollerTag>,"成员不存在");
        return static_cast<PollerTag*>(this).poll(timeout,event_handler);
        // @note 这里不能值转换，会创建新的对象
    }
    ~Poller()=default;
    void add_listen(EventHandler& event_handler)
    {
        static_assert(has_add_listen_v<PollerTag>,"成员不存在");
        return static_cast<PollerTag*>(this).add_listen(event_handler);
    }
    void update_listen(EventHandler& event_handler)
    {
        static_assert(has_update_listen_v<PollerTag>,"成员不存在");
        return static_cast<PollerTag*>(this).update_listen(event_handler);
    }
    void remove_listen(EventHandler& event_handler)
    {
        static_assert(has_remove_listen_v<PollerTag>,"成员不存在");
        return static_cast<PollerTag*>(this).remove_listen(event_handler);
    }

  
protected:    
    std::map<int,EventHandler*> event_handlers_;
    // @brief 选择指针存储，1.避免拷贝开销，2.EventHandler拒绝拷贝
    Poller()=default; // @note CRTP模型下不能创建Poller对象，只能创建子类对象，因为无法解释子类多出的内存布局

    template<class PollerTag_>
    friend EventHandler* get_event_handler(const Poller<PollerTag_>* poller,int fd);

    template<class PollerTag_>
    friend void add_handler(Poller<PollerTag_>* poller,EventHandler& handler);

    template<class PollerTag_>
    friend void remove_handler(Poller<PollerTag_>* poller,EventHandler& handler);

    template<class PollerTag_>
    friend bool has_handler(const Poller<PollerTag_>* poller,EventHandler* event_handler);
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
    template<class T>
    static auto has_add_listen(int)->decltype
    (
        std::declval<T>().add_listen(
            std::declval<EventHandler&>()
        ),
        std::true_type{}
    );
    template<class T>
    static auto has_add_listen(...)->decltype
    (
        std::false_type{}
    );
    template<class T>
    static auto has_update_listen(int)->decltype
    (
        std::declval<T>().update_listen(
            std::declval<EventHandler&>()
        ),
        std::true_type{}
    );
    template<class T>
    static auto has_update_listen(...)->decltype
    (
        std::false_type{}
    );
public:
    template<class T>
    constexpr static bool has_poll_v=decltype(has_poll<T>(0))::value;  
    template<class T>
    constexpr static bool has_remove_listen_v=decltype(has_remove_listen<T>(0))::value;
    template<class T>
    constexpr static bool has_add_listen_v=decltype(has_add_listen<T>(0))::value;
    template<class T>
    constexpr static bool has_update_listen_v=decltype(has_update_listen<T>(0))::value;
};
class Select:public Poller<Select>
{
    // @note Select的拉黑没有Poll和EPoll那样修改事件结构体这么简单，只能移除事件集，等同于删除，所以Select的"拉黑"我没完善
public:
    Select():
    max_fd_(-1),
    is_listening_(1024,false)
    {
        clear_set(read_fds_);
        clear_set(write_fds_);
        clear_set(except_fds_);
    }
    ~Select()
    {
        clear();
    }    
    TimeStamp<LowPrecision> poll(int timeout,HandlerList& event_handlers);

    void add_listen(EventHandler& event_handler);
    void update_listen(EventHandler& event_handler);
    void remove_listen(EventHandler& event_handler);

private:
    void update_max_fd(int fd);
    void clear()
    {
        clear_set(read_fds_);
        clear_set(write_fds_);
        clear_set(except_fds_);
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

    int max_fd_;

    std::vector<bool> is_listening_;
    // @brief 主要是找最大的文件描述符，，，选择用vector<bool>因为fd分配原则下不太需要erase,性能更好
};
class Poll:public Poller<Poll>
{
public:
    Poll();
    ~Poll()=default;
    TimeStamp<LowPrecision> poll(int timeout,HandlerList& event_handlers);
    void add_listen(EventHandler& event_handler);
    void update_listen(EventHandler& event_handler);
    void remove_listen(EventHandler& event_handler);
private:
    typedef std::vector<pollfd> PollFdList;
    PollFdList pollfds_;
};

class Epoll:public Poller<Epoll>
{
public:
    Epoll();
    ~Epoll();
    TimeStamp<LowPrecision> poll(int timeout,HandlerList& event_handlers);
    void add_listen(EventHandler& event_handler);
    void update_listen(EventHandler& event_handler);
    void remove_listen(EventHandler& event_handler);
private:
    void operator_epoll(int operation,EventHandler& event_handler);

    typedef std::vector<struct ::epoll_event> EventList;
    EventList events_;
    int epollfd_;
};

#ifdef POLL
    typedef Poll PollerType;
#elif defined(EPOLL)
    typedef Epoll PollerType;
#elif defined(SELECT)
    typedef Select PollerType;  
#else
    typedef Epoll PollerType;
#endif

}    
}

#endif