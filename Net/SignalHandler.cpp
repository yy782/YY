#include "SignalHandler.h"
#include "EventHandler.h"
#include "sockets.h"
#include "../Common/LogFilter.h"
#include <signal.h>
#include "EventLoop.h"
#include <functional>
#include <map>
namespace yy
{
namespace net
{
// SignalHandler::SignalHandler(EventLoop* loop):
// sigset_(),
// fd_(-1),
// loop_(loop),
// handler_()
// {
//     ::sigemptyset(&sigset_);

//     // fd_=sockets::setSignalOrDie(-1,&sigset_,SFD_NONBLOCK|SFD_CLOEXEC);
//     // handler_->init(fd_,loop_);
//     // handler_->setReadCallBack(std::bind(&SignalHandler::handle,this));
//     // handler_->setReading();
//     // handler_->set_name("SignalHandler");

// }  
// void SignalHandler::updateHandler()
// {
//     handler_=std::make_unique<EventHandler>(fd_,loop_);
//     handler_->setReadCallBack(std::bind(&SignalHandler::handle,this));
//     handler_->setReading();
//     handler_->set_name("SignalHandler");  
// }
// void SignalHandler::addSign(int sig,SigCallBack cb)
// {
    
//     ::sigaddset(&sigset_,sig);
//     ::sigprocmask(SIG_BLOCK,&sigset_,NULL);
    
//     fd_=sockets::setSignalOrDie(-1,&sigset_,SFD_NONBLOCK|SFD_CLOEXEC);   
//     callbacks_[sig]=std::move(cb);
//     // @brief 这里没有断言检查map里原来有没有对应的sig,主要是sig的信号处理是可以更新的，没必要
//     if(handler_)
//     {
//         handler_.reset();
//     }
//     updateHandler();
// }
// void SignalHandler::handle()
// {
//     EXCLUDE_BEFORE_COMPILATION(
//         LOG_SIGNAL_DEBUG("SignalHandler::handle()");
//     )
//     struct signalfd_siginfo sig_info;
    
//     sockets::readAuto(fd_,&sig_info,sizeof(sig_info));

//     int sig = sig_info.ssi_signo;
//     auto it = callbacks_.find(sig);
//     assert(it!=callbacks_.end());
//     it->second();
// } 
std::map<int,std::function<void()>> handlers;
void signal_handler(int sig) {
    handlers[sig]();
}


void Signal::signal(int sig, const std::function<void()> &handler) {
    handlers[sig] = handler;
    ::signal(sig, signal_handler);
}
}    
}
