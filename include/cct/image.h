#ifndef LIBCCT_IMAGE_H_INCLUDED
#define LIBCCT_IMAGE_H_INCLUDED

#include "graph.h"

#include <utils/fp.h>
#include <utils/opencv.h>
#include <utils/pun.h>
#include <utils/uint_conv.h>
#include <utils/underlying_value.h>

#include <memory>

#include <boost/assert.hpp>
#include <boost/optional.hpp>

#include <opencv2/core/core.hpp>

namespace cct {
namespace img {

#define CCT_WHOLE_IMAGE_RECT {0,0,0,0}

/** \defgroup ImgGraph Image graph
 * @{
 */

/** \brief Variants of image connectivity
 */
enum class Connectivity
{
    C4 = 0, // 4 connectivity
    C6P = 1, // 4 + diagonal top left -> bottom right
    C6N = 2,
    C8 = C6P | C6N // 8 connectivity
};

template<typename T>
T vertexCount(cv::Size_<T> const & size) noexcept;

template<typename T>
T edgeCount(cv::Size_<T> const & size, Connectivity connectivity = Connectivity::C4) noexcept;

/** \brief Image point index as if scanned row by row.
 */
template<typename T>
T pointId(cv::Point_<T> const & point, cv::Size_<T> const & size) noexcept;

template<typename T>
cv::Point_<T> idToPoint(T id, cv::Size_<T> const & size) noexcept;

/** \defgroup ImgGraphVert Vertex extraction
 * @{
 */

/** \brief Call f for each vertex.
 * Call f(p,i) for each point in rect :
 * - f : (cv::Point_<T>, size_t) -> ?
 * - i == pointId(p, size)
 *
 * Default rect means whole size.
 */
template<typename F, typename T>
void forEachVertex(
        cv::Size_<T> const & size,
        F f = F(),
        cv::Rect_<T> const & rect = CCT_WHOLE_IMAGE_RECT
    );

/** \brief Extract image vertices using weight functor.
 * Calls f(p) for each image vertex :
 * - f : cv::Point_<T> -> W or boost::optional<W>
 *
 * \returns Number of extracted vertices.
 * - f returns optional : <= vertexCount(rect.size())
 * - otherwise : == vertexCount(rect.size())
 */	
template<typename F, typename T, typename V, typename W>
size_t getImageVertices(
        cv::Size_<T> const & size,
        Vertex<V, W> * verts,//[vertexCount(rect.size())]
        F f = F(),
        cv::Rect_<T> const & rect = CCT_WHOLE_IMAGE_RECT
    );

/** \brief Extract and sort vertices.
 * Calls f(p) for each image vertex :
 * - f : cv::Point_<T> -> W or boost::optional<W>
 */	
template<typename F, typename T, typename V, typename W>
size_t getSortedImageVertices(
        cv::Size_<T> const & size,
        Vertex<V, W> * verts,//[vertexCount(rect.size())]
        F f = F(),
        cv::Rect_<T> const & rect = CCT_WHOLE_IMAGE_RECT
    );

template<
    template<typename C,int N> class F,
    typename T = int,
    typename C, int N,
    typename V, typename W>
size_t getImageVertices(
        cv::Mat_<cv::Vec<C,N>> const & image,
        Vertex<V,W> * verts,
        cv::Rect_<T> const & rect = CCT_WHOLE_IMAGE_RECT
    );

/**@}*/

/** \defgroup ImgGraphEdge Edge extraction
 * @{
 */

/** \brief Call f for each edge.
 * Call f(p_1, i_1, p_2, i_2) for each edge in rect :
 * - f : (Point, size_t, Point, size_t) -> ?
 * - i_x == pointId(p_x, size)
 */
template<typename F, typename T>
void forEachEdge(
        cv::Size_<T> const & size,
        F f = F(),
        cv::Rect_<T> const & rect = CCT_WHOLE_IMAGE_RECT,
        Connectivity connectivity = Connectivity::C4
    );

/** \brief Extract image edges using weight functor.
 * Calls f(p_1,p_2) for each image edge :
 * - f : (Point,Point) -> W or boost::optional<W>
 */	
template<typename F, typename T, typename V, typename W>
inline size_t getImageEdges(
        cv::Size_<T> const & size,//image size
        Edge<V, W> * edges,//[edgeCount(rect.size())]
        F f = F(),
        cv::Rect_<T> const & rect = CCT_WHOLE_IMAGE_RECT,
        Connectivity connectivity = Connectivity::C4
    );

template<typename F, typename T, typename V, typename W>
size_t getSortedImageEdges(
        cv::Size_<T> const & size,
        Edge<V, W> * edges,//[edgeCount(size)]
        F f = F(),
        cv::Rect_<T> const & rect = CCT_WHOLE_IMAGE_RECT,
        Connectivity connectivity = Connectivity::C4
    );

/** \brief Extract image edges from specialized cv::Mat_.
 * Uses F<C,N>(image) as the weight functor.
 */
template<
    template<typename,int> class F,// parameterized by C and N
    typename T = int, // pixel coord type
    typename C, int N, // pixel format
    typename V, typename W> // edge format
inline size_t getImageEdges(
        cv::Mat_<cv::Vec<C, N>> const & image,
        Edge<V, W> * edges,
        cv::Rect_<T> const & rect = CCT_WHOLE_IMAGE_RECT,
        Connectivity connectivity = Connectivity::C4
    );

/** \brief Extract image edges from specialized cv::Mat_.
 * Uses F<C,N>(image) as the weight functor.
 */
template<
    template<typename,int> class F,// parameterized by C and N
    typename T = int, // pixel coord type
    typename C, int N, // pixel format
    typename V, typename W> // edge format
inline size_t getSortedImageEdges(
        cv::Mat_<cv::Vec<C, N>> const & image,
        Edge<V, W> * edges,
        cv::Rect_<T> const & rect = CCT_WHOLE_IMAGE_RECT,
        Connectivity connectivity = Connectivity::C4
    );

/** \brief Extract image edges from generic cv::Mat.
 * Uses two level switch to call correct specialized function.
 */
template<
    template<typename,int> class F,
    typename T = int,
    typename V, typename W>
inline size_t getImageEdges(
        cv::Mat const & image, Edge<V, W> * edges,
        cv::Rect_<T> const & rect = CCT_WHOLE_IMAGE_RECT,
        Connectivity connectivity = Connectivity::C4
    );

/** \brief Extract image edges from generic cv::Mat.
 * Uses two level switch to call correct specialized function.
 */
template<
    template<typename,int> class F,
    typename T = int,
    typename V, typename W>
inline size_t getSortedImageEdges(
        cv::Mat const & image, Edge<V, W> * edges,
        cv::Rect_<T> const & rect = CCT_WHOLE_IMAGE_RECT,
        Connectivity connectivity = Connectivity::C4
    );

/**@}*/

/** \defgroup ImgGraphElem Graph extraction
 * @{
 */

/**
 */
template<typename V, typename E, typename T>
void forEachElement(
        cv::Size_<T> const & size,
        V v = V(), E e = E(),
        cv::Rect_<T> const & rect = CCT_WHOLE_IMAGE_RECT,
        Connectivity connectivity = Connectivity::C4
    );

/**
 */
template<typename V, typename E, typename T, typename I, typename VW, typename EW>
std::pair<size_t, size_t> getImageGraph(
        cv::Size_<T> const & size,
        Vertex<I, VW> * verts,//[vertexCount(rect.size())]
        Edge<I, EW> * edges,//[edgeCount(rect.size())]
        V v = V(), E e = E(),
        cv::Rect_<T> const & rect = CCT_WHOLE_IMAGE_RECT,
        Connectivity connectivity = Connectivity::C4
    );

/**
 */
template<typename V, typename E, typename T, typename I, typename VW, typename EW>
std::pair<size_t, size_t> getSortedImageGraph(
        cv::Size_<T> const & size,
        Vertex<I, VW> * verts,//[vertexCount(rect.size())]
        Edge<I, EW> * edges,//[edgeCount(rect.size())]
        V v = V(), E e = E(),
        cv::Rect_<T> const & rect = CCT_WHOLE_IMAGE_RECT,
        Connectivity connectivity = Connectivity::C4
    );

/**@}*/

/** \defgroup ImgGraphBuild Tree construction
 * @{
 */

/** \brief Construct alpha-tree from an image described by edge weight functor.
 *
 * e is functor of type cv::Point_<T>^2 -> Weight (or something convertible to it)
 */ 
template<
    template<typename> class Builder,
    typename EdgeWeightFunction,
    typename T,
    typename Tree>
std::pair<size_t, typename Tree::RootCount>
    buildTree(
        cv::Size_<T> const & size,
        Tree & tree,
        EdgeWeightFunction e,
        typename Tree::CompHandle * edge_comps = nullptr,
        Connectivity connectivity = Connectivity::C4
    );

/** \brief ...
 */ 
template<
    template<typename> class Builder,
    typename VertexWeightFunction, typename EdgeWeightFunction,
    typename T,
    typename Tree>
std::pair<size_t, typename Tree::RootCount>
    buildTree(
        cv::Size_<T> const & size,
        Tree & tree,
        VertexWeightFunction v, EdgeWeightFunction e,
        typename Tree::CompHandle * edge_comps = nullptr,
        Connectivity connectivity = Connectivity::C4
    );

/** \brief ...
 */ 
template<
    template<typename> class Builder,
    typename WeightFunction,
    typename T,
    typename Tree>
std::pair<size_t, typename Tree::RootCount>
    buildTree2(
        cv::Size_<T> const & size,
        Tree & tree,
        WeightFunction w,
        typename Tree::CompHandle * edge_comps = nullptr,
        Connectivity connectivity = Connectivity::C4
    );

/** \brief Construct alpha-tree from image (cv::Mat_).
 */
template<
    template<typename> class Builder,
    template<typename,int> class EdgeWeightFunctor,
    typename T = int,
    typename Mat,
    typename Tree>
std::pair<size_t, typename Tree::RootCount>
    buildTree(
        Mat const & image,
        Tree & tree,
        typename Tree::CompHandle * edge_comps = nullptr,
        Connectivity connectivity = Connectivity::C4
    );

/**@}*/

/** \brief Fill image from tree leaves.
 *
 * Iterates over image and uses functor F to convert leaf indices to
 * pixel values.
 */
template<typename P, typename F>
F leavesToColors(cv::Mat_<P> & image, F f)
{
    forEachVertex(image.size(),
            [&image,&f](cv::Point const & p, size_t i)
            {
                image(p) = f(i);
            }
        );
    return f;
}
    
/**@}*/

#include "image.inl"

}//namespace img
}//namespace cct

#endif//LIBCCT_IMAGE_H_INCLUDED
