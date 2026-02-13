#ifndef _YY_NET_EVENTHANDLER_
#define _YY_NET_EVENTHANDLER_


#include "Socket.h"
#include "../Common/noncopyable.h"
namespace yy
{
namespace net
{
class EventHandler:public noncopyable
{
public:
    EventHandler()=delete;
    Socket get_socket(){return k_;}
    int get_fd(){return k_.get_fd();}
private:
    const Socket k_;
    int events_;
};
}    
}

#endif