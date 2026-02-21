#include "TimerWheel.h"
#include "Timer.h"
#include "EventHandler.h"
#include "../Common/Types.h"
#include "sockets.h"
namespace yy
{
namespace net
{
struct TimerWheel::Node
{
    WeakNodePtr prev;
    NodePtr next;

    LTimerPtr data;
    int rotation;//记录定时器在时间轮转多少圈后生效
    int time_slot;//记录定时器属于时间轮上哪个槽    
};    
TimerWheel::TimerWheel(EventLoop* loop):
cur_slot_(0),
handler_(new EventHandler(sockets::create_timerfd(CLOCK_MONOTONIC,TFD_CLOEXEC|TFD_NONBLOCK),loop))
{
    for(int i=0;i<MAX_SLOTS;++i)
    {
        slots_[i]=nullptr;
    }
    assert(handler_);
    int fd=handler_->get_fd();
    struct itimerspec new_ts;
    memZero(&new_ts,sizeof new_ts);
    new_ts.it_value.tv_sec=SI;
    new_ts.it_interval.tv_sec=SI;
    sockets::timerfd_settime(fd,0,new_ts);

    handler_->setReadCallBack(std::bind(&TimerWheel::tick,this));
}
TimerWheel::~TimerWheel()
{
    for(int i=0;i<MAX_SLOTS;++i)
    {
        auto tmp=slots_[i];
        while(tmp)
        {
            tmp=tmp->next;
        }
    }
    delete handler_;
}
void TimerWheel::add_timer(LTimerPtr timer)
{
    assert(timer!=nullptr);
    
    int ticks=0;
    int timeout=static_cast<int>(timer->getTimeInterval().getTimes());
    //计算超时值在多少个滴答后被触发，并把滴答数存储在ticks里，如果待插入定时器的超时值小于时间轮的槽间隔SI,ticks向上折合为1，否则向下折合timeout/SI
    if(timeout<SI){
        ticks=1;
    }else{
        ticks=timeout/SI;
    }
    int rotation=ticks/MAX_SLOTS;
    int ts=(cur_slot_+(ticks%MAX_SLOTS))%MAX_SLOTS;

    auto node=std::make_shared<Node>();
    node->data=timer;
    node->rotation=rotation;
    node->time_slot=ts;        


    if(!slots_[ts]){
        slots_[ts]=node;
        node->prev.lock()=nullptr;
    }else{
        node->next=slots_[ts];
        slots_[ts]->prev=node;
        slots_[ts]=node;
    }    
}
void TimerWheel::tick()
{
    auto tmp=slots_[cur_slot_];
    while(tmp){
        if(tmp->rotation>0){
            tmp->rotation--;
            tmp=tmp->next;
        }else{
            auto timer=tmp->data;
            assert(timer);
            timer->execute();
            if(timer->remain_count())add_timer(timer);
            
            auto next=tmp->next;//可能是nullptr,但是在接下来的操作不可能被释放，因为还保持引用
            auto prev=tmp->prev.lock();
            
            if(tmp==slots_[cur_slot_]){
                slots_[cur_slot_]=next;
                if(next){
                    next->prev.lock()=nullptr;
                }
            }else{
                prev->next=tmp->next;
                if(tmp->next){
                    next->prev=tmp->prev;
                }                        
            }
            tmp=next;    
        }    
    }
    ++cur_slot_;
    cur_slot_%=MAX_SLOTS;     
}  
   
}    
}