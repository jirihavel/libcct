#ifndef CONNECTED_COMPONENT_TREE_IMAGE_GRAPH_H_INCLUDED
#define CONNECTED_COMPONENT_TREE_IMAGE_GRAPH_H_INCLUDED

#include "utils/fp.h"

#include <array>

#include <boost/optional.hpp>

#include <opencv2/core/core.hpp>

namespace cct {

namespace image {

// 1D indices for image points

template<typename T>
inline T pointId(cv::Point_<T> const & p, cv::Size_<T> const & s)
{
    BOOST_ASSERT(p.x >= T(0));
    BOOST_ASSERT(p.y >= T(0));
    BOOST_ASSERT(p.x < s.width);
    BOOST_ASSERT(p.y < s.height);
    return p.x + p.y*s.width;
} 

template<typename T>
inline T pointId(cv::Point_<T> const & p, cv::Rect_<T> const & r)
{
    BOOST_ASSERT(r.contains(p));
    return p.x-r.x + (p.y-r.y)*r.width;
} 

/** \brief Variants of image connectivity
 */
enum class Connectivity
{
    C4 = 0,
    C6P = 1,
    C6N = 2,
    C8 = C6P | C6N
};

// Image graph properties

template<typename T>
inline T vertexCount(const cv::Size_<T> & size)
{
    BOOST_ASSERT(size.width  >= T(0));
    BOOST_ASSERT(size.height >= T(0));
    return size.width*size.height;
}

template<typename T>
inline T edgeCount(const cv::Size_<T> & size, Connectivity connectivity = Connectivity::C4)
{
    BOOST_ASSERT(size.width  >= T(0));
    BOOST_ASSERT(size.height >= T(0));
    if((size.width == T(0)) || (size.height == T(0)))
        return 0;
    switch(connectivity)
    {
    case Connectivity::C4 :
        return size.width*(size.height-1) + (size.width-1)*size.height;
    case Connectivity::C6N :
    case Connectivity::C6P :
        return size.width*(size.height-1) + (size.width-1)*size.height + (size.width-1)*(size.height-1);
    default :
        BOOST_ASSERT(connectivity == Connectivity::C8);
        return size.width*(size.height-1) + (size.width-1)*size.height + 2*(size.width-1)*(size.height-1);
    };
}

// Image graph iteration

/** \brief Call function for every image edge.
 * Calls f for every 4-connected edge of the input rectangle.
 * The image itself must be passed inside the functor.
 */
template<typename E, typename T>
E forEachEdge(
    cv::Rect_<T> const & rect,
    E e// f(p0, p1)
);
template<typename E, typename T>
E forEachEdge(
    cv::Rect_<T> const & rect,
    cv::Size_<T> const & tile,
    E e// f(p0, p1)
);

/** \brief Call function for every image vertex and edge.
 * Calls v for every pixel and e for every edge of the input rectangle.
 */
template<typename V, typename E, typename T>
void forEachElement(
        cv::Rect_<T> const & rect,
        V v,// v(p)
        E e // e(p0, p1)
        );

// Image graph elements

/** \brief Weighted vertex of image graph */
template<typename T, typename W>
struct Vertex
{
    typedef cv::Point_<T> Point;
    typedef W Weight;

    Point point;

    Weight weight;

    bool operator<(Vertex const & v) const
    {
        return weight < v.weight;
    }
};

/** \brief Weighted edge of image graph */
template<typename T, typename W>
struct Edge
{
    typedef cv::Point_<T> Point;
    typedef W Weight;

    Point points[2];

    Weight weight;

    bool operator<(Edge const & e) const
    {
        return weight < e.weight;
    }
};

// Extraction of graph edges

/** \brief Extracts image edges
 */
template<typename T, typename W, typename EdgeWeightFunction>
size_t getImageEdges(
    cv::Rect_<T> const & rect,
    Edge<T, W> * edges,//[edgeCount(rect.size())]
    EdgeWeightFunction e//f(cv::Point, cv::Point)
);
template<typename T, typename W, typename EdgeWeightFunction>
size_t getImageEdges(
    cv::Rect_<T> const & rect, cv::Size_<T> const & tile,
    Edge<T, W> * edges,//[edgeCount(rect.size())]
    EdgeWeightFunction e//f(cv::Point, cv::Point)
);

/** \brief Extracts image edges and sorts them
 *
 * Depending on edge weight :
 *  uint8_t - O(|E|) = O(|V|)
 *   countingsort + 2 image passes
 *  otherwise - O(|E|log|E|)
 *   std::sort + 1 image pass
 */
template<typename T, typename W, typename EdgeWeightFunction>
size_t getSortedImageEdges(
    cv::Rect_<T> const & rect, cv::Size_<T> const & tile,
    Edge<T, W> * edges,//[edgeCount(rect.size())]
    EdgeWeightFunction e//f(cv::Point, cv::Point)
);
template<typename T, typename EdgeWeightFunction>
size_t getSortedImageEdges(
    cv::Rect_<T> const & rect, cv::Size_<T> const & tile,
    Edge<T, uint8_t> * edges,//[edgeCount(rect.size())]
    EdgeWeightFunction e//f(cv::Point, cv::Point)
);

/*
template<typename T, typename EdgeWeightFunction>
size_t getSortedPixelEdges(
    cv::Rect_<T> const & rect,
    Edge<T, uint8_t> * edges,//[vertexCount(rect.size())]
    EdgeWeightFunction e//f(cv::Point)
);
*/
/*
template<typename T,
    typename VertexType, typename EdgeType,
    typename VertexWeightFunction, typename EdgeWeightFunction
    >
std::tuple<size_t, size_t> getImageGraph(
        cv::Rect_<T> const & rect,
        VertexType * vertices,//[vertexCount(rect.size())]
        EdgeType   * edges,   //[edgeCount  (rect.size())]
        VertexWeightFunction v,//f(cv::Point) -> boost::optional<???>
        EdgeWeightFunction   e //f(cv::Point, cv::Point) -> boost::optional<???>
        );
*/

/* \brief Extract horizontal edges connecting tiles.
 */
template<typename T, typename W, typename EdgeWeightFunction, typename EdgeWeightFilter = utils::fp::constant_true>
size_t getHorizontalConnectors(
    cv::Point_<T> const & point, T height,
    Edge<T, W> * edges,//[height]
    EdgeWeightFunction e,//e(cv::Point, cv::Point)
    EdgeWeightFilter f = EdgeWeightFilter()
);

/* \brief Extract vertical edges connecting tiles.
 */
template<typename T, typename W, typename EdgeWeightFunction, typename EdgeWeightFilter = utils::fp::constant_true>
size_t getVerticalConnectors(
    cv::Point_<T> const & point, T width,
    Edge<T, W> * edges,//[width]
    EdgeWeightFunction e,//e(cv::Point, cv::Point)
    EdgeWeightFilter f = EdgeWeightFilter()
);

/* \brief Extract horizontal edges connecting tiles.
 */
template<typename T, typename W, typename EdgeWeightFunction, typename EdgeWeightFilter = utils::fp::constant_true>
size_t getSortedHorizontalConnectors(
    cv::Point_<T> const & point, T height,
    Edge<T, W> * edges,//[height]
    EdgeWeightFunction e,//e(cv::Point, cv::Point)
    EdgeWeightFilter f = EdgeWeightFilter()
);

/* \brief Extract vertical edges connecting tiles.
 */
template<typename T, typename W, typename EdgeWeightFunction, typename EdgeWeightFilter = utils::fp::constant_true>
size_t getSortedVerticalConnectors(
    cv::Point_<T> const & point, T width,
    Edge<T, W> * edges,//[width]
    EdgeWeightFunction e,//e(cv::Point, cv::Point)
    EdgeWeightFilter f = EdgeWeightFilter()
);

template<typename T, typename EdgeWeightFunction, typename EdgeWeightFilter = utils::fp::constant_true>
size_t getSortedHorizontalConnectors(
    cv::Point_<T> const & point, T height,
    Edge<T, uint8_t> * edges,//[height]
    EdgeWeightFunction e,//e(cv::Point, cv::Point)
    EdgeWeightFilter f = EdgeWeightFilter()
);

template<typename T, typename EdgeWeightFunction, typename EdgeWeightFilter = utils::fp::constant_true>
size_t getSortedVerticalConnectors(
    cv::Point_<T> const & point, T width,
    Edge<T, uint8_t> * edges,//[width]
    EdgeWeightFunction e,//e(cv::Point, cv::Point)
    EdgeWeightFilter f = EdgeWeightFilter()
);

#if 0
/**
 * Dummy predicate for disabling the tests.
 */
template<typename Input>
class TruePredicate
{
public :
    typedef Input input_type;
    typedef bool result_type;
    bool operator()(typename boost::call_traits<Input>::param_type) { return true; }
};

/**
 * Image operator wraps image and the access to the pixels.
 * This class meant only as a base -> protected constructor
 */
template<typename ImageType, typename ResultType, typename PixelType>
class ImageOperator
{
public :
    typedef ResultType result_type;
protected :
    ImageOperator(ImageType & i) : image(i) {}
    typename boost::mpl::if_<boost::is_const<ImageType>, PixelType const &, PixelType &>::type
        at(cv::Point const & p) const { return image.template at<PixelType>(p); }
private :
    ImageType & image;
};

/**
 * Base class for local image operators (return value for one position in image).
 */
template<typename ImageType, typename ResultType, typename PixelType>
class UnaryImageOperator
    : public ImageOperator<ImageType, ResultType, PixelType>
{
public :
    typedef cv::Point const & argument_type;
protected :
    UnaryImageOperator(ImageType & i) : ImageOperator<ImageType, ResultType, PixelType>(i) {}
};

/**
 * BinaryImageOperator is base for operators that calculate result for a pair of image locations
 */
template<typename ImageType, typename ResultType, typename PixelType>
class BinaryImageOperator
    : public ImageOperator<ImageType, ResultType, PixelType>
{
public :
    typedef cv::Point const & first_argument_type;
    typedef cv::Point const & second_argument_type;
protected :
    BinaryImageOperator(ImageType & i) : ImageOperator<ImageType, ResultType, PixelType>(i) {}
};

/**
 * Returns value of the pixel at the requested location in the image.
 */
template<typename PixelType>
class PixelValue
    : public UnaryImageOperator<cv::Mat const, PixelType, PixelType>
{
public :
    PixelValue(cv::Mat const & i) : UnaryImageOperator<cv::Mat const, PixelType, PixelType>(i) {}
    PixelType operator()(const cv::Point & p)
    {
        return this->at(p);
    }
};

/**
 * Combines two image pixels using provided binary operator.
 */
template<typename PixelType, typename BinaryFunction>
class PixelCombine
    : private BinaryFunction//empty base optimization
    , public BinaryImageOperator<cv::Mat const, typename BinaryFunction::result_type, PixelType>
{
public :
    PixelCombine(cv::Mat const & i, BinaryFunction f = BinaryFunction())
        : BinaryFunction(f), BinaryImageOperator<cv::Mat const, typename BinaryFunction::result_type, PixelType>(i) {}
    typename BinaryFunction::result_type operator()(const cv::Point & a, const cv::Point & b)
    {
        return BinaryFunction::operator()(this->at(a), this->at(b));
    }
};

/**
 * Functor wrapper for std::max.
 */
template<typename T>
struct Max
{
    typedef T result_type, first_argument_type, second_argument_type;
    T operator()(T a, T b)
    {
        return std::max(a, b);
    }
};

/**
 * Functor wrapper for std::min.
 */
template<typename T>
struct Min
{
    typedef T result_type, first_argument_type, second_argument_type;
    T operator()(T a, T b)
    {
        return std::min(a, b);
    }
};

/**
 * Functor for maximum absolute difference between the channels of two pixels.
 */
template<typename T, size_t N>
class MaxAbsDiff
    : public BinaryImageOperator<cv::Mat const, T, cv::Vec<T,N> >
{
public :
    MaxAbsDiff(cv::Mat const & i) : BinaryImageOperator<cv::Mat const, T, cv::Vec<T,N> >(i) {}
    T operator()(const cv::Point & a, const cv::Point & b)
    {
        assert(N > 0);
        //auto pa = this->at(a)
        //auto pb = this->at(b);
/*        T retval = abs_diff(pa[0], pb[0]);
        for(size_t i = 1; i < N; ++i)
            retval = std::max(retval, abs_diff(pa[i], pb[i]));
        return retval;*/
        return max_abs_diff(this->at(a), this->at(b));
    }
};

// Prefix sum of histogram array
template<typename T>
inline void histogramToOffsets(
        const T * in , // [n]
              T * out, // [n+1]
        size_t n
        )
{
    out[0] = 0;
    std::partial_sum(in, in + n, out + 1);
}

inline void reorderBuckets(
        const Edge * unsorted,//[count]
        Edge * sorted,//[count]
        size_t count,
        size_t * offsets // [256]
        )
{
    assert(unsorted); assert(sorted);
    for(size_t i = 0; i < count; ++i)
        sorted[offsets[unsorted[i].weight]++] = unsorted[i];
}
#endif

#include "image_graph.inl"

}//namespace image

}//namespace cct

#endif//CONNECTED_COMPONENT_TREE_IMAGE_GRAPH_H_INCLUDED
