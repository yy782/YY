#ifndef YY_NET_EVENTLOOP_H_
#define YY_NET_EVENTLOOP_H_
// #include <vector>
// #include <functional>
// #include <queue>
// #include <mutex>



// #include "EventHandler.h"
// #include "sockets.h"
// #include "PollerType.h"
// #include "../Common/noncopyable.h"
// #include "Timer.h"
// #include "../Common/locker.h"
// #include "../Common/RingBuffer.h"
// #include "../Common/SafeVector.h"
// #include <iostream>
// #include <queue>

// namespace yy
// {
// namespace net
// {

// struct Fun
// {
//     std::function<void()> Functor_;
//     std::string name_;
//     Fun()=default;
//     template<typename Callable>
//     Fun(Callable&& callable, std::string name)
//         : Functor_(std::forward<Callable>(callable))
//         , name_(std::move(name))
//     {
//         assert(Functor_);
//     }  
//     std::string getName(){return name_;} 
//     void operator()()
//     {
//         Functor_();
//     }
//     bool operator!() const
//     {
//         return !Functor_;
//     }
// };



// class EventLoop:public noncopyable
// {
// public:
    
//     /**
//      * @brief 线程ID类型
//      */
//     typedef Thread::Pid_t Pid_t;
//     /**
//      * @brief 任务函数类型
//      */
//     typedef Fun Functor;
//     /**
//      * @brief 函数列表类型
//      */
//     typedef RingBuffer<Functor> FunctionList;  
//     typedef SafeVector<Functor> SecondaryFuncList;
//     /**
//      * @brief 构造函数
//      * 
//      * 创建一个EventLoop实例，初始化事件处理器和唤醒机制。
//      */
//     EventLoop(int id=-1);
    
//     /**
//      * @brief 析构函数
//      * 
//      * 析构EventLoop实例，确保在IO线程中析构。
//      */
//     ~EventLoop()
//     {
//         int uncaughtCount = std::uncaught_exceptions();
//         if(uncaughtCount > 0)
//         {
//             LOG_LOOP_ERROR("loopID:"<<id_<<" 析构时有未捕获的异常，数量: " << uncaughtCount);
//             assert(uncaughtCount==0); // 确保没有未捕获的异常
//         }
//         assert(isInLoopThread());
//         LOG_LOOP_DEBUG("loopID:"<<id_<<" 析构");
//     }
    
//     /**
//      * @brief 启动事件循环
//      * 
//      * 进入事件循环，处理IO事件和任务队列中的任务。
//      */
//     void loop();
    
//     /**
//      * @brief 退出事件循环
//      * 
//      * 通知事件循环退出，不再处理新的事件。
//      */
//     void quit();
    
//     /**
//      * @brief 检查事件循环是否正在退出
//      * 
//      * @return bool 事件循环是否正在退出
//      */
//     bool isQuit() noexcept;

//     /**
//      * @brief 检查当前线程是否是事件循环线程
//      * 
//      * @return bool 当前线程是否是事件循环线程
//      */
//     bool isInLoopThread() noexcept;

//     /**
//      * @brief 提交任务到事件循环线程
//      * 
//      * @tparam Callable 可调用对象类型
//      * @param cb 要执行的任务
//      * 
//      * 如果当前线程是事件循环线程，直接执行任务；否则，将任务添加到任务队列。
//      */
//     template<typename Callable>
//     void submit(Callable&& cb,const std::string& TaskName);
    
//     /**
//      * @brief 延迟执行任务
//      * 
//      * @tparam Callable 可调用对象类型
//      * @param cb 要执行的任务
//      * 
//      * 如果任务队列已满，将任务放入定时器，延迟执行。
//      */
//     template<typename Callable>
//     void DelayedExecution(Callable&& cb,const std::string& DelayedExecutionInformation);
    
//     /**
//      * @brief 运行定时器
//      * 
//      * @tparam PrecisionTag 精度标签
//      * @param cb 定时器回调函数
//      * @param interval 定时器间隔
//      * @param execute_count 执行次数
//      * 
//      * 启动一个定时器，按照指定的间隔执行回调函数。
//      */
//     template<class PrecisionTag>
//     void runTimer(BaseTimer::TimerCallBack cb,typename Timer<PrecisionTag>::Time_Interval interval,int execute_count);
//     template<class PrecisionTag>
//     void runTimer(std::shared_ptr<Timer<PrecisionTag>> timer);


//     int id() const noexcept{return id_;}

// private:
//     friend class EventHandler;
    
//     /**
//      * @brief 唤醒事件循环
//      * 
//      * 向事件循环发送唤醒信号，使其处理任务队列中的任务。
//      */
//     void wakeup();
    
//     /**
//      * @brief 添加事件监听器
//      * 
//      * @param handler 事件处理器
//      * 
//      * 将事件处理器添加到事件监听器中。
//      */
//     void addListen(EventHandler* handler,const std::string& addInformation)
//     {
//         assert(handler);
//         if(isInLoopThread())
//         {
//             LOG_LOOP_DEBUG("loopID:"<<id_<<" "<<addInformation);
//             poller_.add_listen(handler);
//         }        
//         else
//             submit([this, handler]()
//             {
//                 poller_.add_listen(handler);
//             },addInformation);
//     }
    
//     /**
//      * @brief 更新事件监听器
//      * 
//      * @param handler 事件处理器
//      * 
//      * 更新事件处理器在事件监听器中的状态。
//      */
//     void update_listen(EventHandler* handler,const std::string& upInformation="No")
//     { 
//         assert(handler);    
//         if(isInLoopThread())
//             poller_.update_listen(handler);
//         else
//             submit([this, handler]()
//             {
//                 poller_.update_listen(handler);
//             },upInformation);
//     }
    
//     /**
//      * @brief 移除事件监听器
//      * 
//      * @param handler 事件处理器
//      * 
//      * 从事件监听器中移除事件处理器。
//      */
//     void remove_listen(EventHandler* handler,const std::string& removeInformation)
//     {
//         assert(handler);
//         if(isInLoopThread())
//             poller_.remove_listen(handler);
//         else
//             submit([this, handler]()
//             {
//                 poller_.remove_listen(handler);
//             },removeInformation);
//     }        
    
//     /**
//      * @brief 执行待处理的任务
//      * 
//      * 处理任务队列中的任务。
//      */
//     void doPendingFunctions();
    
//     /**
//      * @brief 检查事件循环状态
//      * 
//      * @return bool 事件循环状态是否有效
//      */
//     bool CheckeEventLoopStatus();
    
//     /**
//      * @brief 事件监听器
//      */
//     PollerType poller_;// InLoop
//     int id_;
//     /**
//      * @brief 活跃的事件处理器列表
//      */
//     PollerHandlerList activeHandlers_;
    
//     /**
//      * @brief 任务队列
//      */
//     FunctionList FunctionList_;// Not
//     SecondaryFuncList AsyncTaskQueue_;// InLoop
//     /**
//      * @brief 事件循环线程ID
//      */
//     Pid_t threadId_;
    
//     /**
//      * @brief 事件循环状态
//      */
//     int status_;
    
//     /**
//      * @brief 唤醒事件处理器
//      */
//     EventHandler wakeHandler_;

// };


// template<typename Callable>
// void EventLoop::submit(Callable&& cb,const std::string& TaskNameInformation)
// {
//     if(isInLoopThread())
//     {
//         cb();
//     }
//     else 
//     {
//         assert(!isInLoopThread());
//         Fun functor(std::forward<Callable>(cb),TaskNameInformation);
//         if(!FunctionList_.append(functor))
//         {
//             AsyncTaskQueue_.push_back(functor);
//         }
//         wakeup();
//     }
// }

// template<typename Callable>
// void EventLoop::DelayedExecution(Callable&& cb,const std::string& DelayedExecutionInformation)
// {
//     Fun functor(std::forward<Callable>(cb),DelayedExecutionInformation);
//     if(!FunctionList_.append(functor))
//     {
//         AsyncTaskQueue_.push_back(functor);
//     }
//     wakeup();
// }



// }    
// }

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
#include "../Common/concurrentqueue.h"
#include <iostream>
#include <queue>

namespace yy
{
namespace net
{

struct Fun
{
    std::function<void()> Functor_;
#ifndef NDEBUG
    std::string name_;
#endif

    Fun()=default;
    template<typename Callable>
    Fun(Callable&& callable,const std::string& name): 
    Functor_(std::forward<Callable>(callable)),
#ifndef NDEBUG
    name_(name)
#endif
    {
        assert(Functor_);
    }  
    void operator()()
    {
        Functor_();
    }
    bool operator!() const
    {
        return !Functor_;
    }
};



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
    typedef moodycamel::ConcurrentQueue<Functor> FunctionList;  

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
        int uncaughtCount = std::uncaught_exceptions();
        if(uncaughtCount > 0)
        {
            LOG_LOOP_ERROR("loopID:"<<id_<<" 析构时有未捕获的异常，数量: " << uncaughtCount);
            assert(uncaughtCount==0); // 确保没有未捕获的异常
        }
        assert(isInLoopThread());
        LOG_LOOP_DEBUG("loopID:"<<id_<<" 析构");
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
    template<typename Callable>
    void DelayedExecution(Callable&& cb,const std::string& DelayedExecutionInformation);
    
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


    int id() const noexcept{return id_;}

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
            LOG_LOOP_DEBUG("loopID:"<<id_<<" "<<addInformation);
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
    void update_listen(EventHandler* handler,const std::string& upInformation="No")
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
    moodycamel::ProducerToken token_;
    /**
     * @brief 事件循环线程ID
     */
    Pid_t threadId_;
    
    /**
     * @brief 事件循环状态
     */
    int status_;
    
    /**
     * @brief 唤醒事件处理器
     */
    EventHandler wakeHandler_;

};


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
        FunctionList_.enqueue(Fun(std::forward<Callable>(cb),TaskNameInformation));
        wakeup();
    }
}

template<typename Callable>
void EventLoop::DelayedExecution(Callable&& cb,const std::string& DelayedExecutionInformation)
{

    FunctionList_.enqueue(Fun(std::forward<Callable>(cb),DelayedExecutionInformation));
    wakeup();
}
}
}
#endif // _YY_NET_EVENTLOOP_H_