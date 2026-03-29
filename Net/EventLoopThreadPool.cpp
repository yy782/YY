#include "EventLoopThreadPool.h"
namespace yy
{
namespace net
{
// void EventLoopThreadPool::addHandler(EventHandler* handler)
// {
//     static size_t nextId=0;
//     handler->init(fd_);
//     assert(nextId<threads_.size());
//     auto loop=threads_[nextId]->getEventLoop();
//     threads_[nextId]->getEventLoop()->addListen(handler);

//     // assert(handlers_.find(handler)==handlers_.end());
//     // handlers_[handler]=loop;

//     nextId=(nextId+1)%threads_.size();
// }
EventLoop* EventLoopThreadPool::getEventLoop()
{
    static std::atomic<size_t> nextId=0;// 注意多Acceptor模式nexId并不安全
    assert(nextId<threads_.size());
    auto loop=threads_[nextId]->getEventLoop();
    nextId=(nextId+1)%threads_.size();
    return loop;
}
// void EventLoopThreadPool::updateHandler(EventHandlerPtr handler)
// {
//     auto loop=handlers_[handler];
//     assert(loop);
//     loop->addListen(handler);
// }
// void EventLoopThreadPool::removeHandler(EventHandlerPtr handler)
// {
//     auto loop=handlers_[handler];
//     assert(loop);
//     loop->remove_listen(handler);
// }
}   
}