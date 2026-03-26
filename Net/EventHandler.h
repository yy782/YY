#ifndef YY_NET_EVENTHANDLER_
#define YY_NET_EVENTHANDLER_

#include <functional>
#include <memory>
#include <string>

#include "Event.h"
#include "../Common/noncopyable.h"
#include "../Common/TimeStamp.h"
#include "sockets.h"

namespace yy
{
namespace net
{

class EventLoop;


class EventHandler:public noncopyable
{
public:
    typedef std::function<void()> EventCallBack;

    typedef EventCallBack ReadCallBack;
    typedef EventCallBack WriteCallBack;
    typedef EventCallBack CloseCallBack;
    typedef EventCallBack ErrorCallBack;
    typedef EventCallBack ExceptCallBack;
    EventHandler():
    status_(-1)
    {}
    ~EventHandler()
    {
     
    }
    void tie(const std::shared_ptr<void>&);
    EventHandler(int fd,EventLoop* loop,const  char* name);
    void init(int fd,EventLoop* loop,const char* name);
    int get_fd()const{return fd_;}
    EventLoop* get_loop()const{return loop_;}
    Event get_event()const{return events_;}
    int get_status()const{return status_;}


    void set_event(LogicEvent event){
        events_=event;
        update();
    }
    void set_event(Event event){
        events_=event;
        update();
    }
    
    void set_revent(LogicEvent event)
    {
        revents_=event;
    }
    void set_revent(uint32_t event)
    {
        revents_=event;
    }
    void set_status(int status){status_=status;}
    bool isWriting()const{return events_.has(LogicEvent::Write);}
    bool isReading()const{return events_.has(LogicEvent::Read);}
    bool isExcept()const{return events_.has(LogicEvent::Except);}
    bool isReadingAndExcept()const{return events_&(LogicEvent::Read|LogicEvent::Except);}
    void setReading(){
        events_.add(LogicEvent::Read);update();
    }
    void cancelReading(){events_.remove(LogicEvent::Read);update();}
    void setWriting(){events_.add(LogicEvent::Write);update();}
    void cancelWriting(){events_.remove(LogicEvent::Write);update();}
    void setExcept(){events_.add(LogicEvent::Except);update();}
    void cancelExcept(){events_.remove(LogicEvent::Except);update();}
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
    void setExceptCallBack(EventCallBack cb){exceptCallback_=std::move(cb);}

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
        events_=LogicEvent::None;
        update();
    }
    void removeListen();



private:
    void update();
    int status_;// @brief 这个状态是关联事件监听器Poller的状态，含义由事件监听器解释
    std::weak_ptr<void> tie_;
    bool tied_={false};
    int fd_={-1};
    Event events_;
    Event revents_;
    EventLoop* loop_;
    std::string name_;
    // @brief events_是要监听的事件，revents_是触发的事件

    ReadCallBack readCallback_;
    WriteCallBack writeCallback_;
    ErrorCallBack errorCallback_;
    ExceptCallBack exceptCallback_;

    CloseCallBack closeCallback_;
};
}    
}

#endif