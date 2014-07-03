#ifndef ABSOLUTE_DIFFERENCE_H_INCLUDED
#define ABSOLUTE_DIFFERENCE_H_INCLUDED

#include <opencv2/core/core.hpp>

namespace utils {

template<typename T>
constexpr inline T sqr(T x)
{
    return x*x;
}

template<typename T>
constexpr inline T abs_diff(T a, T b)
{
    return a>b ? a-b : b-a;
}

template<typename O, typename T, int N>
inline O l1_abs_diff(cv::Vec<T, N> const & a, cv::Vec<T, N> const & b)
{
    O y = abs_diff(a[0], b[0]);
    for(int i = 1; i < N; ++i)
    {
        y += abs_diff(a[i], b[i]);
    }
    return y;
}

template<typename O, typename T, int N>
inline O l2_abs_diff(cv::Vec<T, N> const & a, cv::Vec<T, N> const & b)
{
    O y = sqr<O>(abs_diff(a[0], b[0]));
    for(int i = 1; i < N; ++i)
    {
        y += sqr<O>(abs_diff(a[i], b[i]));
    }
    return sqrt(y);
}

template<typename T, int N>
inline T max_abs_diff(cv::Vec<T, N> const & a, cv::Vec<T, N> const & b)
{
    T y = abs_diff(a[0], b[0]);
    for(int i = 1; i < N; ++i)
    {
        y = std::max(y, abs_diff(a[i], b[i]));
    }
    return y;
}

template<typename O, typename T, int N>
class L1AbsDiff
{
    typedef cv::Mat_<cv::Vec<T,N>> Mat;

    Mat const & m_image;
public :
    L1AbsDiff(Mat const & i) : m_image(i) {}

    O operator()(cv::Point const & a, cv::Point const & b) const
    {
        return l1_abs_diff<O>(m_image(a), m_image(b));
    }
};

template<typename O, typename T, int N>
class L2AbsDiff
{
    typedef cv::Mat_<cv::Vec<T,N>> Mat;

    Mat const & m_image;
public :
    L2AbsDiff(Mat const & i) : m_image(i) {}

    O operator()(cv::Point const & a, cv::Point const & b) const
    {
        return l2_abs_diff<O>(m_image(a), m_image(b));
    }
};

template<typename O, typename T, int N>
class MaxAbsDiff
{
    typedef cv::Vec<T,N> Vec;

    cv::Mat const & m_image;
public :
    typedef O result_type;

    MaxAbsDiff(cv::Mat const & i) : m_image(i) {}

    result_type operator()(cv::Point const & a, cv::Point const & b) const
    {
        return max_abs_diff(m_image.at<Vec>(a), m_image.at<Vec>(b));
    }
};

/*
template<typename O, typename I, typename C>
class TODO
{
    typedef cv::Vec<T,N> Vec;

    cv::Mat const & m_image;
public :
    typedef typename O::result_type result_type;

//    MaxAbsDiff(cv::Mat const & i) : m_image(i) {}

    result_type operator()(cv::Point const & a, cv::Point const & b) const
    {
        return max_abs_diff(m_image.at<Vec>(a), m_image.at<Vec>(b));
    }
};
*/

}//namespace utils

#endif//ABSOLUTE_DIFFERENCE_H_INCLUDED
