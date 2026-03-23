#include "TimerWheel.h"
#include "Timer.h"

#include "../Common/Types.h"
#include "sockets.h"
#include <memory>
#include "EventLoop.h"
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
TimerWheel::TimerWheel(EventLoop* loop,int maxSlots,int SI):
maxSlots_(maxSlots),
SI_(SI),
cur_slot_(0),
slots_(maxSlots),
loop_(loop),
handler_(sockets::createTimerFdOrDie(CLOCK_MONOTONIC,TFD_CLOEXEC|TFD_NONBLOCK),loop,"TimerWheel")
{
    for(int i=0;i<maxSlots_;++i)
    {
        slots_[i]=nullptr;
    }
    
    int fd=handler_.get_fd();
    struct itimerspec new_ts;
    memZero(&new_ts,sizeof new_ts);
    new_ts.it_value.tv_sec=1;
    new_ts.it_interval.tv_sec=SI_;
    sockets::timerfd_settime(fd,0,new_ts);

    handler_.setReadCallBack(std::bind(&TimerWheel::tick,this));
    handler_.setReading();
        
}
TimerWheel::~TimerWheel()
{
    for(int i=0;i<maxSlots_;++i)
    {
        slots_[i].reset();
    }
}
void TimerWheel::insert(LTimerPtr timer)
{
    assert(timer!=nullptr);

    LOG_TIME_DEBUG("Insert!");

    int ticks=0;
    int timeout=static_cast<int>(timer->getTimeInterval().getTimes());
    //计算超时值在多少个滴答后被触发，并把滴答数存储在ticks里，如果待插入定时器的超时值小于时间轮的槽间隔SI,ticks向上折合为1，否则向下折合timeout/SI_
    if(timeout<SI_){
        ticks=1;
    }else{
        ticks=timeout/SI_;
    }
    int rotation=ticks/maxSlots_;
    int ts=(cur_slot_+(ticks%maxSlots_))%maxSlots_;

    auto node=std::make_shared<Node>();
    node->data=timer;
    node->rotation=rotation;
    node->time_slot=ts;        

    LOG_TIME_DEBUG("node rotation:"<<rotation<<" time_slot:"<<ts);
    loop_->submit([this,ts,node](){ 
        if(!slots_[ts]){
            slots_[ts]=node;
        }else{
            node->next=slots_[ts];
            slots_[ts]->prev=node;
            slots_[ts]=node;
        } 
    });

    
    

}




void TimerWheel::tick()
{

    ReadTimerfd();

    EXCLUDE_BEFORE_COMPILATION(
        LOG_TIME_DEBUG("tick!");
    )

    //LOG_TIME_DEBUG("cur_slot_: "<<cur_slot_);

    auto tmp=slots_[cur_slot_];
    while(tmp){
        if(tmp->rotation>0){
            tmp->rotation--;
            tmp=tmp->next;
        }else{
            auto timer=tmp->data;
            assert(timer);


            timer->execute();

            if(timer->remain_count()>0)
            {
                insert(timer);
            }
            auto next=tmp->next;//可能是nullptr,但是在接下来的操作不可能被释放，因为还保持引用
            auto prev=tmp->prev.lock();
            
            if(tmp==slots_[cur_slot_]){
                slots_[cur_slot_]=next;
                if(next){
                    prev=nullptr;
                }
            }else{
                prev->next=next; 
                if(next) 
                {
                    next->prev=prev;
                }                        
            }
            tmp=next;    
        }    
    }
    ++cur_slot_;
    cur_slot_%=maxSlots_;     
}  
void TimerWheel::ReadTimerfd()
{
    uint64_t howmany;
    ssize_t n=sockets::read(handler_.get_fd(),&howmany,sizeof howmany);
    if(n!=sizeof howmany){
        EXCLUDE_BEFORE_COMPILATION(
            LOG_TIME_ERROR("TimerWheel::ReadTimerfd() read error");
        )
    }
}     
}    
}
