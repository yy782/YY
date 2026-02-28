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
    handler_=std::make_shared<EventHandler>(sockets::set_signalfd(-1,&sigset_,SFD_NONBLOCK|SFD_CLOEXEC),loop);
    assert(handler_);
    handler_->setReadCallBack(std::bind(&SignalHandler::handle,this));
    handler_->setReading();

    handler_->set_name("SignalHandler");


    assert(loop);
    loop->submit(std::bind(&EventLoop::addListen,loop,handler_));
}  
void SignalHandler::addSign(int sig,SigCallBack cb)
{
    
    ::sigaddset(&sigset_,sig);
    ::sigprocmask(SIG_BLOCK,&sigset_,NULL);
    int fd=handler_->get_fd();
    sockets::set_signalfd(fd,&sigset_,SFD_NONBLOCK|SFD_CLOEXEC);   
    callbacks_[sig]=std::move(cb);
    // @brief 这里没有断言检查map里原来有没有对应的sig,主要是sig的信号处理是可以更新的，没必要

}
void SignalHandler::handle()
{
    LOG_SIGNAL_DEBUG("SignalHandler::handle()");
    struct signalfd_siginfo sig_info;
    assert(handler_);
    int fd=handler_->get_fd();
    sockets::read(fd,&sig_info,sizeof(sig_info));

    int sig = sig_info.ssi_signo;
    auto it = callbacks_.find(sig);
    assert(it!=callbacks_.end());
    it->second();
} 
}    
}