#ifndef YY_NET_EVENTHANDLER_
#define YY_NET_EVENTHANDLER_

#include <functional>
#include <memory>
#include <string>

#include "Event.h"
#include "../Common/noncopyable.h"
#include "../Common/TimeStamp.h"
#include "sockets.h"

namespace yy
{
namespace net
{

class EventLoop;


/**
 * @brief 事件处理器类，负责处理文件描述符上的事件
 * 
 * EventHandler封装了文件描述符及其相关的事件处理逻辑。
 * 它负责注册、更新和处理文件描述符上的事件，并在事件发生时调用相应的回调函数。
 */
class EventHandler:public noncopyable
{
public:
    /**
     * @brief 事件回调函数类型
     */
    typedef std::function<void()> EventCallBack;

    /**
     * @brief 读事件回调函数类型
     */
    typedef EventCallBack ReadCallBack;
    /**
     * @brief 写事件回调函数类型
     */
    typedef EventCallBack WriteCallBack;
    /**
     * @brief 关闭事件回调函数类型
     */
    typedef EventCallBack CloseCallBack;
    /**
     * @brief 错误事件回调函数类型
     */
    typedef EventCallBack ErrorCallBack;
    /**
     * @brief 异常事件回调函数类型
     */
    typedef EventCallBack ExceptCallBack;
    
    /**
     * @brief 默认构造函数
     */
    EventHandler()=default;
    
    /**
     * @brief 构造函数
     * 
     * @param fd 文件描述符
     * @param loop 事件循环
     * @param name 事件处理器名称
     */
    EventHandler(int fd,EventLoop* loop,const  char* name);
    
    /**
     * @brief 析构函数
     */
    ~EventHandler()=default;
    
    /**
     * @brief 初始化事件处理器
     * 
     * @param fd 文件描述符
     * @param loop 事件循环
     * @param name 事件处理器名称
     */
    void init(int fd,EventLoop* loop,const char* name);
    
    /**
     * @brief 绑定对象的生命周期
     * 
     * @param obj 要绑定的对象
     * 
     * 绑定一个对象的生命周期，当对象被销毁时，事件处理器会自动移除。
     */
    void tie(const std::shared_ptr<void>&);

    /**
     * @brief 获取文件描述符
     * 
     * @return int 文件描述符
     */
    int fd() const noexcept{return fd_;}
    
    /**
     * @brief 获取事件循环
     * 
     * @return EventLoop* 事件循环
     */
    EventLoop* loop() const noexcept{return loop_;}
    
    /**
     * @brief 获取要监听的事件
     * 
     * @return Event 要监听的事件
     */
    Event event() const noexcept{return events_;}
    
    /**
     * @brief 获取状态
     * 
     * @return int 状态
     */
    int status() const noexcept{return status_;}
    
    /**
     * @brief 获取事件处理器名称
     * 
     * @return const std::string& 事件处理器名称
     */
    const std::string& printName()
    {
        if(name_.empty())
        {
            static const std::string NoName="NoName";
            return NoName;
        }
        return name_;
    }

    /**
     * @brief 设置要监听的事件
     * 
     * @param event 要监听的事件
     */
    void set_event(Event event)
    {
        events_=event;
        update();
    }
    
    /**
     * @brief 设置触发的事件
     * 
     * @param event 触发的事件
     */
    void set_revent(Event event) noexcept{revents_=event;}
    
    /**
     * @brief 设置状态
     * 
     * @param status 状态
     */
    void set_status(int status) noexcept{status_=status;}

    /**
     * @brief 检查是否监听写事件
     * 
     * @return bool 是否监听写事件
     */
    bool isWriting()const noexcept{return events_.has(LogicEvent::Write);}
    
    /**
     * @brief 检查是否监听读事件
     * 
     * @return bool 是否监听读事件
     */
    bool isReading()const noexcept{return events_.has(LogicEvent::Read);}
    
    /**
     * @brief 检查是否监听异常事件
     * 
     * @return bool 是否监听异常事件
     */
    bool isExcept()const noexcept{return events_.has(LogicEvent::Except);}
    
    /**
     * @brief 检查是否同时监听读事件和异常事件
     * 
     * @return bool 是否同时监听读事件和异常事件
     */
    bool isReadingAndExcept()const noexcept{return events_&(LogicEvent::Read|LogicEvent::Except);}

    /**
     * @brief 设置监听读事件
     */
    void setReading(){events_.add(LogicEvent::Read);update();}
    
    /**
     * @brief 取消监听读事件
     */
    void cancelReading(){events_.remove(LogicEvent::Read);update();}
    
    /**
     * @brief 设置监听写事件
     */
    void setWriting(){events_.add(LogicEvent::Write);update();}
    
    /**
     * @brief 取消监听写事件
     */
    void cancelWriting(){events_.remove(LogicEvent::Write);update();}
    
    /**
     * @brief 设置监听异常事件
     */
    void setExcept(){events_.add(LogicEvent::Except);update();}
    
    /**
     * @brief 取消监听异常事件
     */
    void cancelExcept(){events_.remove(LogicEvent::Except);update();}
    
    /**
     * @brief 设置同时监听读事件和异常事件
     */
    void setReadingAndExcept()
    {
        setExcept();
        setReading();
    }
    
    /**
     * @brief 取消同时监听读事件和异常事件
     */
    void cancelReadingAndExcept()
    {
        cancelExcept();
        cancelReading();
    }
    
    /**
     * @brief 禁用所有事件
     */
    void disableAll()
    {
        events_=LogicEvent::None;
        update();
    }

    /**
     * @brief 设置读事件回调函数
     * 
     * @tparam Callable 可调用对象类型
     * @param cb 读事件回调函数
     */
    template<typename Callable>
    void setReadCallBack(Callable&& cb) { 
        readCallback_ = std::forward<Callable>(cb);
    }

    /**
     * @brief 设置写事件回调函数
     * 
     * @tparam Callable 可调用对象类型
     * @param cb 写事件回调函数
     */
    template<typename Callable>
    void setWriteCallBack(Callable&& cb) { 
        writeCallback_ = std::forward<Callable>(cb);
    }

    /**
     * @brief 设置关闭事件回调函数
     * 
     * @tparam Callable 可调用对象类型
     * @param cb 关闭事件回调函数
     */
    template<typename Callable>
    void setCloseCallBack(Callable&& cb) { 
        closeCallback_ = std::forward<Callable>(cb);
    }

    /**
     * @brief 设置错误事件回调函数
     * 
     * @tparam Callable 可调用对象类型
     * @param cb 错误事件回调函数
     */
    template<typename Callable>
    void setErrorCallBack(Callable&& cb) { 
        errorCallback_ = std::forward<Callable>(cb);
    }

    /**
     * @brief 设置异常事件回调函数
     * 
     * @tparam Callable 可调用对象类型
     * @param cb 异常事件回调函数
     */
    template<typename Callable>
    void setExceptCallBack(Callable&& cb) { 
        exceptCallback_ = std::forward<Callable>(cb);
    }

    /**
     * @brief 处理触发的事件
     * 
     * 根据触发的事件类型，调用相应的回调函数。
     */
    void handler_revent();
    
    /**
     * @brief 移除事件监听器
     * 
     * 从事件循环中移除事件监听器。
     */
    void removeListen();



private:
    /**
     * @brief 更新事件监听器
     * 
     * 更新事件处理器在事件监听器中的状态。
     */
    void update();
    
    /**
     * @brief 状态
     * 
     * 这个状态是关联事件监听器Poller的状态，含义由事件监听器解释。
     */
    int status_={-1};
    
    /**
     * @brief 绑定的对象
     */
    std::weak_ptr<void> tie_;
    
    /**
     * @brief 是否绑定了对象
     */
    bool tied_={false};
    
    /**
     * @brief 文件描述符
     */
    int fd_={-1};
    
    /**
     * @brief 要监听的事件
     */
    Event events_;
    
    /**
     * @brief 触发的事件
     */
    Event revents_;
    
    /**
     * @brief 事件循环
     */
    EventLoop* loop_;
    
    /**
     * @brief 事件处理器名称
     */
    std::string name_;
    
    /**
     * @brief 读事件回调函数
     */
    ReadCallBack readCallback_;
    
    /**
     * @brief 写事件回调函数
     */
    WriteCallBack writeCallback_;
    
    /**
     * @brief 错误事件回调函数
     */
    ErrorCallBack errorCallback_;
    
    /**
     * @brief 异常事件回调函数
     */
    ExceptCallBack exceptCallback_;

    /**
     * @brief 关闭事件回调函数
     */
    CloseCallBack closeCallback_;
};
}    
}

#endif