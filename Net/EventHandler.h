#ifndef _YY_NET_EVENTHANDLER_
#define _YY_NET_EVENTHANDLER_

#include <functional>

#include "Event.h"
#include "EventLoop.h"
#include "../Common/noncopyable.h"
#include "../Common/TimeStamp.h"
namespace yy
{
namespace net
{

class EventHandler:public noncopyable
{
public:
    typedef std::function<void(TimeStamp<LowPrecision>)> TimeStampEventCallBack;
    typedef std::function<void()> EventCallBack;

    typedef TimeStampEventCallBack ReadCallBack;
    EventHandler()=delete;
    explicit EventHandler(int fd,EventLoop* loop):
    status_(-1),
    fd_(fd),
    events_(EventType::NoneEvent),
    revents_(EventType::NoneEvent),
    loop_(loop)
    {
        assert(loop_!=nullptr);
    }
    int get_fd()const{return fd_;}
    Event get_event()const{return events_;}
    int get_status()const{return status_;}
    bool has_ignore()const{return events_==EventType::NoneEvent;}
    void set_ignore()
    {
        assert(events_!=EventType::NoneEvent);
        events_=EventType::NoneEvent;
    }
    void set_revent(Event event){revents_.add_event(event);}
    void set_status(int status){status_=status;}
    bool isWriting()const{return events_&EventType::WriteEvent;}
    bool isReading()const{return events_&EventType::ReadEvent;}
    bool isExcept()const{return events_&EventType::ExceptEvent;}
    void setReading(){events_.add_event(EventType::ReadEvent);}
    void setWriting(){events_.add_event(EventType::WriteEvent);}
    void setExcept(){events_.add_event(EventType::ExceptEvent);}

    void setReadCallBack(TimeStampEventCallBack cb){readCallback_=std::move(cb);}
    void setWriteCallBack(EventCallBack cb){writeCallback_=std::move(cb);}

    void handler_revent(){};
private:
    void update();
    int status_;// @brief 这个状态是关联事件监听器Poller的状态，含义由事件监听器解释
    int fd_;
    Event events_;
    Event revents_;
    EventLoop* loop_;
    // @brief events_是要监听的事件，revents_是触发的事件

    ReadCallBack readCallback_;
    EventCallBack writeCallback_;
    EventCallBack exceptCallback_;
    EventCallBack hupCallback_;
    EventCallBack errorCallback_;
};
}    
}

#endif