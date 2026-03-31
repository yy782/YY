// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Modified by yy on 2026
// Additional modifications and improvements
#ifndef _YY_NET_TIMERQUEUE_H_
#define _YY_NET_TIMERQUEUE_H_

#include "EventLoop.h"
#include <set>
#include <memory>
#include "../Common/noncopyable.h"
#include "../Common/TimeStamp.h"
#include <sys/timerfd.h>
#include <vector>
#include <memory>
#include "Timer.h"
#include "EventHandler.h"
#include <utility>
namespace yy
{
namespace net
{

/**
 * @file TimerQueue.h
 * @brief 定时器队列类的定义
 * 
 * 本文件定义了定时器队列类，负责管理高精度的定时器任务。
 */

/**
 * @brief 定时器队列类
 * 
 * 模板类，负责管理高精度的定时器任务。
 * 
 * @tparam PrecisionTag 精度标签
 */
template<class PrecisionTag>
class TimerQueue:public noncopyable
{
public:
    /**
     * @brief 定时器类型
     */
    typedef Timer<PrecisionTag> PTimer;
    /**
     * @brief 时间戳类型
     */
    typedef typename PTimer::Time_Stamp Time_Stamp;
    /**
     * @brief 定时器指针类型
     */
    typedef std::shared_ptr<PTimer> TimerPtr;
    /**
     * @brief 事件处理器指针类型
     */
    typedef std::shared_ptr<EventHandler> EventHandlerPtr;
    /**
     * @brief 定时器条目类型
     */
    typedef std::pair<Time_Stamp,TimerPtr> Entry;
    /**
     * @brief 定时器列表类型
     */
    typedef std::set<Entry> TimerList;


    /**
     * @brief 删除默认构造函数
     */
    TimerQueue()=delete;
    
    /**
     * @brief 构造函数
     * 
     * @param loop 事件循环
     */
    TimerQueue(EventLoop* loop);
    
    /**
     * @brief 析构函数
     */
    ~TimerQueue();
    
    /**
     * @brief 插入定时器
     * 
     * @param cb 回调函数
     * @param interval 时间间隔
     * @param execute_count 执行次数
     */
    void insert(BaseTimer::TimerCallBack cb,typename PTimer::Time_Interval interval,int execute_count);
    
    /**
     * @brief 插入定时器
     * 
     * @param timer 定时器指针
     */
    void insert(TimerPtr timer);
    
    /**
     * @brief 取消定时器
     * 
     * @param timer 定时器指针
     */
    void cancelTimer(TimerPtr timer);
    
    /**
     * @brief 处理读事件
     */
    void handlerRead();
    
    /**
     * @brief 在事件循环中插入定时器
     * 
     * @param timer 定时器指针
     */
    void insertInLoop(TimerPtr timer);
    
    /**
     * @brief 在事件循环中取消定时器
     * 
     * @param timer 定时器指针
     */
    void cancelTimerInLoop(TimerPtr timer);

private:
    /**
     * @brief 读取timerfd
     */
    void ReadTimerfd();
    
    /**
     * @brief 修改timerfd
     */
    void modifyTimerfd();
    
    /**
     * @brief 获取到期的任务
     * 
     * @param now 当前时间
     * @return std::vector<Entry> 到期的任务列表
     */
    std::vector<Entry> getDueTasks(const Time_Stamp& now); 

    /**
     * @brief timerfd文件描述符
     */
    const int fd_;// InOne
    
    /**
     * @brief 事件处理器
     */
    EventHandler handler_;
    
    /**
     * @brief 定时器列表
     */
    TimerList timers_;// 需要外部保证
    //QuerySet querySet_;
};


template<class PrecisionTag>
void EventLoop::runTimer(BaseTimer::TimerCallBack cb,typename Timer<PrecisionTag>::Time_Interval interval,int execute_count)
{
    auto timer=std::make_shared<Timer<PrecisionTag>>(std::move(cb),std::move(interval),execute_count);
    runTimer<PrecisionTag>(timer);
}
template<class PrecisionTag>
void EventLoop::runTimer(std::shared_ptr<Timer<PrecisionTag>> timer)
{
    submit([timer,this]()mutable
    {///////////////////////////////////////////////
        assert(this->isInLoopThread());
        thread_local static TimerQueue<PrecisionTag> queue(this);
        queue.insertInLoop(timer);
    },std::string("EventLoop::runTimer"));
}
}    
}

#include "TimerQueue.tpp"

#endif