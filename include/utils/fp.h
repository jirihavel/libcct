#ifndef FUNCTIONAL_PROGRAMMING_H_INCLUDED
#define FUNCTIONAL_PROGRAMMING_H_INCLUDED

#include <cmath>   // sqrt
#include <cstddef> // nullptr_t

#include <type_traits> // integral_constant
#include <utility>     // move, forward

#include <boost/assert.hpp>

namespace utils {
namespace fp {

struct ignore
{
    template<typename ... Args>
    void operator()(Args && ...) const
    {
    }
};

struct identity
{
    template<typename X>
    X operator()(X && x) const
    {
        return std::forward<X>(x);
    }
};

template<typename Value>
class constant
{
    Value const m_value;
public :
    template<typename ... Args>
    explicit constexpr constant(Args && ... args)
        : m_value(std::forward<Args>(args)...) {}

    constant(constant const &) = default;
    constant(constant &&) = default;
    constant & operator=(constant const &) = default;
    constant & operator=(constant &&) = default;

    template<typename ... Args>
    constexpr Value operator()(Args && ...) const
    {
        return m_value;
    }
};

template<>
struct constant<std::nullptr_t>
{
    template<typename ... Args>
    constexpr std::nullptr_t operator()(Args && ...) const noexcept
    {
        return nullptr;
    }
};

template<typename T, T v>
struct constant<std::integral_constant<T, v>>
    : public std::integral_constant<T, v>
{
    template<typename ... Args>
    constexpr T operator()(Args && ...) const noexcept
    {
        return v;
    }
};

using constant_null = constant<std::nullptr_t>;

using constant_true  = constant<std::true_type >;
using constant_false = constant<std::false_type>;

template<int N>
using constant_int = constant<std::integral_constant<int, N>>;

// Math stuff

struct sqr
{
    template<typename X>
    X operator()(X const & x) const
    {
        return x*x;
    }
};

struct sqrt
{
    template<typename X>
    X operator()(X const & x) const
    {
        return std::sqrt(x);
    }
};

}//namespace fp

template<typename T>
class assumption
    : private fp::constant<T>
{
    using base = fp::constant<T>;
public :
    assumption(T const & y)
        : base(y) {}

    template<typename X>
    X const & operator=(X const & x)
    {
        BOOST_ASSERT(base::operator()() == x);
        return x;
    }
};

template<typename T>
assumption<T> assume(T const & x)
{
    return assumption<T>(x);
}

}//namespace utils

#endif//FUNCTIONAL_PROGRAMMING_H_INCLUDED
