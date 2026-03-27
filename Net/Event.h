#ifndef YY_NET_EVENT_
#define YY_NET_EVENT_

#include <cstdint>
#include <assert.h>
#include <type_traits>

#include "../Common/copyable.h"
namespace yy {
namespace net {

/**
 * @file Event.h
 * @brief 事件类型和事件类的定义
 * 
 * 本文件定义了事件类型枚举和事件类，用于处理网络事件。
 */


/**
 * @brief 逻辑事件枚举
 * 
 * 定义了各种网络事件类型。
 */
enum class LogicEvent : uint32_t 
{
    None   = 0,        /**< 无事件 */
    Read   = 1 << 0,    /**< 读事件 */
    Except = 1 << 1,    /**< 异常事件 */
    Write  = 1 << 2,    /**< 写事件 */
    Error  = 1 << 3,    /**< 错误事件 */
    Hup    = 1 << 4,    /**< 连接关闭事件 */
    Nval   = 1 << 5,    /**< 无效事件 */
    RdHup  = 1 << 6,    /**< 读关闭事件 */
    Edge   = 1 << 7,    /**< 边缘触发 */
    OneShot = 1 << 8,   /**< 一次性事件 */
    
    // 常用组合
    ReadWrite = Read | Write,      /**< 读写事件 */
    AllEvents = Read | Write | Except | Error | Hup | Nval | RdHup,  /**< 所有事件 */
};

/**
 * @brief 位或运算符
 * 
 * @param lhs 左操作数
 * @param rhs 右操作数
 * @return LogicEvent 结果
 */
inline constexpr LogicEvent operator|(LogicEvent lhs, LogicEvent rhs) {
    return static_cast<LogicEvent>(
        static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs)
    );
}

/**
 * @brief 位与运算符
 * 
 * @param lhs 左操作数
 * @param rhs 右操作数
 * @return LogicEvent 结果
 */
inline constexpr LogicEvent operator&(LogicEvent lhs, LogicEvent rhs) {
    return static_cast<LogicEvent>(
        static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs)
    );
}

/**
 * @brief 位取反运算符
 * 
 * @param lhs 操作数
 * @return LogicEvent 结果
 */
inline constexpr LogicEvent operator~(LogicEvent lhs) {
    return static_cast<LogicEvent>(~static_cast<uint32_t>(lhs));
}

/**
 * @brief 等于运算符
 * 
 * @param lhs 左操作数
 * @param rhs 右操作数
 * @return bool 结果
 */
inline constexpr bool operator==(LogicEvent lhs, uint32_t rhs) {
    return static_cast<uint32_t>(lhs) == rhs;
}

/**
 * @brief 检查事件是否存在
 * 
 * @param lhs 左操作数
 * @param rhs 右操作数
 * @return bool 结果
 */
inline constexpr bool has_event(LogicEvent lhs, LogicEvent rhs) {
    return (static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs)) != 0;
}


/**
 * @brief 事件类
 * 
 * 保持与原有接口兼容，用于处理网络事件。
 */
class Event:copyable 
{
public:
    /**
     * @brief 默认构造函数
     */
    constexpr Event() noexcept : events_(LogicEvent::None) {}
    
    /**
     * @brief 构造函数
     * 
     * @param events 事件
     */
    constexpr explicit Event(LogicEvent events) noexcept : events_(events) {
        assert(validate(events_));
    }   
    
    /**
     * @brief 构造函数
     * 
     * @param events 事件
     */
    constexpr explicit Event(uint32_t events) noexcept : events_(static_cast<LogicEvent>(events)) {
        assert(validate(events_));
    }
    
    /**
     * @brief 获取事件
     * 
     * @return LogicEvent 事件
     */
    constexpr LogicEvent get() const noexcept { return events_; }
    
    /**
     * @brief 获取事件值
     * 
     * @return uint32_t 事件值
     */
    constexpr uint32_t value() const noexcept { return static_cast<uint32_t>(events_); }
    
    /**
     * @brief 添加事件
     * 
     * @param event 事件
     */
    constexpr void add(LogicEvent event) noexcept {
        assert(validate(event));
        events_ = events_ | event;
    }
    
    /**
     * @brief 添加事件（兼容原有接口）
     * 
     * @param event 事件
     */
    constexpr void add(uint32_t event) {
        add(static_cast<LogicEvent>(event));
    }
    
    /**
     * @brief 移除事件
     * 
     * @param event 事件
     */
    constexpr void remove(LogicEvent event) noexcept {
        assert(validate(event));
        events_ = static_cast<LogicEvent>(
            static_cast<uint32_t>(events_) & ~static_cast<uint32_t>(event)
        );
    }
    
    /**
     * @brief 移除事件（兼容原有接口）
     * 
     * @param event 事件
     */
    constexpr void remove(uint32_t event) {
        remove(static_cast<LogicEvent>(event));
    }
    
    /**
     * @brief 检查事件是否存在
     * 
     * @param event 事件
     * @return bool 是否存在
     */
    constexpr bool has(LogicEvent event) const noexcept {
        return has_event(events_, event);
    }
    
    /**
     * @brief 检查事件是否存在（兼容原有接口）
     * 
     * @param event 事件
     * @return bool 是否存在
     */
    constexpr bool has(uint32_t event) const {
        return has(static_cast<LogicEvent>(event));
    }
    
    /**
     * @brief 检查是否无事件
     * 
     * @return bool 是否无事件
     */
    constexpr bool is_none() const noexcept {
        return events_ == LogicEvent::None;
    }
    
    /**
     * @brief 赋值运算符
     * 
     * @param other 其他事件
     * @return Event& 引用
     */
    constexpr Event& operator=(LogicEvent other) noexcept {
        assert(validate(other));
        events_ = other;
        return *this;
    }
    
    /**
     * @brief 赋值运算符（兼容原有代码）
     * 
     * @param other 其他事件
     * @return Event& 引用
     */
    Event& operator=(uint32_t other) noexcept {
        LogicEvent le = static_cast<LogicEvent>(other);
        assert(validate(le));
        events_ = le;
        return *this;
    }   

    /**
     * @brief 等于运算符
     * 
     * @param other 其他事件
     * @return bool 结果
     */
    constexpr bool operator==(LogicEvent other) const noexcept {
        return events_ == other;
    }

    /**
     * @brief 不等于运算符
     * 
     * @param other 其他事件
     * @return bool 结果
     */
    constexpr bool operator!=(LogicEvent other) const noexcept {
        return !(*this == other);
    }
    
    /**
     * @brief 位与运算符
     * 
     * @param other 其他事件
     * @return bool 结果
     */
    constexpr bool operator&(LogicEvent other) const noexcept {
        return has(other);
    }
    
    /**
     * @brief 位或赋值运算符
     * 
     * @param other 其他事件
     * @return Event& 引用
     */
    constexpr Event& operator|=(LogicEvent other) noexcept {
        add(other);
        return *this;
    }
    
    /**
     * @brief 位与赋值运算符
     * 
     * @param other 其他事件
     * @return Event& 引用
     */
    constexpr Event& operator&=(LogicEvent other) noexcept {
        events_ = events_ & other;
        return *this;
    }
    
    /**
     * @brief 等于运算符（兼容原有接口）
     * 
     * @tparam T 类型
     * @param other 其他对象
     * @return bool 结果
     */
    template<typename T>
    bool operator==(const T& other) const {
        return events_ == static_cast<LogicEvent>(other.get_event());
    }
    
    /**
     * @brief 不等于运算符（兼容原有接口）
     * 
     * @tparam T 类型
     * @param other 其他对象
     * @return bool 结果
     */
    template<typename T>
    bool operator!=(const T& other) const {
        return !(*this == other);
    }
    
private:
    /**
     * @brief 验证事件
     * 
     * @param events 事件
     * @return bool 是否有效
     */
    static constexpr bool validate(LogicEvent events) noexcept 
    {
        constexpr auto valid_mask = LogicEvent::AllEvents | LogicEvent::Edge | LogicEvent::OneShot;
        uint32_t invalid = static_cast<uint32_t>(events) & ~static_cast<uint32_t>(valid_mask);
        return invalid == 0;
    }
    
    /**
     * @brief 事件
     */
    LogicEvent events_;
};


} // namespace net
}
#endif