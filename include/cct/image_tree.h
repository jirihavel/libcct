#ifndef CONNECTED_COMPONENT_TREE_IMAGE_TREE_H_INCLUDED
#define CONNECTED_COMPONENT_TREE_IMAGE_TREE_H_INCLUDED

#include "node.h"
#include "tree.h"
#include "builder.h"
#include "image_graph.h"

#include <type_traits>

#include <boost/thread/thread.hpp>

namespace cct {

namespace image {

template<typename Level>
class Component
    : public cct::ComponentBase
{
private :
    typedef cct::ComponentBase Base;
    Level m_level;
public :
    template<typename T, typename W>
    explicit Component(Vertex<T,W> const & v)
        : m_level(v.weight)
    {}

    template<typename T, typename W>
    explicit Component(Edge<T,W> const & e)
        : m_level(e.weight)
    {}

    template<typename T, typename W>
    void init(Edge<T,W> const & e)
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
    bool operator<(Vertex<T,W> const & v) const
    {
        return level() < v.weight;
    }
    template<typename T, typename W>
    bool operator<(Edge<T,W> const & e) const
    {
        return level() < e.weight;
    }
    template<typename T, typename W>
    bool operator<=(Edge<T,W> const & e) const
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
};

template<typename L>
std::ostream & operator<<(std::ostream & o, Component<L> const & c)
{
    return o << c.level();
}

std::ostream & operator<<(std::ostream & o, Component<uint8_t> const & c)
{
    return o << unsigned(c.level());
}

typedef cct::LeafBase Leaf;

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

}//namespace image

}//namespace cct

#endif//CONNECTED_COMPONENT_TREE_IMAGE_TREE_H_INCLUDED
