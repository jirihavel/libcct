#ifndef UTILS_UINT_CONV_H_INCLUDED
#define UTILS_UINT_CONV_H_INCLUDED

#include <utils/pun.h>

namespace utils {

template<typename T>
struct uint_conv
{
    using result_type = void;
    T operator()(T x) const noexcept
    {
        return x;
    }
};

namespace detail {

template<typename U>
struct uint_conv_uint
{
    using uint_type = U;
    U operator()(U x) const noexcept
    {
        return x;
    }
};
/*
template<typename U>
struct uint_conv_sint
{
    static_constexpr bool is_specialized = true;
    using uint_type = U;
    U operator()(U x) const noexcept
    {
        return x;
    }
};
*/
}//namespace detail

template<>
struct uint_conv<unsigned char>
    : public detail::uint_conv_uint<unsigned char> {};
template<>
struct uint_conv<unsigned short>
    : public detail::uint_conv_uint<unsigned short> {};
template<>
struct uint_conv<unsigned>
    : public detail::uint_conv_uint<unsigned> {};
template<>
struct uint_conv<unsigned long>
    : public detail::uint_conv_uint<unsigned long> {};
template<>
struct uint_conv<unsigned long long>
    : public detail::uint_conv_uint<unsigned long long> {};

template<>
struct uint_conv<float>
{
    using uint_type = uint32_t;
    static uint32_t conv(float x)
    {
        return pun::f2u_ord(x);
    }
};

}//namespace utils

#endif//UTILS_UINT_CONV_H_INCLUDED
