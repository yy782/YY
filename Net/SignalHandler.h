#ifndef _YY_NET_SIGNALHANDLER_H_
#define _YY_NET_SIGNALHANDLER_H_

#include "../Common/noncopyable.h"
#include <functional>
#include <sys/signalfd.h>
#include <map>
#include <memory>
#include <signal.h>
#include "EventHandler.h"
#include "sockets.h"
#include "../Common/LogFilter.h"
#include <signal.h>
#include "EventLoop.h"
namespace yy
{
namespace net
{


class EventLoop;
// class SignalHandler:public noncopyable
// {
// public:
//    typedef std::function<void()> SigCallBack;
//    static SignalHandler&  getInstance(EventLoop* loop)
//     {
//         static SignalHandler instance(loop);
//         return instance;
//     }
//    void addSign(int sig,SigCallBack cb); 
// private:
//     SignalHandler(EventLoop* loop);
//     void handle();
//     sigset_t sigset_;
//     EventHandler handler_;
//     std::map<int,SigCallBack> callbacks_;
// };
// class SignalHandler :public noncopyable
// {
// public:
//     typedef std::function<void()> SigCallBack;
//     SignalHandler(EventLoop* loop) : 
//     loop_(loop),
//     handler_(std::make_unique<EventHandler>())
//     {
//         sigemptyset(&signal_mask_);
        
//     }
//    static SignalHandler&  getInstance(EventLoop* loop)
//     {
//         static SignalHandler instance(loop);
//         return instance;
//     }    
//     ~SignalHandler() 
//     {
//     }
    
//     void addSign(int sig, SigCallBack cb) 
//     {
        
//         callbacks_[sig] = std::move(cb);
//         sigaddset(&signal_mask_, sig);
//         sigprocmask(SIG_BLOCK, &signal_mask_, nullptr);
//         int signal_fd =sockets::setSignalOrDie(-1, &signal_mask_, SFD_NONBLOCK | SFD_CLOEXEC);
//         if(handler_->get_fd()<0)
//         {
//             handler_->init(signal_fd,loop_);
//         }
//         else 
//         {
//            handler_.reset();
//            handler_=std::make_unique<EventHandler>(signal_fd,loop_); 
//         }
//         handler_->setReadCallBack(std::bind(&SignalHandler::handle, this));
//         handler_->setReading();
//         handler_->set_name("SignalHandler");        
//     }
    
//     void handle() {
//         EXCLUDE_BEFORE_COMPILATION(
//             LOG_SIGNAL_DEBUG("SignalHandler::handle()");
//         )
//         struct signalfd_siginfo sig_info;

//         int fd=handler_->get_fd();
//         sockets::readAuto(fd,&sig_info,sizeof(sig_info));

//         int sig = sig_info.ssi_signo;
//         auto it = callbacks_.find(sig);
//         assert(it!=callbacks_.end());
//         it->second();
//     }
    
// private:
//     EventLoop* loop_;
//     sigset_t signal_mask_;
//     std::unique_ptr<EventHandler> handler_;
//     std::map<int, SigCallBack> callbacks_;
// };
class SignalHandler {
public:
    typedef std::function<void()> SigCallBack;
    SignalHandler(EventLoop* loop):
    loop_(loop)
    {

    }
    static SignalHandler& getInstance(EventLoop* loop) {
        static SignalHandler instance(loop);
        return instance;
    }
    // int init() {
    //     sigset_t mask;
    //     sigemptyset(&mask);
    //     for (const auto& pair : handlers_) {
    //         sigaddset(&mask, pair.first);
    //     }
        
    //     // 阻塞这些信号，让 signalfd 捕获
    //     pthread_sigmask(SIG_BLOCK, &mask, nullptr);
        
    //     // 创建 signalfd
    //     signalFd_ = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    //     return signalFd_;
    // }
    SignalHandler& addSign(int sig, SigCallBack cb) 
    {

    }
    
    // 处理信号事件（在事件循环中调用）
    void handleRead() {
        struct signalfd_siginfo fdsi;
        ssize_t n = ::read(signalFd_, &fdsi, sizeof(fdsi));
        if (n != sizeof(fdsi)) return;
        
        int sig = fdsi.ssi_signo;
        std::function<void()> CallBacks_;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = handlers_.find(sig);
            if (it != handlers_.end()) {
                CallBacks_ = it->second;
            }
        }
        
        if (CallBacks_) {
            CallBacks_();  // 执行用户回调（在事件循环线程中）
        }
    }
    
private:
    EventLoop* loop_;
    EventHandler handler_;
    std::map<int,SigCallBack> CallBacks_;
    std::mutex mutex_;
};

}    
}

#endif