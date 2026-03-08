#ifndef _YY_NET_EVENTHANDLER_
#define _YY_NET_EVENTHANDLER_

#include <functional>

#include "Event.h"
#include "../Common/noncopyable.h"
#include "../Common/TimeStamp.h"
#include <memory>
#include <string>
#include "sockets.h"
namespace yy
{
namespace net
{

class EventLoop;


class EventHandler:public noncopyable
{
public:
    typedef TimeStamp<LowPrecision> Time_Stamp;
    typedef std::function<void(Time_Stamp)> TimeStampEventCallBack;
    typedef std::function<void()> EventCallBack;

    typedef EventCallBack ReadCallBack;
    typedef EventCallBack WriteCallBack;
    typedef EventCallBack CloseCallBack;

    EventHandler():
    status_(-1)
    {}
    ~EventHandler()
    {
        sockets::close(fd_);
    }
    EventHandler(int fd,EventLoop* loop);
    int get_fd()const{return fd_;}
    EventLoop* get_loop()const{return loop_;}
    Event get_event()const{return events_;}
    int get_status()const{return status_;}
    bool has_ignore()const{return events_==EventType::NoneEvent;}
    void set_ignore()
    {
        assert(events_!=EventType::NoneEvent);
        events_=EventType::NoneEvent;
    }
    void set_fd(int fd){fd_=fd;}
    void set_event(Event event){events_.add_event(event);}
    void set_loop(EventLoop* loop){loop_=loop;}
    void set_revent(Event event){revents_.add_event(event);}
    void set_status(int status){status_=status;}
    bool isWriting()const{return events_&EventType::WriteEvent;}
    bool isReading()const{return events_&EventType::ReadEvent;}
    bool isExcept()const{return events_&EventType::ExceptEvent;}
    bool isReadingAndExcept()const{return events_&(EventType::ReadEvent|EventType::ExceptEvent);}
    void setReading(){events_.add_event(EventType::ReadEvent);}
    void cancelReading(){events_.remove_event(EventType::ReadEvent);}
    void setWriting(){events_.add_event(EventType::WriteEvent);}
    void caneclWriting(){events_.remove_event(EventType::WriteEvent);}
    void setExcept(){events_.add_event(EventType::ExceptEvent);}
    void cancelExcept(){events_.remove_event(EventType::ExceptEvent);}
    void setReadingAndExcept()
    {
        setExcept();
        setReading();
    }
    void cancelReadingAndExcept()
    {
        cancelExcept();
        cancelReading();
    }

    void setReadCallBack(EventCallBack cb){readCallback_=std::move(cb);}
    void setWriteCallBack(EventCallBack cb){writeCallback_=std::move(cb);}
    void setCloseCallBack(EventCallBack cb){closeCallback_=std::move(cb);}
    void setErrorCallBack(EventCallBack cb){errorCallback_=std::move(cb);}

    void set_name(const std::string& name){name_=name;}
    const std::string& printName()
    {
        if(name_.empty())
        {
            static const std::string NoName="NoName";
            return NoName;
        }
        return name_;
    }

    void handler_revent();
    void disableAll()
    {
        events_=EventType::NoneEvent;
    }
    void removeListen();

private:
    
    int status_;// @brief 这个状态是关联事件监听器Poller的状态，含义由事件监听器解释
    int fd_;
    Event events_;
    Event revents_;
    EventLoop* loop_;
    std::string name_;
    // @brief events_是要监听的事件，revents_是触发的事件

    ReadCallBack readCallback_;
    WriteCallBack writeCallback_;
    EventCallBack errorCallback_;
    
    CloseCallBack closeCallback_;
};
}    
}

#endif