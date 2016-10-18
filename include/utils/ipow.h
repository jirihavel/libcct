#ifndef UTILS_INTEGRAL_POWER_H_INCLUDED
#define UTILS_INTEGRAL_POWER_H_INCLUDED

#include <cmath>
#include <cstdint>
#include <cstdlib>

#include <ratio>

namespace utils {

/** \defgroup InverseUtil Multiplicative inverse similar to cmath functions like abs or sqrt.
 * @{
 */
    
inline constexpr float       inv(float       x) noexcept { return 1/x; }
inline constexpr double      inv(double      x) noexcept { return 1/x; }
inline constexpr long double inv(long double x) noexcept { return 1/x; }

template<typename T>
inline constexpr typename std::enable_if<
    std::is_integral<T>::value,double>::type inv(T x) noexcept
{
    // all integral types are converted to double
    return inv(static_cast<double>(x));
}

/** @} */

namespace detail {

template<typename T>
using corresponding_float = decltype(inv(T()));
    
template<uintmax_t N, uintmax_t D>
struct ipow_impl
{
    template<typename T>
    auto operator()(T const & x) const
    {
        return std::pow(x, static_cast<corresponding_float<T>>(N)/D);
    }
};

template<uintmax_t N>
struct ipow_impl<N,2>
{
    template<typename T>
    auto operator()(T const & x) const
    {
        return std::sqrt(ipow_impl<N,1>()(x));
    }
};

template<uintmax_t N>
struct ipow_impl<N,3>
{
    template<typename T>
    auto operator()(T const & x) const
    {
        return std::cbrt(ipow_impl<N,1>()(x));
    }
};

template<uintmax_t N>
struct ipow_impl<N,1>
{
    template<typename T>
    auto operator()(T const & x) const
    {
        auto const aux = ipow_impl<N/2,1>()(x);
        return N%2 ? aux*aux*x : aux*aux;
    }
};

template<>
struct ipow_impl<0,1>
{
    template<typename T>
    auto operator()(T const &) const
    {
        return 1;
    }
};

template<bool I>
struct opt_inv
{
    template<typename T>
    T operator()(T const & x) const
    {
        return x;
    }
};

template<>
struct opt_inv<true>
{
    template<typename T>
    auto operator()(T const & x) const
    {
        return inv(x);
    }
};

}//namespace detail

/** \brief Power to the compile time rational number.
 *
 * \returns x^(N/D)
 *
 * Return type is T if D==1 and N >= 0. Otherwise float or [long] double is returned.
 */
template<intmax_t N, intmax_t D = 1, typename T>
auto ipow_(T const & x)
{
    using r = std::ratio<N,D>;//use std::ratio for normalization
    static_assert(r::num > INTMAX_MIN, "Absolute value is undefined");
    auto const aux = detail::ipow_impl<(r::num<0?-r::num:r::den),r::den>()(x);
    return detail::opt_inv<(r::num<0)>()(aux);
}

template<typename T>
auto sqr(T const & x)
{
    return ipow_<2>(x);
}

template<typename T>
T ipow(T const & base, uintmax_t exp)
{
    T result = 1;
    while(exp)
    {
        if(exp&1 != 0)
            result *= base;
        exp >>= 1;
        base *= base;
    }
    return result;
}

}//namespace utils

#endif//UTILS_INTEGRAL_POWER_H_INCLUDED
