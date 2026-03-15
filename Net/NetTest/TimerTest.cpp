
#include "../TimerQueue.h" 
#include "../../Common/SyncLog.h"
using namespace yy;
using namespace yy::net;
//./TimerTest
int main()
{
    SyncLog::getInstance("../Log.log").getFilter() 
        .set_global_level(LOG_LEVEL_DEBUG) 
        .set_module_enabled("TCP")
        .set_module_enabled("SYSTEM")
        .set_module_enabled("TIME");
    auto timer=std::make_shared<LTimer>([](){
        LOG_TIME_INFO("Timer Callback");
    },3s,1);
    EventLoop loop;
    TimerQueue<LowPrecision> timerQueue(&loop);
    timerQueue.insert(timer);
    loop.loop();
}