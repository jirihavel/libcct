#ifndef UTILS_OPENCV_H_INCLUDED
#define UTILS_OPENCV_H_INCLUDED

#include <utility>

#include <opencv2/core/core.hpp>

namespace cv {

template<typename T, typename U>
cv::Size_<T> size_cast(cv::Size_<U> const & s)
{
    return cv::Size_<T>(s.width, s.height);
}

namespace detail {

template<template<typename, int> class F, typename C, typename ... Args>
auto pixelTypeSwitchImpl(cv::Mat const & image, Args && ... args)
{
    if(image.channels() <= 0)
    {
        throw std::runtime_error("Channel count must be positive");
    }
    switch(image.channels())
    {
        case 1 : return F<C, 1>()(image, std::forward<Args>(args)...); break;
        case 2 : return F<C, 2>()(image, std::forward<Args>(args)...); break;
        case 3 : return F<C, 3>()(image, std::forward<Args>(args)...); break;
        case 4 : return F<C, 4>()(image, std::forward<Args>(args)...); break;
        default : return F<C, 0>()(image, std::forward<Args>(args)...); break;
    }
}

}//namespace detail

template<template<typename, int> class F, typename ... Args>
auto pixelTypeSwitch(cv::Mat const & image, Args && ... args)
{
    switch(image.depth())
    {
        case CV_8U  : return detail::pixelTypeSwitchImpl<F, uint8_t>(image, std::forward<Args>(args)...);
        case CV_8S  : return detail::pixelTypeSwitchImpl<F,  int8_t>(image, std::forward<Args>(args)...);
        case CV_16U : return detail::pixelTypeSwitchImpl<F,uint16_t>(image, std::forward<Args>(args)...);
        case CV_16S : return detail::pixelTypeSwitchImpl<F, int16_t>(image, std::forward<Args>(args)...);
        case CV_32S : return detail::pixelTypeSwitchImpl<F, int32_t>(image, std::forward<Args>(args)...);
        case CV_32F : return detail::pixelTypeSwitchImpl<F,   float>(image, std::forward<Args>(args)...);
        case CV_64F : return detail::pixelTypeSwitchImpl<F,  double>(image, std::forward<Args>(args)...);
        default : throw std::runtime_error("Image depth not implemented");
    }
}

}//namespace cv

#endif//UTILS_OPENCV_H_INCLUDED
