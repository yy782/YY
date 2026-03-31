#include "EventLoopThreadPool.h"
namespace yy
{
namespace net
{
EventLoop* EventLoopThreadPool::getEventLoop()
{
    static std::atomic<size_t> nextId=0;// 注意多Acceptor模式nexId并不安全
    assert(nextId<threads_.size());
    auto loop=threads_[nextId]->getEventLoop();
    nextId=(nextId+1)%threads_.size();
    return loop;
}
}   
}