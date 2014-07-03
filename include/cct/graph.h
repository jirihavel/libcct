#ifndef CONNECTED_COMPONENT_TREE_GRAPH_H_INCLUDED
#define CONNECTED_COMPONENT_TREE_GRAPH_H_INCLUDED

#include <cstdint>

#include <algorithm>

namespace cct {

namespace graph {

template<typename V, typename W>
struct Edge
{
    typedef V Vertex;
    typedef W Weight;

    Vertex vertices[2];

    Weight weight;

    bool operator<(Edge const & e) const
    {
        return weight < e.weight;
    } 
};

template<typename V, typename W, typename EdgeWeightFunction> 
size_t getSortedEdges(
    V beg, V end, // vertex range
    Edge<V, W> * edges,//[end-beg]
    EdgeFunction f//f(V)
);
//template<typename V, typename EdgeWeightFunction> 
//size_t getSortedEdges(
//    V beg, V end, // vertex range
//    Edge<V, uint8_t> * edges,//[end-beg]
//    EdgeWeightFunction e//f(V)
//);

#include "graph.inl"

}//namespace graph

}//namespace cct

#endif//CONNECTED_COMPONENT_TREE_GRAPH_H_INCLUDED
