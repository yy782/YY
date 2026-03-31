#ifndef _YY_NET_TIMETWHEEL_H_
#define _YY_NET_TIMETWHEEL_H_

#include "../Common/noncopyable.h"
#include "../Common/TimeStamp.h"
#include <memory>
#include <assert.h>
#include "Timer.h"
#include "EventHandler.h"
#include <vector>
namespace yy
{
namespace net
{

/**
 * @file TimerWheel.h
 * @brief 时间轮类的定义
 * 
 * 本文件定义了时间轮类，用于管理低精度的定时器任务。
 */

class EventLoop;

/**
 * @brief 时间轮类
 * 
 * 用于管理低精度的定时器任务，使用时间轮算法来提高定时器的管理效率。
 */
class TimerWheel:public noncopyable
{
public:
    /**
     * @brief 节点结构体
     */
    struct Node;
    /**
     * @brief 节点指针类型
     */
    typedef std::shared_ptr<Node> NodePtr;
    /**
     * @brief 弱节点指针类型
     */
    typedef std::weak_ptr<Node> WeakNodePtr;  
    
    /**
     * @brief 删除默认构造函数
     */
    TimerWheel()=delete;
    
    /**
     * @brief 构造函数
     * 
     * @param loop 事件循环
     * @param maxSlots 最大槽数
     * @param SI 时间间隔
     */
    TimerWheel(EventLoop* loop,int maxSlots,int SI);  
    
    /**
     * @brief 析构函数
     */
    ~TimerWheel();
    
    /**
     * @brief 插入定时器
     * 
     * @param cb 回调函数
     * @param interval 时间间隔
     * @param execute_count 执行次数
     */
    void insert(Timer<Base>::TimerCallBack cb,LTimer::Time_Interval interval,int execute_count)
    {
        auto timer=std::make_shared<LTimer>(std::move(cb),interval,execute_count);
        insert(timer);
    }
    
    /**
     * @brief 插入定时器
     * 
     * @param timer 定时器指针
     */
    void insert(LTimerPtr timer);
    
    /**
     * @brief 时间轮 tick
     * 
     * 处理时间轮的刻度，触发到期的定时器。
     */
    void tick();
    
private:
    /**
     * @brief 读取timerfd
     */
    void ReadTimerfd();
    
    /**
     * @brief timerfd文件描述符
     */
    const int fd_;//InOne
    
    /**
     * @brief 最大槽数
     */
    const int maxSlots_;
    
    /**
     * @brief 时间间隔
     */
    const int SI_;
    
    /**
     * @brief 当前槽位
     */
    int cur_slot_;// 需要保证
    
    /**
     * @brief 槽位数组
     */
    std::vector<NodePtr> slots_;
    
    /**
     * @brief 事件循环
     */
    EventLoop* loop_;
    
    /**
     * @brief 事件处理器
     */
    EventHandler handler_;
};
   
}    
}
#endif