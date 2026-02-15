#ifndef _YY_NET_EVENTHANDLER_
#define _YY_NET_EVENTHANDLER_


#include "Socket.h"
#include "Event.h"
#include "../Common/noncopyable.h"

namespace yy
{
namespace net
{
class EventHandler:public noncopyable
{
public:
    EventHandler()=delete;
    Socket get_socket()const{return k_;}
    int get_fd()const{return k_.get_fd();}
    Event get_event()const{return events_;}
    void set_revent(Event event){revents_.add_event(event);}

    bool isWriting()const{return events_&EventType::WriteEvent;}
    bool isReading()const{return events_&EventType::ReadEvent;}
    bool isExcept()const{return events_&EventType::ExceptEvent;}
private:
    const Socket k_;
    Event events_;
    Event revents_;
    // @brief events_是要监听的事件，revents_是触发的事件
};
}    
}

#endif