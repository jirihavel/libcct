template<typename E, typename T>
E forEachEdge(
        cv::Rect_<T> const & rect,
        E e// f(p0, p1)
        )
{
    typedef cv::Point_<T> Point;
    BOOST_ASSERT(rect.width  >= 0);
    BOOST_ASSERT(rect.height >= 0);

    if((rect.width > 0) && (rect.height > 0))
    {
        T const last_x = rect.x+rect.width -1;
        T const last_y = rect.y+rect.height-1;
        for(T y = rect.y; /*checked inside*/; ++y)
        {
            //horizontal edges
            for(T x = rect.x; x < last_x; ++x)
            {
                e(Point(x, y), Point(x+1, y));
            }
            // scan last row only horizontally
            if(y >= last_y)
            {
                break;// !!!
            }
            // vertical edges
            for(T x = rect.x; x <= last_x; ++x)
            {
                e(Point(x, y), Point(x, y+1));
            }
        }
    }
    return e;
}

template<typename E, typename T>
E forEachEdge(
        cv::Rect_<T> const & rect,
        cv::Size_<T> const & tile,
        E e
        )
{
    BOOST_ASSERT(rect.x >= T(0));
    BOOST_ASSERT(rect.y >= T(0));
    BOOST_ASSERT(rect.width  >= T(0));
    BOOST_ASSERT(rect.height >= T(0));
    BOOST_ASSERT(tile.width  >  T(0));
    BOOST_ASSERT(tile.height >  T(0));

    typedef cv::Point_<T> Point;

    if((rect.width > 0) && (rect.height > 0))
    {
        T const last_row = rect.y+rect.height-1;
        // tiles in x
        T cx = (rect.width+tile.width-1)/tile.width;
        T bx[cx+1]; // boundaries
        bx[0] = rect.x;
        for(T i = 1; i < cx; ++i)
            bx[i] = bx[i-1]+tile.width;
        bx[cx] = rect.x+rect.width-1; // stop before last column

        // for tile rows except last
        T by = rect.y;
        for(; (by+tile.height) < last_row; by += tile.height)
        {
            // tiles in row
            for(T tx = 0; tx < cx; ++tx)
            {
                for(T y = by; y < by+tile.height; ++y)
                {
                    for(T x = bx[tx]; x < bx[tx+1]; ++x)
                    {
                        e(Point(x, y), Point(x+1, y));
                        e(Point(x, y), Point(x, y+1));
                    }
                }
            }
            //last pixels in tile row
            for(T y = by; y < by+tile.height; ++y)
            {
                e(Point(bx[cx], y), Point(bx[cx], y+1));
            }
        }
        //last row of tiles
        for(T tx = 0; tx < cx; ++tx)
        {
            for(T y = by; y < last_row; ++y)
            {
                for(T x = bx[tx]; x < bx[tx+1]; ++x)
                {
                    e(Point(x, y), Point(x+1, y));
                    e(Point(x, y), Point(x, y+1));
                }
            }
            // pixels in last image row
            for(T x = bx[tx]; x < bx[tx+1]; ++x)
            {
                e(Point(x, last_row), Point(x+1, last_row));
            }
        }
        //last pixels in tile row
        for(T y = by; y < last_row; ++y)
        {
            e(Point(bx[cx], y), Point(bx[cx], y+1));
        }
    }
    return e;   
}

template<typename V, typename E, typename T>
void forEachElement(
        cv::Rect_<T> const & rect,
        V v,// v(p)
        E e // e(p0, p1)
        )
{
    typedef cv::Point_<T> Point;
    BOOST_ASSERT(rect.width  >= 0);
    BOOST_ASSERT(rect.height >= 0);

    if((rect.width > 0) && (rect.height > 0))
    {
        T const last_x = rect.x+rect.width -1;
        T const last_y = rect.y+rect.height-1;
        for(T y = rect.y; /*checked inside*/; ++y)
        {
            //horizontal edges
            for(T x = rect.x; /*checked inside*/; ++x)
            {
                v(Point(x, y));
                //last pixel of a row
                if(x >= last_x)
                {
                    break;// !!!
                }
                e(Point(x, y), Point(x+1, y));
            }
            // scan last row only horizontally
            if(y >= last_y)
            {
                break;// !!!
            }
            // vertical edges
            for(T x = rect.x; x <= last_x; ++x)
            {
                e(Point(x, y), Point(x, y+1));
            }
        }
    }
}

template<typename T, typename W, typename EdgeWeightFunction>
size_t getImageEdges(
    cv::Rect_<T> const & rect,
    Edge<T, W> * edges,//[edgeCount(rect.size())]
    EdgeWeightFunction e//f(cv::Point, cv::Point)
)
{
    typedef cv::Point_<T> Point;
    typedef Edge<T, W> Edge;
    // preconditions
    BOOST_ASSERT(rect.width  >= 0);
    BOOST_ASSERT(rect.height >= 0);
    BOOST_ASSERT(edges);
    // extract edges
    size_t edgeCount = 0;
    forEachEdge(rect,
        [&](Point const & a, Point const & b)
        {
            Edge & edge = edges[edgeCount++];
            edge.points[0] = a;
            edge.points[1] = b;
            edge.weight = e(a, b);
        }
    );
    return edgeCount;
}

template<typename T, typename W, typename EdgeWeightFunction>
size_t getImageEdges(
    cv::Rect_<T> const & rect, cv::Size_<T> const & tile,
    Edge<T, W> * edges,//[edgeCount(rect.size())]
    EdgeWeightFunction e//f(cv::Point, cv::Point)
)
{
    typedef cv::Point_<T> Point;
    typedef Edge<T, W> Edge;
    // preconditions
    BOOST_ASSERT(rect.width  >= 0);
    BOOST_ASSERT(rect.height >= 0);
    BOOST_ASSERT(edges);
    // extract edges
    size_t edgeCount = 0;
    forEachEdge(rect, tile,
        [&](Point const & a, Point const & b)
        {
            Edge & edge = edges[edgeCount++];
            edge.points[0] = a;
            edge.points[1] = b;
            edge.weight = e(a, b);
        }
    );
    return edgeCount;
}

template<typename T, typename EdgeWeightFunction>
size_t getSortedImageEdges(
    cv::Rect_<T> const & rect, cv::Size_<T> const & tile,
    Edge<T, uint8_t> * edges,//[edgeCount(rect.size())]
    EdgeWeightFunction e//f(cv::Point, cv::Point)
)
{
    typedef cv::Point_<T> Point;
    typedef Edge<T, uint8_t> Edge;
    // preconditions
    BOOST_ASSERT(rect.width  >= 0);
    BOOST_ASSERT(rect.height >= 0);
    BOOST_ASSERT(edges);
    // build histogram
    unsigned histogram[256];
    memset(histogram, 0, sizeof(histogram));
    forEachEdge(rect, tile,
            [&](Point const & a, Point const & b)
            {
                ++histogram[e(a, b)];
            }
        );
    // get indices by prefix sum
    unsigned indices[257];
    indices[0] = 0;
    std::partial_sum(histogram, histogram + 256, indices + 1);
    // extract edges
    forEachEdge(rect, tile,
            [&](Point const & a, Point const & b)
            {
                uint8_t w = e(a, b);
                Edge & edge = edges[indices[w]++];
                edge.points[0] = a;
                edge.points[1] = b;
                edge.weight = w;
            }
        );
    BOOST_ASSERT(indices[255] == indices[256]);
    return indices[256];
}

template<typename T, typename W, typename EdgeWeightFunction>
size_t getSortedImageEdges(
        cv::Rect_<T> const & rect, cv::Size_<T> const & tile,
        Edge<T, W> * edges,//[edgeCount(rect.size())]
        EdgeWeightFunction e//f(cv::Point, cv::Point)
        )
{
    // preconditions
    BOOST_ASSERT(rect.width  >= 0);
    BOOST_ASSERT(rect.height >= 0);
    BOOST_ASSERT(edges);
    // extract edges
    size_t const edge_count = getImageEdges(rect, tile, edges, e);
    // sort them
    std::sort(edges, edges + edge_count);
    return edge_count;
}

template<typename T,
    typename VertexType, typename EdgeType,
    typename VertexWeightFunction, typename EdgeWeightFunction
    >
std::tuple<size_t, size_t> getImageGraph(
        cv::Rect_<T> const & rect,
        VertexType * vertices,//[vertexCount(rect.size())]
        EdgeType   * edges,   //[edgeCount(rect.size())]
        VertexWeightFunction v,//f(cv::Point)
        EdgeWeightFunction   e //f(cv::Point, cv::Point)
        )
{
    typedef cv::Point_<T> Point;
    // preconditions
    BOOST_ASSERT(rect.width  >= 0);
    BOOST_ASSERT(rect.height >= 0);
    BOOST_ASSERT(vertices);
    BOOST_ASSERT(edges);
    // extract
    size_t vertexCount = 0;
    size_t edgeCount = 0;
    forEachElement(rect,
            [&](Point const & p)
            {
                auto weight = v(p);
                if(weight)
                {
                    VertexType & vert = vertices[vertexCount++];
                    vert.point = p;
                    vert.weight = *weight;
                }
            },
            [&](Point const & a, Point const & b)
            {
                auto weight = e(a, b);
                if(weight)
                {
                    EdgeType & edge = edges[edgeCount++];
                    edge.points[0] = a;
                    edge.points[1] = b;
                    edge.weight = *weight;
                }
            }
        );
    return std::make_tuple(vertexCount, edgeCount);
}

template<typename T, typename W, typename EdgeWeightFunction, typename EdgeWeightFilter>
size_t getHorizontalConnectors(
        cv::Point_<T> const & p, T height,
        Edge<T, W> * edges,//[height]
        EdgeWeightFunction e,//f(cv::Point, cv::Point)
        EdgeWeightFilter f
        )
{
    typedef cv::Point_<T> Point;
    typedef Edge<T, W> Edge;

    T count = 0;
    for(T i = 0; i < height; ++i)
    {
        Point a(p.x  , p.y+i);
        Point b(p.x+1, p.y+i);
        W w = e(a, b);
        if(f(w))
        {
            Edge & edge = edges[count++];
            edge.points[0] = a;
            edge.points[1] = b;
            edge.weight = w;
        }
    }
    return count;
}

template<typename T, typename W, typename EdgeWeightFunction, typename EdgeWeightFilter>
size_t getVerticalConnectors(
        cv::Point_<T> const & p, T width,
        Edge<T, W> * edges,//[width]
        EdgeWeightFunction e,//f(cv::Point, cv::Point)
        EdgeWeightFilter f
        )
{
    typedef cv::Point_<T> Point;
    typedef Edge<T, W> Edge;

    T count = 0;
    for(T i = 0; i < width; ++i)
    {
        Point a(p.x+i, p.y  );
        Point b(p.x+i, p.y+1);
        W w = e(a, b);
        if(f(w))
        {
            Edge & edge = edges[count++];
            edge.points[0] = a;
            edge.points[1] = b;
            edge.weight = w;
        }
    }
    return count;
}

template<typename T, typename W, typename EdgeWeightFunction, typename EdgeWeightFilter>
size_t getSortedHorizontalConnectors(
        cv::Point_<T> const & p, T height,
        Edge<T, W> * edges,
        EdgeWeightFunction e,
        EdgeWeightFilter f
        )
{
    T const count = getHorizontalConnectors(p, height, edges, e, f);
    std::sort(edges, edges+count);
    return count;
}

template<typename T, typename W, typename EdgeWeightFunction, typename EdgeWeightFilter>
size_t getSortedVerticalConnectors(
        cv::Point_<T> const & p, T width,
        Edge<T, W> * edges,
        EdgeWeightFunction e,
        EdgeWeightFilter f
        )
{
    T const count = getVerticalConnectors(p, width, edges, e, f);
    std::sort(edges, edges+count);
    return count;
}

template<typename T, typename EdgeWeightFunction, typename EdgeWeightFilter>
size_t getSortedHorizontalConnectors(
    cv::Point_<T> const & p, T height,
    Edge<T, uint8_t> * edges,//[height]
    EdgeWeightFunction e,//f(cv::Point, cv::Point)
    EdgeWeightFilter f
)
{
    typedef cv::Point_<T> Point;
    typedef Edge<T, uint8_t> Edge;
    // build histogram
    unsigned histogram[256];
    memset(histogram, 0, sizeof(histogram));
    for(T i = 0; i < height; ++i)
    {
        ++histogram[e(Point(p.x, p.y+i), Point(p.x+1, p.y+i))];
    }
    unsigned indices[257];
    indices[0] = 0;
    std::partial_sum(histogram, histogram + 256, indices + 1);
    // extract edges
    for(T i = 0; i < height; ++i)
    {
        Point a(p.x  , p.y+i);
        Point b(p.x+1, p.y+i);
        uint8_t w = e(a, b);
        Edge & edge = edges[indices[w]++];
        edge.points[0] = a;
        edge.points[1] = b;
        edge.weight = w;
    }
    BOOST_ASSERT(indices[255] == indices[256]);
    return indices[256];
}

template<typename T, typename EdgeWeightFunction, typename EdgeWeightFilter>
size_t getSortedVerticalConnectors(
    cv::Point_<T> const & p, T width,
    Edge<T, uint8_t> * edges,//[width]
    EdgeWeightFunction e,//f(cv::Point, cv::Point)
    EdgeWeightFilter f
)
{
    typedef cv::Point_<T> Point;
    typedef Edge<T, uint8_t> Edge;
    // build histogram
    unsigned histogram[256];
    memset(histogram, 0, sizeof(histogram));
    for(T i = 0; i < width; ++i)
    {
        ++histogram[e(Point(p.x+i, p.y), Point(p.x+i, p.y+1))];
    }
    unsigned indices[257];
    indices[0] = 0;
    std::partial_sum(histogram, histogram + 256, indices + 1);
    // extract edges
    for(T i = 0; i < width; ++i)
    {
        Point a(p.x+i, p.y  );
        Point b(p.x+i, p.y+1);
        uint8_t w = e(a, b);
        Edge & edge = edges[indices[w]++];
        edge.points[0] = a;
        edge.points[1] = b;
        edge.weight = w;
    }
    BOOST_ASSERT(indices[255] == indices[256]);
    return indices[256];
}
