#ifndef _YY_RINGBUFFER_H_
#define _YY_RINGBUFFER_H_ 
#include "noncopyable.h"
#include "LockFreeCurcularQueue.h"
namespace yy 
{ 
template<typename T>
class RingBuffer:noncopyable
{ 

public:
    RingBuffer(size_t size):queue_(size)
    {}
    bool push(const T& t)
    {
        return queue_.push(t);
    }
    bool push(T&& t)
    {
        return queue_.push(std::move(t));
    }
    bool pop(T& t)
    {
        return queue_.pop(t);
    }
    bool empty()
    {
        return queue_.empty();
    }
    bool full()
    {
        return queue_.full();
    }    
private:
    LockFreeCurcularQueue<T> queue_;    
};

}


#endif