#ifndef YY_NET_POLLER_ 
#define YY_NET_POLLER_
#include <vector>
#include <type_traits>
#include <map>
#include "../Common/noncopyable.h"
#include "../Common/TimeStamp.h"
#include "EventHandler.h"
#include <memory>
// @note CRTP模型下不能创建Poller对象，只能创建子类对象，因为无法解释子类多出的内存布局
namespace yy
{
namespace net
{

/**
 * @file Poller.h
 * @brief 轮询器基类的定义
 * 
 * 本文件定义了轮询器基类，使用CRTP（奇异递归模板模式）实现。
 */

/**
 * @brief 轮询器处理器列表类型
 */
typedef std::vector<EventHandler*> PollerHandlerList;

/**
 * @brief 轮询器基类
 * 
 * 模板类，使用CRTP（奇异递归模板模式）实现。
 * 不能直接创建Poller对象，只能创建子类对象。
 * 
 * @tparam PollerTag 轮询器标签类型
 */
template<class PollerTag>
class Poller:noncopyable
{
public:
    /**
     * @brief 处理器列表类型
     */
    typedef PollerHandlerList HandlerList;
    
    /**
     * @brief 轮询事件
     * 
     * @param timeout 超时时间（毫秒）
     * @param handler 处理器列表
     */
    void poll(int timeout,HandlerList& handler)
    {
        static_assert(has_poll_v<PollerTag>,"成员不存在");
        return static_cast<PollerTag*>(this).poll(timeout,handler);
    }
    
    /**
     * @brief 析构函数
     */
    ~Poller()=default;
    
    /**
     * @brief 添加监听
     * 
     * @param handler 事件处理器
     */
    void add_listen(EventHandler* handler)
    {
        static_assert(has_add_listen_v<PollerTag>,"成员不存在");
        return static_cast<PollerTag*>(this).add_listen(handler);
    }
    
    /**
     * @brief 更新监听
     * 
     * @param handler 事件处理器
     */
    void update_listen(EventHandler* handler)
    {
        static_assert(has_update_listen_v<PollerTag>,"成员不存在");
        return static_cast<PollerTag*>(this).update_listen(handler);
    }
    
    /**
     * @brief 移除监听
     * 
     * @param handler 事件处理器
     */
    void remove_listen(EventHandler* handler)
    {
        static_assert(has_remove_listen_v<PollerTag>,"成员不存在");
        return static_cast<PollerTag*>(this).remove_listen(handler);
    }

  
protected:    
    /**
     * @brief 处理器映射
     * 
     * 选择指针存储，1.避免拷贝开销，2.EventHandler拒绝拷贝
     */
    std::map<int,EventHandler*> handlers_;
    
    /**
     * @brief 构造函数
     */
    Poller()=default; 

    /**
     * @brief 获取事件处理器
     * 
     * @param fd 文件描述符
     * @return EventHandler* 事件处理器
     */
    EventHandler* get_event_handler(int fd);

    /**
     * @brief 添加处理器
     * 
     * @param handler 事件处理器
     */
    void add_handler(EventHandler* handler);

    /**
     * @brief 移除处理器
     * 
     * @param handler 事件处理器
     */
    void remove_handler(EventHandler* handler);

    /**
     * @brief 检查是否有处理器
     * 
     * @param event_handler 事件处理器
     * @return bool 是否有处理器
     */
    bool has_handler(EventHandler* event_handler);

private:

    /**
     * @brief 检查是否有poll成员函数
     * 
     * @tparam T 类型
     * @param int 占位参数
     * @return decltype 检查结果
     */
    template<class T>
    static auto has_poll(int)->decltype
    (
        std::declval<T>().poll(
            std::declval<int>(),
            std::declval<std::vector<EventHandler>&>()
        ),
        std::true_type{}
    );
    
    /**
     * @brief 检查是否有poll成员函数（重载）
     * 
     * @tparam T 类型
     * @param ... 占位参数
     * @return decltype 检查结果
     */
    template<class T>
    static auto has_poll(...)->decltype
    (
        std::false_type{}
    );

    /**
     * @brief 检查是否有remove_listen成员函数
     * 
     * @tparam T 类型
     * @param int 占位参数
     * @return decltype 检查结果
     */
    template<class T>
    static auto has_remove_listen(int)->decltype
    (
        std::declval<T>().remove_listen(
            std::declval<EventHandler&>()
        ),
        std::true_type{}
    );
    
    /**
     * @brief 检查是否有remove_listen成员函数（重载）
     * 
     * @tparam T 类型
     * @param ... 占位参数
     * @return decltype 检查结果
     */
    template<class T>
    static auto has_remove_listen(...)->decltype
    (
        std::false_type{}
    );    
    
    /**
     * @brief 检查是否有add_listen成员函数
     * 
     * @tparam T 类型
     * @param int 占位参数
     * @return decltype 检查结果
     */
    template<class T>
    static auto has_add_listen(int)->decltype
    (
        std::declval<T>().add_listen(
            std::declval<EventHandler&>()
        ),
        std::true_type{}
    );
    
    /**
     * @brief 检查是否有add_listen成员函数（重载）
     * 
     * @tparam T 类型
     * @param ... 占位参数
     * @return decltype 检查结果
     */
    template<class T>
    static auto has_add_listen(...)->decltype
    (
        std::false_type{}
    );
    
    /**
     * @brief 检查是否有update_listen成员函数
     * 
     * @tparam T 类型
     * @param int 占位参数
     * @return decltype 检查结果
     */
    template<class T>
    static auto has_update_listen(int)->decltype
    (
        std::declval<T>().update_listen(
            std::declval<EventHandler&>()
        ),
        std::true_type{}
    );
    
    /**
     * @brief 检查是否有update_listen成员函数（重载）
     * 
     * @tparam T 类型
     * @param ... 占位参数
     * @return decltype 检查结果
     */
    template<class T>
    static auto has_update_listen(...)->decltype
    (
        std::false_type{}
    );
public:
    /**
     * @brief 是否有poll成员函数
     */
    template<class T>
    constexpr static bool has_poll_v=decltype(has_poll<T>(0))::value;  
    
    /**
     * @brief 是否有remove_listen成员函数
     */
    template<class T>
    constexpr static bool has_remove_listen_v=decltype(has_remove_listen<T>(0))::value;
    
    /**
     * @brief 是否有add_listen成员函数
     */
    template<class T>
    constexpr static bool has_add_listen_v=decltype(has_add_listen<T>(0))::value;
    
    /**
     * @brief 是否有update_listen成员函数
     */
    template<class T>
    constexpr static bool has_update_listen_v=decltype(has_update_listen<T>(0))::value;
};


template<class PollerTag>
EventHandler* Poller<PollerTag>::get_event_handler(int fd)
{
    auto it=handlers_.find(fd);
    assert(it!=handlers_.end()&&"监听的套接字不存在");
    auto handler=it->second;
    assert(handler->fd()==fd&&"奇怪的套接字");
    return handler;
}


template<class PollerTag>
void Poller<PollerTag>::add_handler(EventHandler* handler)
{
    assert(handlers_.find(handler->fd())==handlers_.end());
    handlers_[handler->fd()]=handler;
}


template<class PollerTag>
void Poller<PollerTag>::remove_handler(EventHandler* handler)
{
    assert(handlers_.find(handler->fd())!=handlers_.end());
#ifndef NDEBUG
    size_t n=handlers_.erase(handler->fd());
    assert(n==1);        
#else
    handlers_.erase(handler->fd());  
#endif
}


template<class PollerTag>
bool Poller<PollerTag>::has_handler(EventHandler* event_handler)
{
    auto it=handlers_.find(event_handler->fd());    
    return it!=handlers_.end();
}
}    
}



#endif

 
