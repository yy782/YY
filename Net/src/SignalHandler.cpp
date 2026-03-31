#include "SignalHandler.h"
#include <signal.h>
#include <functional>
#include <map>
namespace yy
{
namespace net
{
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
