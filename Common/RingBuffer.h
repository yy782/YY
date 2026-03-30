#ifndef _YY_RINGBUFFER_H_
#define _YY_RINGBUFFER_H_ 
#include "noncopyable.h"
#include "LockFreeCurcularQueue.h"
#include <thread>
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
        return appendImpl(t);
    }
    bool append(T&& t)
    {
        return appendImpl(std::move(t));
    }
    bool retrieve(T& t)
    {
        return queue_.dequeue(t);
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
    template<typename U>
    bool appendImpl(U&& t)
    {
        return queue_.enqueue(std::forward<U>(t));
    }
    
    LockFreeCurcularQueue<T> queue_;    
};

}


#endif