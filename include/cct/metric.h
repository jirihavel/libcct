#ifndef LIBCCT_METRIC_H_INCLUDED
#define LIBCCT_METRIC_H_INCLUDED

#include <utils/fp.h>
#include <utils/ipow.h>

#include <cinttypes>
#include <cstdlib>

#include <opencv2/core/core.hpp>

namespace cct {
namespace detail {

template<bool Abs, typename T>
typename std::enable_if<std::is_signed<T>::value,T>::type opt_abs_diff(T a, T b)
{
    return Abs ? std::abs(a-b) : a-b;
}

template<bool Abs, typename T>
typename std::enable_if<std::is_unsigned<T>::value,T>::type opt_abs_diff(T a, T b)
{
    return a>b ? a-b : b-a;
}

template<size_t P>
struct lp_impl
{
    template<typename T, typename W>
    auto operator()(T const * a, T const * b, size_t n, W w)
    {
        BOOST_ASSERT(n > 0);
        auto y = w(0)*utils::ipow_<P>(opt_abs_diff<P&1>(a[0], b[0]));
        for(size_t i = 1; i < n; ++i)
        {
            y += w(i)*utils::ipow_<P>(opt_abs_diff<P&1>(a[i], b[i]));
        }
        return utils::ipow_<1,P>(y);
    }
};

template<>
struct lp_impl<0>
{
    template<typename T, typename W>
    auto operator()(T const * a, T const * b, size_t n, W w)
    {
        size_t y = 0;
        for(size_t i = 0; i < n; ++i)
        {
            if(w(i)*opt_abs_diff<false>(a[i], b[i]) > 0)
            {
                ++y;
            }
        }
        return y;
    }
};

template<>
struct lp_impl<SIZE_MAX>
{
    template<typename T, typename W>
    auto operator()(T const * a, T const * b, size_t n, W w)
    {
        BOOST_ASSERT(n > 0);
        auto y = w(0)*opt_abs_diff<true>(a[0], b[0]);
        for(size_t i = 1; i < n; ++i)
        {
            y = std::max(y, w(i)*opt_abs_diff<true>(a[i], b[i]));
        }
        return y;
    }
};

}//namespace detail

template<typename T>
T abs_diff(T a, T b)
{
    return detail::opt_abs_diff<true>(a, b);
}

namespace metric {

template<size_t P, typename T, typename W = utils::fp::constant_int<1>>
auto lp_(T const * a, T const * b, size_t n, W w = W())
{
    return detail::lp_impl<P>()(a, b, n, std::move(w));
}

template<size_t P, typename C, int N, typename W = utils::fp::constant_int<1>>
class Lp
{
    static_assert(N > 0, "Invalid number of channels!");
    using Mat = cv::Mat_<cv::Vec<C,N>>;
public :
    Lp(Mat const & i) : m_image(i) {}

    template<typename T>
    auto operator()(cv::Point_<T> const & a, cv::Point_<T> const & b) const
    {
        return lp_<P>(&m_image(a)[0], &m_image(b)[0], N);
    }
private :
    Mat m_image;
};

template<size_t P, typename C, typename W>
class Lp<P, C, 0, W>
{
    using Mat = cv::Mat;
public :
    Lp(Mat const & i) : m_image(i) {}

    template<typename T>
    auto operator()(cv::Point_<T> const & a, cv::Point_<T> const & b) const
    {
        return lp_<P, C>(
            reinterpret_cast<C const*>(m_image.data + m_image.step[0]*a.y + m_image.step[1]*a.x),
            reinterpret_cast<C const*>(m_image.data + m_image.step[0]*b.y + m_image.step[1]*b.x),
            m_image.channels());
    }
private :
    Mat m_image;
};

template<typename C, int N>
using L1 = Lp<1, C, N>;

template<typename C, int N>
using L2 = Lp<2, C, N>;

template<typename C, int N>
using LInf = Lp<SIZE_MAX, C, N>;

template<typename C, int N = 1>
class Min;

template<typename C, int N = 1>
class Max;

template<typename C>
class Min<C, 1>

{
    using Mat = cv::Mat_<C>;
public :
    Min(Mat const & i) : m_image(i) {}

    template<typename T>
    auto operator()(cv::Point_<T> const & a) const
    {
        return m_image(a);
    }
    template<typename T>
    auto operator()(cv::Point_<T> const & a, cv::Point_<T> const & b) const
    {
        return std::min(m_image(a), m_image(b));
    }
private :
    Mat m_image;
};

template<typename C>
class Max<C, 1>
{
    using Mat = cv::Mat_<C>;
public :
    Max(Mat const & i) : m_image(i) {}

    template<typename T>
    auto operator()(cv::Point_<T> const & a) const
    {
        return m_image(a);
    }
    template<typename T>
    auto operator()(cv::Point_<T> const & a, cv::Point_<T> const & b) const
    {
        return std::max(m_image(a), m_image(b));
    }
private :
    Mat m_image;
};

}//namespace metric
}//namespace cct

#endif//LIBCCT_METRIC_H_INCLUDED
