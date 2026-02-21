
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