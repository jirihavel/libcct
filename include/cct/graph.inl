template<typename V, typename W, typename EdgeWeightFunction> 
size_t getSortedEdges(
    V beg, V end, // vertex range
    Edge<V, W> * edges,//[end-beg]
    EdgeFunction e//f(V)
)
{
    size_t count = 0;
    for(V v = beg; v < end; ++v)
    {
//        edges[count].vertices[0] = v;
        edges[count++] = f(v);
        //auto e = f(v);
        //if(e)
        //{
        //    edges[count++] = *e;
        //}
    }
    std::sort(edges, edges+count);
    return count;
}

//template<typename V, typename EdgeWeightFunction> 
//size_t getSortedEdges(
//    V beg, V end, // vertex range
//    Edge<V, uint8_t> * edges,//[end-beg]
//    EdgeWeightFunction f,//f(V)
//)

