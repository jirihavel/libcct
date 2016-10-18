#ifndef LIBCCT_BUILDER_H_INCLUDED
#define LIBCCT_BUILDER_H_INCLUDED

#include <cct/graph.h>
#include <cct/root_finder.h>

namespace cct {

template<typename Tree>
class BuilderBase
{
    using CompHandle = typename Tree::CompHandle;
protected :
    BuilderBase() = default;
public :
    template<typename VertexId, typename Weight>
    bool compareLayer(Tree const &, CompHandle, Edge<VertexId, Weight> const &)
    {
        return false;
    }

    void finishLayer(Tree const &, CompHandle const *, size_t)
    {
    }

    void finish(Tree const &, CompHandle const *, size_t)
    {
    }
};

template<typename Tree> class TreeBuilder;
template<typename Tree> class AlphaTreeBuilder;
template<typename Tree> class AltitudeTreeBuilder;

template<
    template<typename> class Builder,
    typename VertexId, typename Weight,
    typename Tree>
typename Tree::RootCount buildTree(
        Tree & tree, typename Tree::LeafCount vertex_count,
        Edge<VertexId, Weight> const * edges, size_t edge_count,
        typename Tree::CompHandle * edge_comps = nullptr
    );

template<
    template<typename> class Builder,
    typename VertexId, typename VertexWeight, typename EdgeWeight,
    typename Tree>
typename Tree::RootCount buildTree(
        Tree & tree, typename Tree::LeafCount leaf_count,
        Vertex<VertexId, VertexWeight> const * vertices, size_t vertex_count,
        Edge<VertexId, EdgeWeight> const * edges, size_t edge_count,
        typename Tree::CompHandle * edge_comps = nullptr
    );

#include "builder.inl"

}//namespace cct

#endif//LIBCCT_BUILDER_H_INCLUDED
