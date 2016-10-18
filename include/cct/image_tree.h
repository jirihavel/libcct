#ifndef LIBCCT_IMAGE_TREE_H_INCLUDED
#define LIBCCT_IMAGE_TREE_H_INCLUDED

#include "graph.h"
#include "tree.h"
#include "image.h"

#include <type_traits>

#include <opencv2/core/core.hpp>

namespace cct {
namespace img {

template<typename LevelType>
class Component
    : public cct::ComponentBase<DefaultTreeParams>
{
public :
    using Base  = cct::ComponentBase<DefaultTreeParams>;
    using Level = LevelType;

    template<typename T, typename W>
    explicit Component(cct::Vertex<T,W> const & v)
        : m_level(v.weight)
    {}

    template<typename T, typename W>
    explicit Component(cct::Edge<T,W> const & e)
        : m_level(e.weight)
    {}

    template<typename T, typename W>
    void init(cct::Edge<T,W> const & e)
    {
        BOOST_ASSERT(!Base::parent());
        BOOST_ASSERT(Base::empty());
        m_level = e.weight;
    }

    Level level() const
    {
        return m_level;
    }

    template<typename T, typename W>
    bool operator<(cct::Vertex<T,W> const & v) const
    {
        return level() < v.weight;
    }
    template<typename T, typename W>
    bool operator<(cct::Edge<T,W> const & e) const
    {
        return level() < e.weight;
    }
    template<typename T, typename W>
    bool operator<=(cct::Edge<T,W> const & e) const
    {
        return level() <= e.weight;
    }
    bool operator<(Component const & node) const
    {
        return level() < node.level();
    }
    bool operator<=(Component const & node) const
    {
        return level() <= node.level();
    }
private :
    Level m_level;
};

template<typename L>
std::ostream & operator<<(std::ostream & o, Component<L> const & c)
{
    return o << +c.level();
}

using Leaf = cct::LeafBase<DefaultTreeParams>;

template<typename Tree>
class Printer
{
    using Type = typename Tree::LeafIndex;
    using Size = cv::Size_<Type>;
public :
    Printer(Size const & s) : m_size(s) {}

    std::ostream & print(std::ostream & o, Tree const &, typename Tree::Component const & c) const
    {
        return o << '[' << c << ']';
    }
    std::ostream & print(std::ostream & o, Tree const & t, typename Tree::Leaf const & l) const
    {
        auto const i = t.leafId(l);
        auto const p = idToPoint<Type>(i, m_size);
        return o << '(' << i << ':' << p.x << ',' << p.y << ')';
    }
private :
    Size m_size;
};
#if 0
/** \brief Alpha-tree construction
 */
template<
    typename T,
    typename Builder,
    typename EdgeWeightFunction
    >
void buildAlphaTree(
    cv::Size_<T> const & size, // Image size, the actual image is hidden in WeightFunction
    cv::Size_<T> const & tile, // Tile size for tiled image scan. Set to <size> to disable tiled scan.
    Builder & builder, // Tree builder to use
    EdgeWeightFunction e // Function for edge weight calculation
);

template<
    typename T,
    typename Builder,
    typename EdgeWeightFunction
    >
void buildAlphaTree(
    cv::Size_<T> const & size, // Image size, the actual image is hidden in WeightFunction
    cv::Size_<T> const & tile, // Tile size for tiled image scan. Set to <size> to disable tiled scan.
    Builder & builder, // Tree builder to use
    EdgeWeightFunction e, // Function for edge weight calculation
    unsigned depth // Depth of the binary parallelization tree
);

/*
template<typename T,
    typename Builder,
    typename VertexWeightFunction,
    typename EdgeWeightFunction
    >
void buildSecondOrderTree(
        cv::Rect_<T> const & rect,
        Builder & builder,
        VertexWeightFunction v,
        EdgeWeightFunction e
        );
*/
#include "image_tree.inl"
#endif
}//namespace img
}//namespace cct

#endif//LIBCCT_IMAGE_TREE_H_INCLUDED
