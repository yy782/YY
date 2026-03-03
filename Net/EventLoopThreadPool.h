#ifndef _YY_NET_EVENTLOOPTHREADPOOL_H_
#define _YY_NET_EVENTLOOPTHREADPOOL_H_
#include "EventLoopThread.h"
#include "../Common/noncopyable.h"
#include <vector>
#include <cstddef>
#include <assert.h>
#include <memory>
#include <unordered_map>
namespace yy
{
namespace net
{
class EventLoopThreadPool:public noncopyable
{
public:
    //typedef std::unordered_map<EventHandlerPtr,EventLoop*> EventHandlerMap;
    EventLoopThreadPool(int num)
    {
        threads_.reserve(num);
        for(int i=0;i<num;++i)
        {
            threads_.emplace_back(std::make_unique<EventLoopThread>());
        }
    }
    void run()
    {
        for(auto it=threads_.begin();it!=threads_.end();++it)
        {
            (*it)->run();
        }
    }
    void stop()
    {
        for(auto it=threads_.begin();it!=threads_.end();++it)
        {
            (*it)->stop();
        } 
    }
    void addHandler(EventHandler* handler);
    // void removeHandler(EventHandlerPtr handler);
    // void updateHandler(EventHandlerPtr handler);
private:
    std::vector<std::unique_ptr<EventLoopThread>> threads_; //vector要求移动构造，用指针存储，避免填写移动构造的麻烦
    //EventHandlerMap handlers_;
};    
}    
}
#endif