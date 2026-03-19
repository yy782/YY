#ifndef _YY_NET_EVENTLOOPTHREAD_H_
#define _YY_NET_EVENTLOOPTHREAD_H_

#include "../Common/locker.h"
#include "../Common/noncopyable.h"
#include "EventLoop.h"
namespace yy
{
namespace net
{
//thread_local EventLoop safe_loop;
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
//         thread_.join();
//         if(!loop_.isQuit())
//         {
//             loop_.quit();
//         }
//     }
//     void run()
//     {
//         //assert(loop_);
//         thread_.run([this]()mutable
//         {
//             loop_.setPid_t(thread_.getId());
//             loop_.loop();
//         });
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

class EventLoopThread : public noncopyable {
public:
    typedef EventLoop::Functor Functor;

    EventLoopThread() : loop_(nullptr) {}

    ~EventLoopThread() {
        if(loop_ != nullptr) 
        {
            loop_->quit();     
        }
        thread_.join();          
    }
    EventLoop* run() {
        thread_.run([this]() mutable {
            EventLoop loop;           // ✅ 在子线程中创建 loop
            loop.setPid_t(thread_.getId());

            {
                std::lock_guard<std::mutex> lock(mutex_);
                loop_ = &loop;         // 将地址传给主线程
                cond_.notify_one();    // 通知主线程 loop 已创建
            }
            loop.loop();               
            {
                std::lock_guard<std::mutex> lock(mutex_);
                loop_ = nullptr;        
            }
        });
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait(lock, [this] { return loop_ != nullptr; });
        }

        return loop_;  // 返回子线程创建的 loop 指针
    }

    void stop() {
        if (loop_ != nullptr) {
            loop_->quit();  // 通知退出
        }
        thread_.join();     // 等待线程结束
    }

private:
    Thread thread_;
    EventLoop* loop_;                // 改为指针，指向子线程的 loop
    std::mutex mutex_;               // 保护 loop_ 的互斥锁
    std::condition_variable cond_;    // 用于同步
};


}    
}



#endif