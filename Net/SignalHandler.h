#ifndef _YY_NET_SIGNALHANDLER_H_
#define _YY_NET_SIGNALHANDLER_H_

#include "../Common/noncopyable.h"
#include <signal.h>
#include <functional>

namespace yy
{
namespace net
{
struct Signal:noncopyable
{
    static void signal(int sig, const std::function<void()> &handler);
};
}    
}

#endif