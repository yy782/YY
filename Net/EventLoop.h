#ifndef _YY_NET_EVENTLOOP_H_
#define _YY_NET_EVENTLOOP_H_
#include <vector>
#include <functional>
#include <queue>
#include <mutex>



#include "EventHandler.h"
#include "sockets.h"
#include "PollerType.h"
#include "../Common/noncopyable.h"
#include "Timer.h"
#include "../Common/locker.h"
#include "../Common/RingBuffer.h"
namespace yy
{
namespace net
{

struct Fun
{
    std::function<void()> Functor_;
    std::string name_;
    Fun()=default;
    template<typename Callable>
    Fun(Callable&& callable, std::string name)
        : Functor_(std::forward<Callable>(callable))
        , name_(std::move(name))
    {
    }  
    std::string getName(){return name_;} 
    void operator()()
    {
        Functor_();
    }
};

/**
 * @brief 事件循环类，负责处理IO事件和任务调度
 * 
 * EventLoop是网络库的核心类，每个线程只能有一个EventLoop实例。
 * 它负责监听文件描述符上的事件，并在事件发生时调用相应的回调函数。
 * 同时，它也提供了任务队列，用于在IO线程中执行任务。
 */
class EventLoop:public noncopyable
{
public:
    
    /**
     * @brief 线程ID类型
     */
    typedef Thread::Pid_t Pid_t;
    /**
     * @brief 任务函数类型
     */
    typedef Fun Functor;
    /**
     * @brief 函数列表类型
     */
    typedef RingBuffer<Functor> FunctionList;

    /**
     * @brief 构造函数
     * 
     * 创建一个EventLoop实例，初始化事件处理器和唤醒机制。
     */
    EventLoop(int id=-1);
    
    /**
     * @brief 析构函数
     * 
     * 析构EventLoop实例，确保在IO线程中析构。
     */
    ~EventLoop()
    {
        assert(isInLoopThread());
    }
    
    /**
     * @brief 启动事件循环
     * 
     * 进入事件循环，处理IO事件和任务队列中的任务。
     */
    void loop();
    
    /**
     * @brief 退出事件循环
     * 
     * 通知事件循环退出，不再处理新的事件。
     */
    void quit();
    
    /**
     * @brief 检查事件循环是否正在退出
     * 
     * @return bool 事件循环是否正在退出
     */
    bool isQuit() noexcept;

    /**
     * @brief 检查当前线程是否是事件循环线程
     * 
     * @return bool 当前线程是否是事件循环线程
     */
    bool isInLoopThread() noexcept;

    /**
     * @brief 提交任务到事件循环线程
     * 
     * @tparam Callable 可调用对象类型
     * @param cb 要执行的任务
     * 
     * 如果当前线程是事件循环线程，直接执行任务；否则，将任务添加到任务队列。
     */
    template<typename Callable>
    void submit(Callable&& cb,const std::string& TaskName);
    
    /**
     * @brief 延迟执行任务
     * 
     * @tparam Callable 可调用对象类型
     * @param cb 要执行的任务
     * 
     * 如果任务队列已满，将任务放入定时器，延迟执行。
     */
    template<bool isInLoop,typename Callable>
    void DelayedExecution(Callable&& cb);
    
    /**
     * @brief 运行定时器
     * 
     * @tparam PrecisionTag 精度标签
     * @param cb 定时器回调函数
     * @param interval 定时器间隔
     * @param execute_count 执行次数
     * 
     * 启动一个定时器，按照指定的间隔执行回调函数。
     */
    template<class PrecisionTag>
    void runTimer(BaseTimer::TimerCallBack cb,typename Timer<PrecisionTag>::Time_Interval interval,int execute_count);
    template<class PrecisionTag>
    void runTimer(std::shared_ptr<Timer<PrecisionTag>> timer);

private:
    friend class EventHandler;
    
    /**
     * @brief 唤醒事件循环
     * 
     * 向事件循环发送唤醒信号，使其处理任务队列中的任务。
     */
    void wakeup();
    
    /**
     * @brief 添加事件监听器
     * 
     * @param handler 事件处理器
     * 
     * 将事件处理器添加到事件监听器中。
     */
    void addListen(EventHandler* handler,const std::string& addInformation)
    {
        assert(handler);
        if(isInLoopThread())
        {
            LOG_LOOP_DEBUG("loopID:"<<id_<<addInformation);
            poller_.add_listen(handler);
        }        
        else
            submit([this, handler]()
            {
                poller_.add_listen(handler);
            },addInformation);
    }
    
    /**
     * @brief 更新事件监听器
     * 
     * @param handler 事件处理器
     * 
     * 更新事件处理器在事件监听器中的状态。
     */
    void update_listen(EventHandler* handler,const std::string& upInformation)
    { 
        assert(handler);    
        if(isInLoopThread())
            poller_.update_listen(handler);
        else
            submit([this, handler]()
            {
                poller_.update_listen(handler);
            },upInformation);
    }
    
    /**
     * @brief 移除事件监听器
     * 
     * @param handler 事件处理器
     * 
     * 从事件监听器中移除事件处理器。
     */
    void remove_listen(EventHandler* handler,const std::string& removeInformation)
    {
        assert(handler);
        if(isInLoopThread())
            poller_.remove_listen(handler);
        else
            submit([this, handler]()
            {
                poller_.remove_listen(handler);
            },removeInformation);
    }        
    
    /**
     * @brief 执行待处理的任务
     * 
     * 处理任务队列中的任务。
     */
    void doPendingFunctions();
    
    /**
     * @brief 检查事件循环状态
     * 
     * @return bool 事件循环状态是否有效
     */
    bool CheckeEventLoopStatus();
    
    /**
     * @brief 事件监听器
     */
    PollerType poller_;// InLoop
    int id_;
    /**
     * @brief 活跃的事件处理器列表
     */
    PollerHandlerList activeHandlers_;
    
    /**
     * @brief 任务队列
     */
    FunctionList FunctionList_;// Not
    
    /**
     * @brief 事件循环线程ID
     */
    Pid_t threadId_;// InLoop
    
    /**
     * @brief 事件循环状态
     */
    int status_;
    
    /**
     * @brief 唤醒事件处理器
     */
    EventHandler wakeHandler_;

    //std::mutex mtx_;

    //int LoopNum={0};
    //std::string name_;
    
};

/**
 * @brief 提交任务到事件循环线程
 * 
 * @tparam Callable 可调用对象类型
 * @param cb 要执行的任务
 * 
 * 如果当前线程是事件循环线程，直接执行任务；否则，将任务添加到任务队列。
 */
template<typename Callable>
void EventLoop::submit(Callable&& cb,const std::string& TaskNameInformation)
{
    if(isInLoopThread())
    {
        cb();
    }
    else 
    {
        assert(!isInLoopThread());
        FunctionList_.blockappend(std::forward<Callable>(cb),TaskNameInformation);
    }
}

/**
 * @brief 延迟执行任务
 * 
 * @tparam Callable 可调用对象类型
 * @param cb 要执行的任务
 * 
 * 如果任务队列已满，将任务放入定时器，延迟执行。
 */
template<bool isInLoop,typename Callable>
void EventLoop::DelayedExecution(Callable&& cb,const std::string& DelayedExecution)
{
    if constexpr (isInLoop)
    {
        assert(isInLoopThread());
        if(!FunctionList_.append(std::forward<Callable>(cb)))
        {
            runTimer<LowPrecision>([Fun=std::forward<Callable>(cb),this]()mutable
            {
                DelayedExecution<true>(std::forward<Callable>(Fun));
            },5s,1);
        }
    }
    else 
    {
        assert(!isInLoopThread());
        FunctionList_.blockappend(std::forward<Callable>(cb));
    }
}
}    
}
#endif // _YY_NET_EVENTLOOP_H_