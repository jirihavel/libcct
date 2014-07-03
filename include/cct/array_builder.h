#ifndef CONNECTED_COMPONENT_TREE_ARRAY_TREE_BUILDER_H_INCLUDED
#define CONNECTED_COMPONENT_TREE_ARRAY_TREE_BUILDER_H_INCLUDED

#include "cct/image_graph.h"
#include "cct/array_tree.h"
#include "cct/root_finder.h"

namespace cct {

namespace image {

template<
    typename T, typename I, typename S, typename Weight,
    typename EdgeWeightFunction
>
void buildAlphaTree(
    cv::Size_<T> const & size, cv::Size_<T> const & tile,
    array_tree<I, S, Weight> & tree,
    EdgeWeightFunction e
)
{
    // T ... [0, max(W, H)]
    // I ... [0, V[ (V = W*H)
    // S ... [0, 2*V-1[
    typedef cv::Rect_<T> Rect;

    typedef Edge<T, Weight> Edge;

    std::vector<Edge> edges(edgeCount<size_t>(size));
    size_t const edge_count = getSortedImageEdges(
        Rect(0, 0, size.width, size.height), tile, &edges[0], e);

    cct::PackedRootFinder<S, I> root(vertexCount<I>(size), cct::LeafIndexTag());
    
    std::unique_ptr<S[]> merges(new S[tree.leaf_count]);

    Weight weight = edges[0].weight;
    S layer_begin = tree.node_count;
    for(size_t i = 0; i < edge_count; ++i)
    {
        if(edges[i].weight > weight)
        {
            weight = edges[i].weight;
            layer_begin = tree.node_count;
        }
        I const ha = root.find_update(pointId<I>(edges[i].points[0], size));
        I const hb = root.find_update(pointId<I>(edges[i].points[1], size));
        if(ha != hb)
        {
            root.merge_set(ha, hb,
                tree.alpha_merge(root.data(ha), root.data(hb), layer_begin, edges[i].weight, merges.get())
            );
        }
    }
    tree.finish_alpha_merges(merges.get());
    tree.compress(merges.get());
} 

}//namespace image

}//namespace cct

#endif//CONNECTED_COMPONENT_TREE_ARRAY_TREE_BUILDER_H_INCLUDED
