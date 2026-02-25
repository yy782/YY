#include "EventLoopThreadPool.h"
namespace yy
{
namespace net
{
void EventLoopThreadPool::addThread()
{
#ifdef NDEBUG        
    threads_.emplace_back(std::make_unique<EventLoopThread>());   
    threads_[threads_.size()-1]->run();
#else
    size_t n=threads_.size();
    threads_.emplace_back(std::make_unique<EventLoopThread>());
    assert(n+1==threads_.size());  
    threads_[threads_.size()-1]->run();
#endif
}
void EventLoopThreadPool::addHandler(EventHandlerPtr handler)
{
    static size_t nextId=0;
    assert(nextId<threads_.size());
    auto loop=threads_[nextId]->getEventLoop();
    handler->set_loop(loop);
    threads_[nextId]->sumbit(std::bind(&EventLoop::addListen,loop,handler));
    nextId=(nextId+1)%threads_.size();
}
   
}   
}