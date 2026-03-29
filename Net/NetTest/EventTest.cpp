#include "../EventLoopThread.h"
using namespace yy;
using namespace yy::net;
using namespace yy::net::sockets;

int main()
{
    EventLoopThread loopThread(1);
    auto loop=loopThread.run();
    while(true)
    {
        loop->submit([](){
            for(volatile int i=0;i<10000000;++i)
            {
                
            }
        },"");
    }
}