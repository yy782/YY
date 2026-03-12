#include "SignalHandler.h"
#include "EventHandler.h"
#include "sockets.h"
#include "../Common/LogFilter.h"
#include <signal.h>
#include "EventLoop.h"
namespace yy
{
namespace net
{
SignalHandler::SignalHandler(EventLoop* loop):
sigset_(),
handler_()
{
    ::sigemptyset(&sigset_);

    int fd=sockets::setSignalOrDie(-1,&sigset_,SFD_NONBLOCK|SFD_CLOEXEC);
    handler_.init(fd,loop);
    
    handler_.setReadCallBack(std::bind(&SignalHandler::handle,this));
    handler_.setReading();

    handler_.set_name("SignalHandler");


    assert(loop);
    loop->addListen(&handler_);
}  
void SignalHandler::addSign(int sig,SigCallBack cb)
{
    
    ::sigaddset(&sigset_,sig);
    ::sigprocmask(SIG_BLOCK,&sigset_,NULL);
    
    sockets::setSignalOrDie(-1,&sigset_,SFD_NONBLOCK|SFD_CLOEXEC);   
    callbacks_[sig]=std::move(cb);
    // @brief 这里没有断言检查map里原来有没有对应的sig,主要是sig的信号处理是可以更新的，没必要

}
void SignalHandler::handle()
{
    IGNORE(
        LOG_SIGNAL_DEBUG("SignalHandler::handle()");
    )
    struct signalfd_siginfo sig_info;

    int fd=handler_.get_fd();
    sockets::readAuto(fd,&sig_info,sizeof(sig_info));

    int sig = sig_info.ssi_signo;
    auto it = callbacks_.find(sig);
    assert(it!=callbacks_.end());
    it->second();
} 
}    
}
