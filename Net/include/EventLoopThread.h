#ifndef _YY_NET_EVENTLOOPTHREAD_H_
#define _YY_NET_EVENTLOOPTHREAD_H_

#include "../Common/locker.h"
#include "../Common/noncopyable.h"
#include "EventLoop.h"
#include <exception>
#include <iostream>
namespace yy
{
namespace net
{
class EventLoopThread : public noncopyable 
{
public:
    typedef EventLoop::Functor Functor;

    EventLoopThread(int id=-1):
    id_(id), 
    loop_(nullptr) 
    {}

    ~EventLoopThread() 
    {
        assert(!thread_.joinable());
    }
    EventLoop* run() 
    {
        thread_.run([this]() mutable 
        {
            EventLoop loop(id_);          
            {
                std::lock_guard<std::mutex> lock(mutex_);
                loop_ = &loop;        
                cond_.notify_one();    
            }
            loop.loop();     
        });
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait(lock,[this] { return loop_ != nullptr; });
        }
        return loop_;
    }
    void stop() 
    {
        if (loop_ != nullptr) 
        {
            loop_->quit();  
        }
        thread_.join();
    }
    EventLoop* getEventLoop()
    {
        return loop_;
    }
    bool joinable()
    {
        return thread_.joinable();
    }
private:
    Thread thread_;
    int id_;
    EventLoop* loop_;            
    std::mutex mutex_;              
    std::condition_variable cond_;   
};


}    
}



#endif