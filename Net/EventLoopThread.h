#ifndef _YY_NET_EVENTLOOPTHREAD_H_
#define _YY_NET_EVENTLOOPTHREAD_H_

#include "../Common/locker.h"
#include "../Common/noncopyable.h"
#include "EventLoop.h"
namespace yy
{
namespace net
{
class EventLoopThread : public noncopyable {
public:
    typedef EventLoop::Functor Functor;

    EventLoopThread() : loop_(nullptr) {}

    ~EventLoopThread() {
        assert(!thread_.joinable());
    }
    EventLoop* run() 
    {
        thread_.run([this]() mutable 
        {
            EventLoop loop;          
            {
                std::lock_guard<std::mutex> lock(mutex_);
                loop_ = &loop;        
                cond_.notify_one();    
            }
            loop.loop();               
        });
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait(lock, [this] { return loop_ != nullptr; });
        }
        return loop_;
    }
    void stop() {
        if (loop_ != nullptr) {
            loop_->quit();  
        }
        thread_.join();
    }
    EventLoop* getEventLoop()
    {
        return loop_;
    }
    void join()
    {
        thread_.join();
    }
private:
    Thread thread_;
    EventLoop* loop_;            
    std::mutex mutex_;              
    std::condition_variable cond_;   
};
// class EventLoopThread:public  noncopyable
// {
// public:
//     typedef EventLoop::Functor Functor;
//     EventLoopThread():
//     loop_()
//     {
        
//     }
//     ~EventLoopThread()
//     {
//         if(!loop_.isQuit())
//         {
//             loop_.quit();
//         }
//         if(thread_.joinable())
//         {
//             thread_.join();
//         }   
//     }
//     EventLoop* run()
//     {
//         //assert(loop_);
//         thread_.run([this]()mutable
//         {
//             loop_.setPid_t(thread_.getId());
//             loop_.loop();
//         });
//         return &loop_;
//     }
//     void stop()
//     {
//         //assert(loop_);
//         loop_.quit();
//         thread_.join();
//     }
//     EventLoop* getEventLoop(){return &loop_;}
// private:
//     Thread thread_;
//     EventLoop loop_;
// };

}    
}



#endif