namespace detail {

template<
    typename Tree,
    typename Builder,
    typename Finder,
    typename Vertex>
typename Builder::RootHandle addVertex(
        Tree & tree, Builder & builder, Finder & finder,
        Vertex const & vertex
    )
{
    auto const v = finder.find_update(vertex.id);
    auto const n = builder.lift(tree, finder.data(v), vertex);
    finder.data(v) = n;
    return n;
}

template<
    typename Tree,
    typename Builder,
    typename Finder,
    typename Edge>
typename Builder::RootHandle addEdge(
        Tree & tree, typename Tree::LeafCount & root_count,
        Builder & builder, Finder & finder,
        Edge const & edge
    )
{
    auto const a = finder.find_update(edge.vertices[0]);
    auto const b = finder.find_update(edge.vertices[1]);
    auto n = finder.data(a);
    if(a != b)
    {
        n = builder.merge(tree, n, finder.data(b), edge);
        finder.merge_set(a, b, n);
        --root_count;// every merge removes one root
    }
    return n;
}

}//namespace detail

template<
    template<typename> class BuilderTemplate,
    typename Vertex, typename Weight,
    typename Tree>
typename Tree::RootCount buildTree(
        Tree & tree, typename Tree::LeafCount leaf_count,
        Edge<Vertex, Weight> const * edges, size_t edge_count,
        typename Tree::CompHandle * edge_comps
    )
{
    // at the beginning, every leaf is a root
    auto root_count = leaf_count;

    if((root_count > 1) && (edge_count > 0))
    {
        using Builder = BuilderTemplate<Tree>;
        // Represents root of a subtree (may need associated VertexId)
        using RootHandle = typename Builder::RootHandle;

        Builder builder(tree, leaf_count, edges[0].weight);

        PackedRootFinder<RootHandle, typename Tree::LeafCount>
            finder(leaf_count, typename Builder::RootFinderInitializer());

        RootHandle n{};// node to which last edge belongs

        size_t i = 0;
//        size_t j = 0;
        // go over extracted edges, stop when tree has single root
        //for(; (root_count > 1) && (i < edge_count); ++i)
        for(;;)
        {
            n = detail::addEdge(tree, root_count, builder, finder, edges[i]);
            if(edge_comps)
                edge_comps[i] = builder.r2c(tree, n);
            ++i;
            if((root_count <= 1) && (i >= edge_count))
                break;
            if(builder.compareLayer(tree, n, edges[i]))
                builder.finishLayer(tree, edge_comps, i);//TODO
        }
        // all remaining edges belong to tree root (n)
        if(edge_comps)
        {
            std::fill(edge_comps+i, edge_comps+edge_count, builder.r2c(tree, n));
        }
        // finish construction
        builder.finish(tree, edge_comps, edge_count);
    }
    else
    {
        tree.init(leaf_count);
    }
    return root_count;
}

template<
    template<typename> class BuilderTemplate,
    typename VertexId, typename VertexWeight, typename EdgeWeight,
    typename Tree>
typename Tree::RootCount buildTree(
        Tree & tree, typename Tree::LeafCount leaf_count,
        Vertex<VertexId, VertexWeight> * vertices, size_t vertex_count,
        Edge<VertexId, EdgeWeight> * edges, size_t edge_count,
        typename Tree::CompHandle * edge_comps
    )
{
    // at the beginning, every leaf is a root
    auto root_count = leaf_count;

    if((root_count > 1) && (edge_count > 0))
    {
        using Builder = BuilderTemplate<Tree>;
        // Represents root of a subtree (may need associated VertexId)
        using RootHandle = typename Builder::RootHandle;

        Builder builder(tree, leaf_count, edges[0].weight);

        PackedRootFinder<RootHandle, typename Tree::LeafCount>
            finder(leaf_count, typename Builder::RootFinderInitializer());

        RootHandle n{};// node to which last edge belongs

        size_t vi = 0;
        size_t ei = 0;
        // stop when all vertices are processed and no edges will change the tree structure
        while((vi < vertex_count) || ((root_count > 1) && (ei < edge_count)))
        {
            if((vi < vertex_count) && ((ei >= edge_count) || (vertices[vi] < edges[ei])))
            {
                n = detail::addVertex(tree, builder, finder, vertices[vi]);
                ++vi;
            }
            else
            {
                n = detail::addEdge(tree, root_count, builder, finder, edges[ei]);
                if(edge_comps)
                {
                    edge_comps[ei] = builder.r2c(tree, n);
                }
                ++ei;
            }
        }
        // all remaining edges belong to tree root (n)
        if(edge_comps)
        {
            std::fill(edge_comps+ei, edge_comps+edge_count, builder.r2c(tree, n));
        }
        // finish construction
        builder.finish(tree, edge_comps, edge_count);
    }
    else
    {
        tree.init(leaf_count);
    }
    return root_count;
}
