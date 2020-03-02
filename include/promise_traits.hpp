#pragma once

#include <type_traits>

struct undefined
{
};

template <class>
class Promise;

template <class>
struct is_promise : std::false_type
{
};

template <class T>
struct is_promise<Promise<T>> : std::true_type
{
};

template <class T>
inline constexpr bool is_promise_v = is_promise<T>::value;

template <class>
struct unwrap_promise
{
};

template <class T>
struct unwrap_promise<Promise<T>>
{
    using type = std::conditional_t<std::is_void_v<T>, undefined, T>;
};

template <class T>
using unwrap_promise_t = typename unwrap_promise<T>::type;

template <class Func, class... Args>
auto return_undefined_if_void(Func &&f, Args &&... args)
{
    if constexpr (std::is_void_v<std::invoke_result_t<Func, Args...>>)
    {
        std::forward<Func>(f)(std::forward<Args>(args)...);
        return undefined{};
    }
    else
    {
        return std::forward<Func>(f)(std::forward<Args>(args)...);
    }
}
