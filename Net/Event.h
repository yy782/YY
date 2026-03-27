#ifndef YY_NET_EVENT_
#define YY_NET_EVENT_

#include <cstdint>
#include <assert.h>
#include <type_traits>

#include "../Common/copyable.h"
namespace yy {
namespace net {

enum class LogicEvent : uint32_t 
{
    None   = 0,
    Read   = 1 << 0,
    Except = 1 << 1,    
    Write  = 1 << 2,
    Error  = 1 << 3,
    Hup    = 1 << 4,
    Nval   = 1 << 5,
    RdHup  = 1 << 6,
    Edge   = 1 << 7,
    OneShot = 1 << 8,
    
    // 常用组合
    ReadWrite = Read | Write,
    AllEvents = Read | Write | Except | Error | Hup | Nval | RdHup,
};

// 位操作运算符
inline constexpr LogicEvent operator|(LogicEvent lhs, LogicEvent rhs) {
    return static_cast<LogicEvent>(
        static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs)
    );
}

inline constexpr LogicEvent operator&(LogicEvent lhs, LogicEvent rhs) {
    return static_cast<LogicEvent>(
        static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs)
    );
}

inline constexpr LogicEvent operator~(LogicEvent lhs) {
    return static_cast<LogicEvent>(~static_cast<uint32_t>(lhs));
}

inline constexpr bool operator==(LogicEvent lhs, uint32_t rhs) {
    return static_cast<uint32_t>(lhs) == rhs;
}

inline constexpr bool has_event(LogicEvent lhs, LogicEvent rhs) {
    return (static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs)) != 0;
}


// 事件类 - 保持与原有接口兼容
class Event:copyable 
{
public:

    constexpr Event() noexcept : events_(LogicEvent::None) {}
    constexpr explicit Event(LogicEvent events) noexcept : events_(events) {
        assert(validate(events_));
    }   
    constexpr explicit Event(uint32_t events) noexcept : events_(static_cast<LogicEvent>(events)) {
        assert(validate(events_));
    }
    constexpr LogicEvent get() const noexcept { return events_; }
    constexpr uint32_t value() const noexcept { return static_cast<uint32_t>(events_); }
    constexpr void add(LogicEvent event) noexcept {
        assert(validate(event));
        events_ = events_ | event;
    }
    
    // 兼容原有接口：添加事件（通过 uint32_t）
    constexpr void add(uint32_t event) {
        add(static_cast<LogicEvent>(event));
    }
    
    constexpr void remove(LogicEvent event) noexcept {
        assert(validate(event));
        events_ = static_cast<LogicEvent>(
            static_cast<uint32_t>(events_) & ~static_cast<uint32_t>(event)
        );
    }
    
    // 兼容原有接口：移除事件
    constexpr void remove(uint32_t event) {
        remove(static_cast<LogicEvent>(event));
    }
    
    constexpr bool has(LogicEvent event) const noexcept {
        return has_event(events_, event);
    }
    
    // 兼容原有接口：检查事件
    constexpr bool has(uint32_t event) const {
        return has(static_cast<LogicEvent>(event));
    }
    
    constexpr bool is_none() const noexcept {
        return events_ == LogicEvent::None;
    }
    // 赋值运算符
    constexpr Event& operator=(LogicEvent other) noexcept {
        assert(validate(other));
        events_ = other;
        return *this;
    }
    
    // 从 uint32_t 赋值（兼容原有代码）
    Event& operator=(uint32_t other) noexcept {
        LogicEvent le = static_cast<LogicEvent>(other);
        assert(validate(le));
        events_ = le;
        return *this;
    }   

    // 运算符重载 - 保持与原有代码兼容
    constexpr bool operator==(LogicEvent other) const noexcept {
        return events_ == other;
    }

    constexpr bool operator!=(LogicEvent other) const noexcept {
        return !(*this == other);
    }
    
    constexpr bool operator&(LogicEvent other) const noexcept {
        return has(other);
    }
    
    constexpr Event& operator|=(LogicEvent other) noexcept {
        add(other);
        return *this;
    }
    
    constexpr Event& operator&=(LogicEvent other) noexcept {
        events_ = events_ & other;
        return *this;
    }
    
    // 兼容原有接口：与 EventType 的比较（如果你还有 EventType 类）
    template<typename T>
    bool operator==(const T& other) const {
        return events_ == static_cast<LogicEvent>(other.get_event());
    }
    
    template<typename T>
    bool operator!=(const T& other) const {
        return !(*this == other);
    }
    
private:
    static constexpr bool validate(LogicEvent events) noexcept 
    {
        constexpr auto valid_mask = LogicEvent::AllEvents | LogicEvent::Edge | LogicEvent::OneShot;
        uint32_t invalid = static_cast<uint32_t>(events) & ~static_cast<uint32_t>(valid_mask);
        return invalid == 0;
    }
    
    LogicEvent events_;
};


} // namespace net
}
#endif