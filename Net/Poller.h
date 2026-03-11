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
#include "EventHandler.h"
#include <memory>

namespace yy
{
namespace net
{
typedef std::vector<EventHandler*> PollerHandlerList;
template<class PollerTag>
class Poller:noncopyable
{
public:
    
    typedef PollerHandlerList HandlerList;
    
    TimeStamp<LowPrecision> poll(int timeout,HandlerList& handler)// @param timeout是毫秒为单位
    {
        static_assert(has_poll_v<PollerTag>,"成员不存在");
        return static_cast<PollerTag*>(this).poll(timeout,handler);
        // @note 这里不能值转换，会创建新的对象
    }
    ~Poller()=default;
    void add_listen(EventHandler* handler)
    {
        static_assert(has_add_listen_v<PollerTag>,"成员不存在");
        return static_cast<PollerTag*>(this).add_listen(handler);
    }
    void update_listen(EventHandler* handler)
    {
        static_assert(has_update_listen_v<PollerTag>,"成员不存在");
        return static_cast<PollerTag*>(this).update_listen(handler);
    }
    void remove_listen(EventHandler* handler)
    {
        static_assert(has_remove_listen_v<PollerTag>,"成员不存在");
        return static_cast<PollerTag*>(this).remove_listen(handler);
    }

  
protected:    
    std::map<int,EventHandler*> handlers_;
    // @brief 选择指针存储，1.避免拷贝开销，2.EventHandler拒绝拷贝
    Poller()=default; // @note CRTP模型下不能创建Poller对象，只能创建子类对象，因为无法解释子类多出的内存布局

    EventHandler* get_event_handler(int fd);

    void add_handler(EventHandler* handler);

    void remove_handler(EventHandler* handler);

    bool has_handler(EventHandler* event_handler);

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

template<class PollerTag>
EventHandler* Poller<PollerTag>::get_event_handler(int fd)
{
    auto it=handlers_.find(fd);
    assert(it!=handlers_.end()&&"监听的套接字不存在");
    auto handler=it->second;
    assert(handler->get_fd()==fd&&"奇怪的套接字");
    return handler;
}

template<class PollerTag>
void Poller<PollerTag>::add_handler(EventHandler* handler)
{
    assert(handlers_.find(handler->get_fd())==handlers_.end());
    handlers_[handler->get_fd()]=handler;
}

template<class PollerTag>
void Poller<PollerTag>::remove_handler(EventHandler* handler)
{
    assert(handlers_.find(handler->get_fd())!=handlers_.end());
#ifndef NDEBUG
    size_t n=handlers_.erase(handler->get_fd());
    assert(n==1);        
#else
    handlers_.erase(handler->get_fd());  
#endif
}

template<class PollerTag>
bool Poller<PollerTag>::has_handler(EventHandler* event_handler)
{
    auto it=handlers_.find(event_handler->get_fd());
    return it!=handlers_.end();
}





}    
}



#endif

 
