#ifndef _YY_NET_EVENTLOOPTHREADPOOL_H_
#define _YY_NET_EVENTLOOPTHREADPOOL_H_
#include "EventLoopThread.h"
#include "../Common/noncopyable.h"
#include <vector>
#include <cstddef>
#include <assert.h>
#include <memory>
namespace yy
{
namespace net
{
class EventLoopThreadPool:public noncopyable
{
public:
    typedef EventLoopThread::Functor Functor;
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
    void addThread();
    //void caneclThread(); FIXME:取消线程比较麻烦
    void addHandler(EventHandlerPtr handler);

private:
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
};    
}    
}
#endif