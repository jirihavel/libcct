#ifndef FUNCTIONAL_PROGRAMMING_H_INCLUDED
#define FUNCTIONAL_PROGRAMMING_H_INCLUDED

#include <cmath> // std::sqrt

#include <type_traits> // std::integral_constant
#include <utility> // std::move, std::forward

namespace utils {

namespace fp {

struct ignore
{
    template<typename ... Args>
    void operator()(Args && ...) const
    {
    }
};

template<typename Value>
class constant
{
    Value const m_value;
public :
    template<typename ... Args>
    explicit constant(Args && ... args)
        : m_value(std::forward<Args>(args)...)
    {
    }

    constant(constant const &) = default;
    constant(constant &&) = default;
    constant & operator=(constant const &) = default;
    constant & operator=(constant &&) = default;

    template<typename ... Args>
    Value operator()(Args && ...) const
    {
        return m_value;
    }
};

template<typename T, T v>
struct constant<std::integral_constant<T, v>>
    : public std::integral_constant<T, v>
{
    template<typename ... Args>
    T operator()(Args && ...) const noexcept
    {
        return v;
    }
};

typedef constant<std::true_type > constant_true;
typedef constant<std::false_type> constant_false;

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

}//namespace utils

#endif//FUNCTIONAL_PROGRAMMING_H_INCLUDED
