#ifndef _YY_NET_SIGNALHANDLER_H_
#define _YY_NET_SIGNALHANDLER_H_

#include "../Common/noncopyable.h"
#include <functional>
#include <sys/signalfd.h>
#include <map>
#include <memory>
#include <signal.h>
#include "EventHandler.h"
namespace yy
{
namespace net
{


class EventLoop;
class SignalHandler:public noncopyable
{
public:
   typedef std::function<void()> SigCallBack;
   static SignalHandler&  getInstance(EventLoop* loop)
    {
        static SignalHandler instance(loop);
        return instance;
    }
   void addSign(int sig,SigCallBack cb); 
private:
    SignalHandler(EventLoop* loop);
    void handle();
    sigset_t sigset_;
    EventHandler handler_;
    std::map<int,SigCallBack> callbacks_;
};

}    
}

#endif