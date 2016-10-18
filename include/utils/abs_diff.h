#ifndef ABSOLUTE_DIFFERENCE_H_INCLUDED
#define ABSOLUTE_DIFFERENCE_H_INCLUDED

#include "fp.h"
#include "ipow.h"

#include <cinttypes>
#include <cstdlib>

#include <opencv2/core/core.hpp>

namespace utils {
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
        auto y = w(0)*ipow_<P>(opt_abs_diff<P&1>(a[0], b[0]));
        for(size_t i = 1; i < n; ++i)
        {
            y += w(i)*ipow_<P>(opt_abs_diff<P&1>(a[i], b[i]));
        }
        return ipow_<1,P>(y);
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

template<size_t P, typename T, typename W = fp::constant_int<1>>
auto lp_(T const * a, T const * b, size_t n, W w = W())
{
    return detail::lp_impl<P>()(a, b, n, std::move(w));
}

template<size_t P, typename C, int N, typename W = fp::constant_int<1>>
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

}//namespace metric
#if 0
template<typename O, typename T, typename W = fp::constant_int<1>>
inline O l1(T const * a, T const * b, int n, W w = W())
{
    /*BOOST_ASSERT(n > 0);
    O y = w(0)*abs_diff(a[0], b[0]);
    for(int i = 1; i < n; ++i)
    {
        y += w(i)*abs_diff(a[i], b[i]);
    }
    return y;*/
    return lp_<1>(a, b, n, std::move(w));
}

template<typename O, typename T, int N, typename W = fp::constant_int<1>>
inline O l1(cv::Vec<T, N> const & a, cv::Vec<T, N> const & b, W w = W())
{
    return l1<O>(&a[0], &b[0], N, std::move(w));
/*    O y = abs_diff(a[0], b[0]);
    for(int i = 1; i < N; ++i)
    {
        y += abs_diff(a[i], b[i]);
    }
    return y;*/
}

// --

template<typename O, typename T, typename W = fp::constant_int<1>>
inline O l2(T const * a, T const * b, int n, W w = W())
{
    return lp_<2>(a, b, n, std::move(w));
    /*BOOST_ASSERT(n > 0);
    O y = sqr<O>(w(0)*abs_diff(a[0], b[0]));
    for(int i = 1; i < n; ++i)
    {
        y += sqr<O>(w(i)*abs_diff(a[i], b[i]));
    }
    return sqrt(y);*/
}

template<typename O, typename T, int N, typename W = fp::constant_int<1>>
inline O l2(cv::Vec<T, N> const & a, cv::Vec<T, N> const & b, W w = W())
{
    return l2<O>(&a[0], &b[0], N, std::move(w));
/*    O y = sqr<O>(abs_diff(a[0], b[0]));
    for(int i = 1; i < N; ++i)
    {
        y += sqr<O>(abs_diff(a[i], b[i]));
    }
    return sqrt(y);*/
}

// --

template<typename O, typename T, typename W = fp::constant_int<1>>
inline O linf(T const * a, T const * b, int n, W w = W())
{
    BOOST_ASSERT(n > 0);
    O y = w(0)*abs_diff(a[0], b[0]);
    for(int i = 1; i < n; ++i)
    {
        y = std::max<O>(y, w(i)*abs_diff(a[i], b[i]));
    }
    return y;
}

template<typename O, typename T, int N, typename W = fp::constant_int<1>>
inline O linf(cv::Vec<T, N> const & a, cv::Vec<T, N> const & b, W w = W())
{
    return linf<O>(&a[0], &b[0], N, std::move(w));
/*    T y = abs_diff(a[0], b[0]);
    for(int i = 1; i < N; ++i)
    {
        y = std::max(y, abs_diff(a[i], b[i]));
    }
    return y;*/
}

//template<typename O, typename T, typename W

}//namespace metric

template<typename O, typename T, int N>
class L1
{
    static_assert(N > 0, "Invalid number of channels!");

    typedef cv::Mat_<cv::Vec<T,N>> Mat;

    Mat const & m_image;
public :
    L1(Mat const & i) : m_image(i) {}

    O operator()(cv::Point const & a, cv::Point const & b) const
    {
        return metric::l1<O>(m_image(a), m_image(b));
    }
};

template<typename O, typename T>
class L1<O, T, 0>
{
    typedef cv::Mat Mat;

    Mat const & m_image;
public :
    L1(Mat const & i) : m_image(i) {}

    O operator()(cv::Point const & a, cv::Point const & b) const
    {
        return metric::l1<O>(
            m_image.data + m_image.step[0]*a.y + m_image.step[1]*a.x,
            m_image.data + m_image.step[0]*b.y + m_image.step[1]*b.x,
            m_image.channels());
    }
};

template<typename O, typename T, int N>
class L2
{
    static_assert(N > 0, "Invalid number of channels!");

    typedef cv::Mat_<cv::Vec<T,N>> Mat;

    Mat const & m_image;
public :
    L2(Mat const & i) : m_image(i) {}

    O operator()(cv::Point const & a, cv::Point const & b) const
    {
        return metric::l2<O>(m_image(a), m_image(b));
    }
};

template<typename O, typename T, int N>
class LInf
{
    static_assert(N > 0, "Invalid number of channels!");

    typedef cv::Mat_<cv::Vec<T,N>> Mat;

    Mat const & m_image;
public :
    LInf(Mat const & i) : m_image(i) {}

    O operator()(cv::Point const & a, cv::Point const & b) const
    {
        return metric::linf<O>(m_image(a), m_image(b));
    }
};

template<typename O, typename T>
class LInf<O, T, 0>
{
    typedef cv::Mat Mat;

    Mat const & m_image;
public :
    LInf(Mat const & i) : m_image(i) {}

    O operator()(cv::Point const & a, cv::Point const & b) const
    {
        return metric::linf<O>(
            m_image.data + m_image.step[0]*a.y + m_image.step[1]*a.x,
            m_image.data + m_image.step[0]*b.y + m_image.step[1]*b.x,
            m_image.channels());
    }
};
#endif
}//namespace utils

#endif//ABSOLUTE_DIFFERENCE_H_INCLUDED
