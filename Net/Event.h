#ifndef _YY_NET_EVENT_
#define _YY_NET_EVENT_


#include <assert.h>
#include "../Common/copyable.h"

namespace yy
{
namespace net
{
enum EventType
{
    NoneEvent=0,
    ReadEvent=POLLIN,
    WriteEvent=POLLOUT,
    ExceptEvent=POLLPRI,
    // @brief 网络编程下，异常事件主要是TCP带外数据
    ErrorEvent=EPOLLERR,
    HupEvent=EPOLLHUP
};   
class Event:public copyable
{
public:
    Event():
    event_(EventType::NoneEvent)
    {}    

    int get_event()const{return event_;}

    void add_event(int event)
    {
        assert((event==EventType::ReadEvent||
            event==EventType::WriteEvent||
            event==EventType::NoneEvent||
            event==EventType::ExceptEvent)&&
            "事件不存在"
        );
        // @breif 挂起和错误事件是内核主动注册的，不用主动设置
        this->event_|=event;
    }
    void add_event(Event other)
    {
        this->event_|=other.event_;
    }
    void remove_event(int event)
    {
        assert((event==EventType::ReadEvent||
            event==EventType::WriteEvent||
            event==EventType::NoneEvent||
            event==EventType::ExceptEvent)&&
            "事件不存在"
        );        
        this->event_&=~event;
    }
    bool operator==(EventType event)const{return event_==event;}
    bool operator!=(EventType event)const{return !(*this==event);}
    bool operator&(EventType event)const{return event_&event;}
private:
    int event_;
};

 

}    
}

#endif