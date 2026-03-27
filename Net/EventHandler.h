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
    EventHandler()=default;
    EventHandler(int fd,EventLoop* loop,const  char* name);
    ~EventHandler()=default;
    void init(int fd,EventLoop* loop,const char* name);
    void tie(const std::shared_ptr<void>&);

    
    int fd() const noexcept{return fd_;}
    EventLoop* loop() const noexcept{return loop_;}
    Event event() const noexcept{return events_;}
    int status() const noexcept{return status_;}
    const std::string& printName()
    {
        if(name_.empty())
        {
            static const std::string NoName="NoName";
            return NoName;
        }
        return name_;
    }



    void set_event(Event event)
    {
        events_=event;
        update();
    }
    void set_revent(Event event) noexcept{revents_=event;}
    void set_status(int status) noexcept{status_=status;}

    bool isWriting()const noexcept{return events_.has(LogicEvent::Write);}
    bool isReading()const noexcept{return events_.has(LogicEvent::Read);}
    bool isExcept()const noexcept{return events_.has(LogicEvent::Except);}
    bool isReadingAndExcept()const noexcept{return events_&(LogicEvent::Read|LogicEvent::Except);}

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
    void disableAll()
    {
        events_=LogicEvent::None;
        update();
    }

    template<typename Callable>
    void setReadCallBack(Callable&& cb) { 
        readCallback_ = std::forward<Callable>(cb);
    }

    template<typename Callable>
    void setWriteCallBack(Callable&& cb) { 
        writeCallback_ = std::forward<Callable>(cb);
    }

    template<typename Callable>
    void setCloseCallBack(Callable&& cb) { 
        closeCallback_ = std::forward<Callable>(cb);
    }

    template<typename Callable>
    void setErrorCallBack(Callable&& cb) { 
        errorCallback_ = std::forward<Callable>(cb);
    }

    template<typename Callable>
    void setExceptCallBack(Callable&& cb) { 
        exceptCallback_ = std::forward<Callable>(cb);
    }



    void handler_revent();
    void removeListen();



private:
    void update();
    int status_={-1};// @brief 这个状态是关联事件监听器Poller的状态，含义由事件监听器解释
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