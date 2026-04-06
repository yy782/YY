#ifndef _YY_NET_TIMER_H_
#define _YY_NET_TIMER_H_

#include <functional>
#include <assert.h>
#include "../Common/noncopyable.h"
#include "../Common/TimeStamp.h"
#include <atomic>

namespace yy
{
namespace net
{
struct Base;
/**
 * @file Timer.h
 * @brief 定时器类的定义
 * 
 * 本文件定义了定时器类，支持不同精度的定时器。
 */

/**
 * @brief 定时器前向声明
 * 
 * @tparam PrecisionTag 精度标签
 */
template<typename PrecisionTag>  
class Timer;

/**
 * @brief 基础定时器类型，类似于基类
 */
typedef Timer<Base> BaseTimer; 

/**
 * @brief 低精度定时器类型
 */
typedef Timer<LowPrecision> LTimer;

/**
 * @brief 高精度定时器类型
 */
typedef Timer<HighPrecision> HTimer;

/**
 * @brief 低精度定时器指针类型
 */
typedef std::shared_ptr<LTimer> LTimerPtr;

/**
 * @brief 高精度定时器指针类型
 */
typedef std::shared_ptr<HTimer> HTimerPtr;

/**
 * @brief 基础定时器结构
 * 
 * 定义了定时器的回调函数类型和常量。
 */
template<>
struct Timer<Base>:public noncopyable
{
    /**
     * @brief 定时器回调函数类型
     */
    typedef std::function<void()> TimerCallBack;   
    /**
     * @brief 永久执行标志
     */
    static constexpr int FOREVER = -1; 
};

/**
 * @brief 定时器类
 * 
 * 模板类，支持不同精度的定时器。
 * 
 * @tparam PrecisionTag 精度标签
 */
template<typename PrecisionTag>  
class Timer:public noncopyable
{
public:
    /**
     * @brief 时间戳类型
     */
    typedef TimeStamp<PrecisionTag> Time_Stamp;
    /**
     * @brief 时间间隔类型
     */
    typedef TimeInterval<PrecisionTag> Time_Interval;
    
    /**
     * @brief 构造函数
     * 
     * @param cb 回调函数
     * @param interval 时间间隔
     * @param execute_count 执行次数
     */
    Timer(BaseTimer::TimerCallBack cb,Time_Interval interval,int execute_count):
    callback_(std::move(cb)),
    interval_(interval),
    execute_count_(execute_count),
    expiration_(Time_Stamp::now()+interval)
    {
        assert((execute_count_.load()<0&&execute_count_.load()==BaseTimer::FOREVER)||execute_count_.load()>=0);
    }
    
    /**
     * @brief 获取剩余执行次数
     * 
     * @return int 剩余执行次数
     */
    int remain_count()const{return execute_count_.load();}
    
    /**
     * @brief 修改执行次数
     * 
     * @param count 执行次数
     */
    void modifyExecuteCount(size_t count){execute_count_.store(count);}
    
    /**
     * @brief 获取定时器时间戳
     * 
     * @return Time_Stamp& 时间戳
     */
    Time_Stamp& getTimerStamp(){return expiration_;}
    
    /**
     * @brief 获取定时器时间戳
     * 
     * @return const Time_Stamp& 时间戳
     */
    const Time_Stamp& getTimerStamp()const {return expiration_;}
    
    /**
     * @brief 获取时间间隔
     * 
     * @return Time_Interval& 时间间隔
     */
    Time_Interval& getTimeInterval(){return interval_;}
    
    /**
     * @brief 获取时间间隔
     * 
     * @return const Time_Interval& 时间间隔
     */
    const Time_Interval& getTimeInterval()const{return interval_;}
    
    /**
     * @brief 执行定时器回调
     * 
     * 执行回调函数，并更新定时器的过期时间。
     */
    void execute()
    {
        assert(execute_count_.load()!=0);
        
        if(execute_count_.load()!=BaseTimer::FOREVER)
        {
            --execute_count_;
            
        }
        callback_();
        expiration_=Time_Stamp::now()+interval_;
    }
    
    /**
     * @brief 设置剩余执行次数
     * 
     * @param c 剩余执行次数
     */
    void setRemainCount(int c){execute_count_=c;}
    
    /**
     * @brief 取消定时器
     */
    void cancel(){execute_count_=0;}
private:
    /**
     * @brief 回调函数
     */
    const BaseTimer::TimerCallBack callback_;
    
    /**
     * @brief 时间间隔
     */
    Time_Interval interval_;
    
    /**
     * @brief 执行次数
     */
    std::atomic<int> execute_count_;
    
    /**
     * @brief 过期时间
     */
    Time_Stamp expiration_;
};


}    
}

#endif