#ifndef _YY_NET_EVENT_
#define _YY_NET_EVENT_

#include <cstdint>
#include <assert.h>
#include "../Common/copyable.h"

#include "PollerType.h"
#include <sys/poll.h>
#include <sys/epoll.h>
#include <type_traits>

#define EPOLLNVAL 0x020
                        // @FIMXE 这可能不太好，但是EPOLLNVAL不向外面暴露?

namespace yy
{
namespace net
{


struct EventType
{
    typedef uint32_t Type_;
    static constexpr Type_ INVAID_EVENT=-1;
    static constexpr Type_ NoneEvent=0;
    static constexpr Type_ ReadEvent=
                    std::is_same_v<PollerType,Epoll>? static_cast<Type_>(EPOLLIN)
                    :std::is_same_v<PollerType,Poll>?static_cast<Type_>(POLLIN)
                    :std::is_same_v<PollerType,Select>?static_cast<Type_>(POLLIN)
                    :INVAID_EVENT;
    static constexpr Type_ WriteEvent=
                    std::is_same_v<PollerType,Epoll>?static_cast<Type_>(EPOLLOUT)
                    :std::is_same_v<PollerType,Poll>?static_cast<Type_>(POLLOUT)
                    :std::is_same_v<PollerType,Select>?static_cast<Type_>(POLLOUT)
                    :INVAID_EVENT;
    static constexpr Type_ ExceptEvent=std::is_same_v<PollerType,Epoll>?static_cast<Type_>(EPOLLPRI)
                    :std::is_same_v<PollerType,Poll>?static_cast<Type_>(POLLPRI)
                    :std::is_same_v<PollerType,Select>?static_cast<Type_>(POLLPRI)
                    :INVAID_EVENT;
    static constexpr Type_ ErrorEvent=std::is_same_v<PollerType,Epoll>?static_cast<Type_>(EPOLLERR)
                    :std::is_same_v<PollerType,Poll>?static_cast<Type_>(POLLERR)
                    :std::is_same_v<PollerType,Select>?static_cast<Type_>(POLLERR)
                    :INVAID_EVENT;
    static constexpr Type_ HupEvent=std::is_same_v<PollerType,Epoll>?static_cast<Type_>(EPOLLHUP)
                    :std::is_same_v<PollerType,Poll>?static_cast<Type_>(POLLHUP)
                    :std::is_same_v<PollerType,Select>?static_cast<Type_>(POLLHUP)
                    :INVAID_EVENT;
    static constexpr Type_ NvalEvent=std::is_same_v<PollerType,Epoll>?static_cast<Type_>(EPOLLNVAL)
                    :std::is_same_v<PollerType,Poll>?static_cast<Type_>(POLLNVAL)
                    :std::is_same_v<PollerType,Select>?static_cast<Type_>(POLLNVAL)
                    :INVAID_EVENT;
    static constexpr Type_ RdHupEvent=std::is_same_v<PollerType,Epoll>?static_cast<Type_>(EPOLLRDHUP)
                    :std::is_same_v<PollerType,Poll>?static_cast<Type_>(POLLRDHUP)
                    :std::is_same_v<PollerType,Select>?static_cast<Type_>(POLLRDHUP)
                    :INVAID_EVENT;


    static constexpr Type_ EV_ET=EPOLLET;
    static constexpr Type_ OnlyEvent=EPOLLONESHOT;      
                        // @brief OnlyEvent是epoll才有的类型 
    static constexpr Type_ kAllValidEvents=
        NoneEvent|ReadEvent|WriteEvent|ExceptEvent|ErrorEvent|HupEvent|NvalEvent|RdHupEvent|EV_ET;                         
    EventType(Type_ event):
    event_(event)
    {}
    Type_ get_event()const{return event_;}
private:         
    Type_ event_;                    
};
class Event:public copyable
{
public:
    Event():
    event_(EventType::NoneEvent)
    {}    
    Event(uint32_t event):
    event_(event)
    {
        assert(((event&~EventType::kAllValidEvents)==0) && "事件包含非法位" );
    }
    uint32_t get_event()const{return event_;}

    void add_event(uint32_t event)
    {
        assert(((event&~EventType::kAllValidEvents)==0) && "事件包含非法位" );
        // @breif 挂起和错误事件是内核主动注册的，不用主动设置
        this->event_|=event;
    }
    void add_event(Event other)
    {
        this->event_|=other.event_;
    }
    void remove_event(uint32_t event)
    {
        assert(((event&~EventType::kAllValidEvents)==0) && "事件包含非法位" );        
        this->event_&=~event;
    }
    bool operator==(EventType event)const{return event_==event.get_event();}
    bool operator!=(EventType event)const{return !(*this==event);}
    bool operator&(EventType event)const{return event_&event.get_event();}
    Event& operator=(EventType event)
    {
        event_=event.get_event();
        return *this;
    }
    Event& operator=(uint32_t events) {
        event_ = events;
        return *this;
    }
private:

    uint32_t event_;
};

 

}    
}

#endif