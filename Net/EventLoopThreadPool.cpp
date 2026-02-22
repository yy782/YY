#include "EventLoopThreadPool.h"
namespace yy
{
namespace net
{
void EventLoopThreadPool::addThread()
{
#ifdef NDEBUG        
    threads_.emplace_back();   
    threads_[threads_.size()-1]->run();
#else
    size_t n=threads_.size();
    threads_.emplace_back();
    assert(n+1==threads_.size());  
    threads_[threads_.size()-1]->run();
#endif
}
void EventLoopThreadPool::submit(Functor cb)
{
    static size_t id=0;
    assert(id<threads_.size());
    auto loop=threads_[id]->getEventLoop();
    assert(loop);
    loop->submit(std::move(cb));
    id=(id+1)%threads_.size();
}
   
}   
}