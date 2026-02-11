#ifndef _YY_NET_CLIENT_
#define _YY_NET_CLIENT_

#include "../Common/Errno.h"
#include "../Common/Log.h"
#include "Socket.h"
#include "InetAddress.h"
namespace yy
{
namespace net
{
namespace client
{
    void connect(const Socket& k,const Address& addr)
    {
        auto ret=::connect(k.get_fd(),addr.getSockAddr(),addr.get_len());
        if(ret==-1)
        {
            LOG_PRINT_ERRNO;
        }
    } 
}    
}    
}

#endif