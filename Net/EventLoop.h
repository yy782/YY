#ifndef _YY_NET_EVENTLOOP_H_
#define _YY_NET_EVENTLOOP_H_
#include <vector>
#include "Poller.h"
#include "EventHandler.h"
#include "sockets.h"
#include "../Common/noncopyable.h"
#include "../Common/TimeStamp.h"
namespace yy
{
namespace net
{



class EventLoop:public noncopyable
{
public:
    typedef PollerType::HandlerList HandlerList;
    typedef std::function<void()> Functor;
    typedef std::vector<Functor> FunctionList;
    EventLoop();
    ~EventLoop()=default;
    void loop();
    void quit();

    void submit(Functor cb);

    // @note 保证线程安全，这些操作Poller的接口只能在固定线程使用，其他线程必须用submit进行提交
    void addListen(EventHandler* handler)
    {
        assert(handler);
        poller_.add_listen(*handler);
    }
    void update_listen(EventHandler* handler)
    { 
        assert(handler);    
        poller_.update_listen(*handler);
    }
    void remove_listen(EventHandler* handler)
    {
        assert(handler);
        poller_.remove_listen(*handler);
    }    
private:
    void wakeup();

    void doPendingFunctions();

    static bool CheckeEventLoopStatus();
    
    PollerType poller_;
    HandlerList activeHandlers_;
    FunctionList FunctionList_;
    int status_;
    EventHandler wakeupHandler_;
    TimeStamp<LowPrecision> pollReturnTime_;
    int64_t iteration_;//记录事件循环的迭代次数
};
}    
}
#endif // _YY_NET_EVENTLOOP_H_