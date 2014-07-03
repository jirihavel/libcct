template<
    typename T,
    typename Builder,
    typename EdgeWeightFunction
>
void buildAlphaTree(
    cv::Size_<T> const & size, cv::Size_<T> const & tile,
    Builder & builder,
    EdgeWeightFunction e
)
{
    typedef cv::Point_<T> Point;
    typedef decltype(e(Point(),Point())) Weight;
    typedef Edge<T, Weight> Edge;

    std::vector<Edge> edges(edgeCount(size));
    size_t const count = getSortedImageEdges(
        cv::Rect_<T>(0, 0, size.width, size.height), tile, &edges[0], e);

    ThreadBuilder<Builder> thread_builder(builder);

    Weight lastWeight = edges[0].weight;
    for(size_t i = 0; i < count; ++i)
    {
        if(lastWeight < edges[i].weight)
        {
            thread_builder.remove();
            lastWeight = edges[i].weight;
        }
        thread_builder.addEdge(
            pointId(edges[i].points[0], size),
            pointId(edges[i].points[1], size),
            edges[i]
        );
    }
    builder.finish(std::move(thread_builder));
}

template<
    typename T,
    typename B,
    typename WeightFunction
    >
void buildAlphaTree(
    cv::Size_<T> const & size,
    cv::Size_<T> const & tile,
    ThreadBuilder<B> & builder,
    WeightFunction e,
    unsigned treeDepth,
    cv::Rect_<T> const & rect
)
{
    typedef cv::Point_<T> Point;
    typedef decltype(e(Point(),Point())) Weight;
    typedef Edge<T, Weight> Edge;
    typedef typename B::Component Component;

    if(treeDepth == 0)
    {
        size_t ec = edgeCount(rect.size());
        std::vector<Edge> edges(ec);
        size_t count = getSortedImageEdges(rect, tile, &edges[0], e);

        size_t remaining_merges = vertexCount(rect.size())-1;

        Weight lastWeight = edges[0].weight;
        for(size_t i = 0; i < count; ++i)
        {
            if(lastWeight < edges[i].weight)
            {
                builder.remove();
                lastWeight = edges[i].weight;
            }
            if(builder.addEdge(
                pointId(edges[i].points[0], size),
                pointId(edges[i].points[1], size),
                edges[i])
            )
            {
                if(--remaining_merges == 0)
                    break;
            }
        }
        builder.remove();
    }
    else
    {
        size_t count;
        cv::Rect_<T> ra;
        cv::Rect_<T> rb;
        if(rect.width > rect.height)
        {
            // split vertically
            count = rect.height;
            T w2 = rect.width/2;
            ra = cv::Rect_<T>(rect.x, rect.y, w2, rect.height);
            rb = cv::Rect_<T>(rect.x+w2, rect.y, rect.width-w2, rect.height);
        }
        else
        {
            // split horizontally
            count = rect.width;
            T h2 = rect.height/2;
            ra = cv::Rect_<T>(rect.x, rect.y, rect.width, h2);
            rb = cv::Rect_<T>(rect.x, rect.y+h2, rect.width, rect.height-h2);
        }
        // build partial trees
        ThreadBuilder<B> thread_builder(builder.builder());
        // spawn thread, TODO : copy everything to its stack
        boost::thread task([&]()
        {
            buildAlphaTree(size, tile, thread_builder, e, treeDepth-1, rb);
        });
        buildAlphaTree(size, tile, builder, e, treeDepth-1, ra);
        // extract connecting edges
        std::vector<Edge> edges(count);
        if(rect.width > rect.height)
        {
            count = getSortedHorizontalConnectors(Point(rect.x+rect.width/2-1, rect.y), rect.height, &edges[0], e);
        }
        else
        {
            count = getSortedVerticalConnectors(Point(rect.x, rect.y+rect.height/2-1), rect.width, &edges[0], e);
        }
        // merge trees
        task.join();
        builder.absorb(std::move(thread_builder));
        for(size_t i = 0; i < count; ++i)
        {
            Edge const & edge = edges[i];
            auto a = pointId(edge.points[0], size);
            auto b = pointId(edge.points[1], size);
            Component * n = builder.merge_roots(a, b, edge);
            if(*n <= edge)
                break;//continue;
            builder.merge_paths(a, b, edge);
        }
    }
}

template<
    typename T,
    typename Builder,
    typename EdgeWeightFunction
>
void buildAlphaTree(
    cv::Size_<T> const & size,
    cv::Size_<T> const & tile,
    Builder & builder,
    EdgeWeightFunction e,
    unsigned depth
)
{
    ThreadBuilder<Builder> thread_builder(builder);
    buildAlphaTree(size, tile, thread_builder, e, depth,
        cv::Rect_<T>(0, 0, size.width, size.height)
    );
    builder.finish(std::move(thread_builder));
}

#if 0
template<
    typename T,
    typename Builder,
    typename WeightFunction
    >
typename Builder::Roots
buildAlphaTreeNomerge(
    cv::Size_<T> const & size,
    cv::Size_<T> const & tile,
    Builder & builder,
    WeightFunction e,
    unsigned treeDepth,
    cv::Rect_<T> const & rect
)
{
    typedef cv::Point_<T> Point;
    typedef decltype(e(Point(),Point())) Weight;
    typedef Edge<T, Weight> Edge;
    typedef typename Builder::Roots Roots;

    Roots roots;

    if(treeDepth == 0)
    {
        size_t ec = edgeCount(rect.size());
        //std::unique_ptr<Edge[]> edges(new Edge[ec]);
        std::vector<Edge> edges(ec);
        size_t count = getSortedImageEdges(rect, tile, &edges[0], e);
        //std::/*stable_*/sort(edges.get(), edges.get()+count);
        BOOST_ASSERT(count == ec);

        typename Builder::Components pool;
        typename Builder::Components toDelete;

        Weight lastWeight = edges[0].weight;
        for(size_t i = 0; i < count; ++i)
        {
            if(lastWeight < edges[i].weight)
            {
                builder.removeTopLayerComponents(toDelete);
                pool.splice(pool.begin(), toDelete);
                lastWeight = edges[i].weight;
            }
            builder.addEdge(
                cct::pointId(edges[i].points[0], size),
                cct::pointId(edges[i].points[1], size),
                edges[i], roots, toDelete, pool
            );
        }
        builder.removeTopLayerComponents(toDelete);
        pool.splice(pool.begin(), toDelete);
        builder.deleteNodes(std::move(pool));
    }
    else
    {
        Roots threadRoots;
        //std::unique_ptr<Edge[]> edges;
        std::vector<Edge> edges;
        size_t count;
        if(rect.width > rect.height)
        {
            count = rect.height;
            // split vertically
            T w2 = rect.width/2;
            //auto task = boost::async(boost::launch::any, [&]()
            boost::thread task([&]()
            {
                threadRoots = buildAlphaTreeNomerge(
                    size, tile, builder, e, treeDepth-1,
                    cv::Rect_<T>(rect.x+w2, rect.y, rect.width-w2, rect.height)
                );
            });
            roots = buildAlphaTreeNomerge(
                size, tile, builder, e, treeDepth-1,
                cv::Rect_<T>(rect.x, rect.y, w2, rect.height)
            );
            // get connecting edges
            //edges.reset(new Edge[count]);
            edges.resize(count);
            getHorizontalConnectors(Point(rect.x+w2-1, rect.y), rect.height, &edges[0], e);
            //std::sort(edges.get(), edges.get()+count);
            // wait for the second part of the tree
            //task.get();
            task.join();
        }
        else
        {
            count = rect.width;
            // split horizontally
            T h2 = rect.height/2;
            //auto task = boost::async(boost::launch::any, [&]()
            boost::thread task([&]()
            {
                threadRoots = buildAlphaTreeNomerge(
                    size, tile, builder, e, treeDepth-1,
                    cv::Rect_<T>(rect.x, rect.y+h2, rect.width, rect.height-h2)
                );
            });
            roots = buildAlphaTreeNomerge(
                size, tile, builder, e, treeDepth-1,
                cv::Rect_<T>(rect.x, rect.y, rect.width, h2)
            );
            // get connecting edges
            //edges.reset(new Edge[count]);
            edges.resize(count);
            getVerticalConnectors(Point(rect.x, rect.y+h2-1), rect.width, &edges[0], e);
            //std::sort(edges.get(), edges.get()+count);
            // wait for the second part of the tree
            //task.get();
            task.join();
        }
        roots.add(std::move(threadRoots));
        //pruneAlphaTree(size, builder, edges.get(), count, roots);
        //for(size_t i = 0; i < count; ++i)
        //{
        //    builder.pruneForest(
        //        cct::pointId(edges[i].points[0], size),
        //        cct::pointId(edges[i].points[1], size),
        //        edges[i], roots
        //        );
        //}
    }
    return roots;
}
template<
    typename T,
    typename Builder,
    typename WeightFunction
    >
void buildAlphaTreeNomerge(
        cv::Size_<T> const & size,
        cv::Size_<T> const & tile,
        Builder & builder,
        WeightFunction e,
        unsigned depth
        )
{
    //builder.finish(
    //    buildAlphaTreeNomerge(
    //        size, tile, builder, e, depth,
    //        cv::Rect_<T>(0, 0, size.width, size.height)
    //        )
    //);
}
#endif


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
        )
{
    typedef cv::Point_<T> Point;
    typedef decltype(v(Point())) VertexWeight;
    typedef Vertex<T, VertexWeight> Vertex;
    typedef decltype(e(Point(),Point())) EdgeWeight;
    typedef Edge<T, EdgeWeight> Edge;

    size_t vc = vertexCount(rect.size());
    size_t ec = edgeCount(rect.size());
    std::unique_ptr<Vertex[]> vertices(new Vertex[vc]);
    std::unique_ptr<Edge[]> edges(new Edge[ec]);
    getImageGraph(rect, vertices.get(), edges.get(), v, e);

    EdgeWeight lastWeight = edges[0].weight;
    for(size_t vi = 0, ei = 0; (vi < vc) && (ei < ec);)
    {
        if(vertices[vi].weight < edges[ei].weight)
        {
            builder.addVertex(
                cct::pointId(vertices[vi].point, rect),
                vertices[vi]
                );
            ++vi;
        }
        else
        {
            if(lastWeight < edges[ei].weight)
            {
                lastWeight = edges[ei].weight;
                builder.prune();
            }
            builder.addEdge(
                    cct::pointId(edges[ei].points[0], rect),
                    cct::pointId(edges[ei].points[1], rect),
                    edges[ei]
                    );
            ++ei;
        }
    }
    builder.prune();
};
*/
