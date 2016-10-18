#ifndef LIBCCT_GRAPH_H_INCLUDED
#define LIBCCT_GRAPH_H_INCLUDED

namespace cct {

template<typename V, typename W>
struct Vertex
{
    typedef V VertexId;
    typedef W Weight;

    VertexId id;
    Weight weight;

    Vertex() = default;

    void set(Weight w, VertexId i)
    {
        id = i;
        weight = w;
    }
};

template<typename V, typename W>
struct Edge
{
    typedef V VertexId;
    typedef W Weight;

    VertexId vertices[2];
    Weight weight;

    Edge() = default;

    Edge(V v0, V v1, W w) : weight(w)
        { vertices[0] = v0; vertices[1] = v1; }

    void set(Weight w, VertexId a, VertexId b)
    {
        vertices[0] = a;
        vertices[1] = b;
        weight = w;
    }
};

template<typename V, typename W>
inline bool operator<(Vertex<V,W> const & a, Vertex<V,W> const & b) noexcept
{
    return a.weight < b.weight;
}

template<typename V, typename W>
inline bool operator<(Edge<V,W> const & a, Edge<V,W> const & b) noexcept
{
    return a.weight < b.weight;
}

template<typename V, typename W>
inline bool operator<(Vertex<V,W> const & v, Edge<V,W> const & e) noexcept
{
    return v.weight < e.weight;
}

#include "graph.inl"

}//namespace cct

#endif//LIBCCT_GRAPH_H_INCLUDED
