#ifndef _YY_NET_EVENT_
#define _YY_NET_EVENT_


#include <assert.h>
#include "../Common/copyable.h"

#include "Poller.h"
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
    typedef int Type_;
    static constexpr Type_ INVAID_EVENT=-1;
    static constexpr Type_ NoneEvent=0;
    static constexpr Type_ ReadEvent=
                    std::is_same_v<PollerType,Epoll>? static_cast<Type_>(EPOLLIN)// @note 我们选择水平触发模式，所以省略了边缘触发模式的完善
                    :std::is_same_v<PollerType,Poll>?static_cast<Type_>(POLLIN)
                    :std::is_same_v<PollerType,Select>?static_cast<Type_>(POLLIN)
                    :INVAID_EVENT;
    static constexpr int WriteEvent=
                    std::is_same_v<PollerType,Epoll>?static_cast<Type_>(EPOLLOUT)
                    :std::is_same_v<PollerType,Poll>?static_cast<Type_>(POLLOUT)
                    :std::is_same_v<PollerType,Select>?static_cast<Type_>(POLLOUT)
                    :INVAID_EVENT;
    static constexpr int ExceptEvent=std::is_same_v<PollerType,Epoll>?static_cast<Type_>(EPOLLPRI)
                    :std::is_same_v<PollerType,Poll>?static_cast<Type_>(POLLPRI)
                    :std::is_same_v<PollerType,Select>?static_cast<Type_>(POLLPRI)
                    :INVAID_EVENT;
    static constexpr int ErrorEvent=std::is_same_v<PollerType,Epoll>?static_cast<Type_>(EPOLLERR)
                    :std::is_same_v<PollerType,Poll>?static_cast<Type_>(POLLERR)
                    :std::is_same_v<PollerType,Select>?static_cast<Type_>(POLLERR)
                    :INVAID_EVENT;
    static constexpr int HupEvent=std::is_same_v<PollerType,Epoll>?static_cast<Type_>(EPOLLHUP)
                    :std::is_same_v<PollerType,Poll>?static_cast<Type_>(POLLHUP)
                    :std::is_same_v<PollerType,Select>?static_cast<Type_>(POLLHUP)
                    :INVAID_EVENT;
    static constexpr int NvalEvent=std::is_same_v<PollerType,Epoll>?static_cast<Type_>(EPOLLNVAL)
                    :std::is_same_v<PollerType,Poll>?static_cast<Type_>(POLLNVAL)
                    :std::is_same_v<PollerType,Select>?static_cast<Type_>(POLLNVAL)
                    :INVAID_EVENT;
    static constexpr int RdHupEvent=std::is_same_v<PollerType,Epoll>?static_cast<Type_>(EPOLLRDHUP)
                    :std::is_same_v<PollerType,Poll>?static_cast<Type_>(POLLRDHUP)
                    :std::is_same_v<PollerType,Select>?static_cast<Type_>(POLLRDHUP)
                    :INVAID_EVENT;



    static constexpr int OnlyEvent=EPOLLONESHOT;      
                        // @brief OnlyEvent是epoll才有的类型 
    static constexpr int kAllValidEvents=
        NoneEvent|ReadEvent|WriteEvent|ExceptEvent|ErrorEvent|HupEvent|NvalEvent|RdHupEvent;                         
    EventType(int event):
    event_(event)
    {}
    int get_event()const{return event_;}
private:         
    int event_;                    
};
class Event:public copyable
{
public:
    Event():
    event_(EventType::NoneEvent)
    {}    
    explicit Event(int event):
    event_(event)
    {
        assert(((event&~EventType::kAllValidEvents)==0) && "事件包含非法位" );
    }
    int get_event()const{return event_;}

    void add_event(int event)
    {
        assert(((event&~EventType::kAllValidEvents)==0) && "事件包含非法位" );
        // @breif 挂起和错误事件是内核主动注册的，不用主动设置
        this->event_|=event;
    }
    void add_event(Event other)
    {
        this->event_|=other.event_;
    }
    void remove_event(int event)
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
private:

    int event_;
};

 

}    
}

#endif