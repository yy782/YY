#ifndef _YY_LOCKFREECURCULARQUEUE_H_
#define _YY_LOCKFREECURCULARQUEUE_H_
#include <atomic>
#include <cassert>
#include <cstddef>
#include <utility>
#include <vector>
#include "noncopyable.h"
namespace yy
{

template <typename T> 
class LockFreeCurcularQueue:noncopyable
{
public:
    explicit LockFreeCurcularQueue(size_t capacity);
    ~LockFreeCurcularQueue()=default;
    LockFreeCurcularQueue(LockFreeCurcularQueue&&)=delete;
    LockFreeCurcularQueue& operator=(LockFreeCurcularQueue&&)=delete;

    bool enqueue(const T& data);
    bool enqueue(T&& data);
    bool dequeue(T& data);

    bool empty()const;
    size_t size()const;
private:
    struct Slot
    {
        std::atomic<size_t> sequence;
        T data;
    };

    alignas(64) std::atomic<size_t> enqueuePos_; 
    alignas(64) std::atomic<size_t> dequeuePos_; 

    std::vector<Slot> buffer_; 
    size_t bufferMask_;        

    static size_t roundUpToPowerOf2(size_t n);
    static constexpr size_t kCacheLineSize=64;

    template <typename U> 
    bool enqueueImpl(U &&data);    
};

template <typename T>  
size_t LockFreeCurcularQueue<T>::roundUpToPowerOf2(size_t n)
{
    assert(n>0);
    --n;
    n|=n>>1;
    n|=n>>2;
    n|=n>>4;
    n|=n>>8;
    n|=n>>16;
    n|=n>>32;
    return n+1;
}

template <typename T>
LockFreeCurcularQueue<T>::LockFreeCurcularQueue(size_t capacity): 
    buffer_(roundUpToPowerOf2(capacity)),
    bufferMask_(buffer_.size()-1), 
    enqueuePos_(0), 
    dequeuePos_(0)
{
    for(size_t i=0;i<buffer_.size();++i)
    {
        buffer_[i].sequence.store(i,std::memory_order_relaxed);
    }
}

template <typename T> 
template<typename U>  
bool LockFreeCurcularQueue<T>::enqueueImpl(U&& data)
{
    Slot *slot;
    size_t pos=enqueuePos_.load(std::memory_order_relaxed);

    while(true)
    {
        slot=&buffer_[pos & bufferMask_];
        size_t seq=slot->sequence.load(std::memory_order_acquire);
        intptr_t dif=static_cast<intptr_t>(seq)-static_cast<intptr_t>(pos);

        if(dif==0)
        {
            if(enqueuePos_.compare_exchange_weak(pos,pos + 1,std::memory_order_relaxed))
            {
                break;
            }
        }
        else if(dif<0)
        {
            return false; 
        }
        else
        {
            pos=enqueuePos_.load(std::memory_order_relaxed);
        }
    }
    slot->data=std::forward<U>(data);
    slot->sequence.store(pos+1,std::memory_order_release);
    return true;
}

template <typename T> 
bool LockFreeCurcularQueue<T>::enqueue(const T& data)
{
    return enqueueImpl(data);
}

template <typename T> 
bool LockFreeCurcularQueue<T>::enqueue(T&& data)
{
    return enqueueImpl(std::forward<T>(data));
}

template <typename T> 
bool LockFreeCurcularQueue<T>::dequeue(T& data)
{
    Slot *slot;
    size_t pos=dequeuePos_.load(std::memory_order_relaxed);

    while(true)
    {
        slot=&buffer_[pos&bufferMask_];
        size_t seq=slot->sequence.load(std::memory_order_acquire);
        intptr_t dif=static_cast<intptr_t>(seq)-static_cast<intptr_t>(pos + 1);

        if(dif==0)
        {
            if(dequeuePos_.compare_exchange_weak(pos,pos + 1,std::memory_order_relaxed))
            {
                break;
            }
        }
        else if(dif<0)
        {
            return false;
        }
        else
        {
            pos=dequeuePos_.load(std::memory_order_relaxed);
        }
    }

    data=std::move(slot->data);
    slot->sequence.store(pos+buffer_.size(),std::memory_order_release);
    return true;
}

template <typename T> 
bool LockFreeCurcularQueue<T>::empty()const
{
    size_t head=dequeuePos_.load(std::memory_order_relaxed);
    size_t tail=enqueuePos_.load(std::memory_order_relaxed);
    return head==tail;
}

template <typename T> 
size_t LockFreeCurcularQueue<T>::size()const
{
    size_t head=dequeuePos_.load(std::memory_order_relaxed);
    size_t tail=enqueuePos_.load(std::memory_order_relaxed);
    return tail-head;
}
}
#endif