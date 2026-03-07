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
    RingBuffer(size_t capacity):queue_(capacity)
    {}
    bool append(const T& t)
    {
        return queue_.push(t);
    }
    bool append(T&& t)
    {
        return queue_.push(std::move(t));
    }
    bool retrieve(T& t)
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
    size_t getRemainCapacity()
    {
        return queue_.getRemainCapacity();
    }  
private:
    LockFreeCurcularQueue<T> queue_;    
};

}


#endif