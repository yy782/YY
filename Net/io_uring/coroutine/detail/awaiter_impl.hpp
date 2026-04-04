
#ifndef XYNET_COROUTINE_DETAIL_IS_AWAITER_H
#define XYNET_COROUTINE_DETAIL_IS_AWAITER_H
#include <type_traits>
#include <coroutine>

namespace yy
{
namespace net
{
#include <concepts>
#include <coroutine>
#include <type_traits>

namespace detail {

// 检测 await_suspend 返回值是否合法
template<typename Return>
concept valid_await_suspend_return = 
    std::is_void_v<Return> ||
    std::same_as<Return, bool> ||
    requires { typename std::coroutine_handle<std::coroutine_traits<Return>>; };

} // namespace detail

// 1. Awaiter 概念：检查三个必需方法
template<typename T>
concept Awaiter = requires(T&& value, std::coroutine_handle<> h) {
    // await_ready 必须返回 bool
    { value.await_ready() } -> std::convertible_to<bool>;
    
    // await_suspend 返回值必须合法（void/bool/coroutine_handle）
    { value.await_suspend(h) } -> detail::valid_await_suspend_return;
    
    // await_resume 存在即可
    value.await_resume();
};

// 2. 获取 Awaiter 的重载（用概念替代 SFINAE）

// 重载1：有 operator co_await
template<typename T>
requires requires(T&& value) { static_cast<T&&>(value).operator co_await(); }
auto get_awaiter_impl(T&& value, int)
    noexcept(noexcept(static_cast<T&&>(value).operator co_await()))
    -> decltype(static_cast<T&&>(value).operator co_await())
{
    return static_cast<T&&>(value).operator co_await();
}

// 重载2：有全局 operator co_await
template<typename T>
requires requires(T&& value) { operator co_await(static_cast<T&&>(value)); }
auto get_awaiter_impl(T&& value, long)
    noexcept(noexcept(operator co_await(static_cast<T&&>(value))))
    -> decltype(operator co_await(static_cast<T&&>(value)))
{
    return operator co_await(static_cast<T&&>(value));
}

// 重载3：本身已经是 Awaiter
template<typename T>
requires Awaiter<T&&>
auto get_awaiter_impl(T&& value, ...) noexcept
    -> T&&
{
    return static_cast<T&&>(value);
}

// 3. 主函数
template<typename T>
auto get_awaiter(T&& value)
    noexcept(noexcept(detail::get_awaiter_impl(static_cast<T&&>(value), 123)))
    -> decltype(detail::get_awaiter_impl(static_cast<T&&>(value), 123))
{
    return detail::get_awaiter_impl(static_cast<T&&>(value), 123);
}

}
}

#endif