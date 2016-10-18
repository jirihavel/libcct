#define ASSERT_SIZE(s) do {        \
    BOOST_ASSERT((s).width  >= 0); \
    BOOST_ASSERT((s).height >= 0); } while(false)

#define ASSERT_RECT(r) do {        \
    BOOST_ASSERT((r).width  >= 0); \
    BOOST_ASSERT((r).height >= 0); } while(false)

#define ASSERT_POINT_IN_SIZE(p, s) do { \
    ASSERT_SIZE(s);                     \
    BOOST_ASSERT((p).x >= 0);           \
    BOOST_ASSERT((p).y >= 0);           \
    BOOST_ASSERT((p).x < (s).width );   \
    BOOST_ASSERT((p).y < (s).height); } while(false)

#define ASSERT_RECT_IN_SIZE(r, s) do {            \
    ASSERT_RECT(r); ASSERT_SIZE(s);               \
    BOOST_ASSERT((r).x >= 0);                     \
    BOOST_ASSERT((r).y >= 0);                     \
    BOOST_ASSERT((r).width  >= 0);                \
    BOOST_ASSERT((r).height >= 0);                \
    BOOST_ASSERT((r).x+(r).width  <= (s).width ); \
    BOOST_ASSERT((r).y+(r).height <= (s).height); } while(false)

template<typename T>
inline T vertCount(cv::Size_<T> const & s) noexcept
{
    ASSERT_SIZE(s);
    return s.width*s.height;
}

template<typename T>
inline T vertexCount(cv::Size_<T> const & s) noexcept
{
    return vertCount(s);
}

template<typename T>
inline T edgeCount(cv::Size_<T> const & s, Connectivity connectivity) noexcept
{
    ASSERT_SIZE(s);

    if(s.area() == 0)
        return 0;

    switch(connectivity)
    {
    case Connectivity::C4 :
        return s.width*(s.height-1) + (s.width-1)*s.height;
    case Connectivity::C6N :
    case Connectivity::C6P :
        return s.width*(s.height-1) + (s.width-1)*s.height + (s.width-1)*(s.height-1);
    default :
        BOOST_ASSERT(connectivity == Connectivity::C8);
        return s.width*(s.height-1) + (s.width-1)*s.height + 2*(s.width-1)*(s.height-1);
    };
}

template<typename T>
inline T pointId(cv::Point_<T> const & point, cv::Size_<T> const & size) noexcept
{
    ASSERT_POINT_IN_SIZE(point, size);
    return point.x + point.y*size.width;
}

template<typename T>
inline cv::Point_<T> idToPoint(T id, cv::Size_<T> const & size) noexcept
{
    cv::Point_<T> const point(id%size.width, id/size.width);
    ASSERT_POINT_IN_SIZE(point, size);
    return point;
}

//------------------------
// -- Graph extraction --
//------------------------

namespace detail {

/** \brief Handle default rectangle.
 */    
template<typename T>
cv::Rect_<T> adjustRect(cv::Rect_<T> const & rect, cv::Size_<T> const & size) noexcept
{
    ASSERT_SIZE(size);
    return rect.area() > 0 ? rect : cv::Rect_<T>({0, 0}, size);
}

template<typename T>
size_t vertCount(cv::Rect_<T> const & rect, cv::Size_<T> const & size) noexcept
{
    ASSERT_SIZE(size);
    return ::cct::img::vertCount<size_t>(rect.area() > 0 ? rect.size() : size);
}

template<typename T>
size_t edgeCount(cv::Rect_<T> const & rect, cv::Size_<T> const & size, Connectivity connectivity) noexcept
{
    ASSERT_SIZE(size);
    return ::cct::img::edgeCount<size_t>(rect.area() > 0 ? rect.size() : size, connectivity);
}

// conn&flag == flag for connectivity flags
bool constexpr connHas(Connectivity conn, Connectivity flag) noexcept
{
    return (underlying_value(conn) & underlying_value(flag))
        == underlying_value(flag);
}

template<typename F, typename T>
void forEachVertex(
        cv::Rect_<T> const & rect,
        cv::Size_<T> const & size,
        F f
    )
{
    ASSERT_RECT_IN_SIZE(rect, size);
    if(rect.area() > 0)
    {
        size_t i = rect.y*size.width;
        for(T y = rect.y; y < (rect.y+rect.height); ++y)
        {
            size_t j = i + rect.x;
            for(T x = rect.x; x < (rect.x+rect.width); ++x)
            {
                f(cv::Point_<T>{x,y}, j++);
            }
            i += size.width;
        }
    }
}

template<Connectivity C, typename V, typename E, typename T>
void forEachElement(
    cv::Rect_<T> const & rect,
    cv::Size_<T> const & size,
    V v, E e)
{
    ASSERT_RECT_IN_SIZE(rect, size);
    if(rect.area() > 0)
    {
        using Point = cv::Point_<T>;
        T y = rect.y;
        size_t i = rect.x + static_cast<size_t>(rect.y)*size.width;
        for(T h = rect.height; h > 1; ++y, i += size.width, --h)
        {
            T x = rect.x;
            size_t j = i;
            for(T w = rect.width; w > 1; ++x, ++j, --w)
            {
                v(Point{x,y}, j);
                e(Point{x,y}, j, Point{x+1,y  }, j+1);// horizontal
                e(Point{x,y}, j, Point{x  ,y+1}, j+size.width);// vertical
                if(connHas(C, Connectivity::C6P))
                    e(Point{x,y}, j, Point{x+1,y+1}, j+size.width+1);
                if(connHas(C, Connectivity::C6N))
                    e(Point{x+1,y}, j+1, Point{x,y+1}, j+size.width);
            }
            // last col pixel and vertical edge
            v(Point{x,y}, j);
            e(Point{x,y}, j, Point{x,y+1}, j+size.width);
        }
        // last row
        T x = rect.x;
        for(; x < (rect.x+rect.width-1); ++x, ++i)
        {
            v(Point{x,y}, i);
            e(Point{x,y}, i, Point{x+1,y}, i+1);// horizontal edge
        }
        v(Point{x,y}, i);// bottom right pixel
    }
}

}//namespace detail

template<typename F, typename T>
void forEachVertex(
        cv::Size_<T> const & size,
        F f,
        cv::Rect_<T> const & rect
    )
{
    detail::forEachVertex(detail::adjustRect(rect, size), size, std::move(f));
}

template<typename V, typename E, typename T>
void forEachElement(
        cv::Size_<T> const & size,
        V v = V(), E e = E(),
        cv::Rect_<T> const & rect,
        Connectivity connectivity
    )
{
    cv::Rect_<T> r = detail::adjustRect(rect, size);
    switch(connectivity)
    {
        case Connectivity::C4 :
            return detail::forEachElement<Connectivity::C4 >(r, size, std::move(v), std::move(e));
        case Connectivity::C6P :
            return detail::forEachElement<Connectivity::C6P>(r, size, std::move(v), std::move(e));
        case Connectivity::C6N :
            return detail::forEachElement<Connectivity::C6N>(r, size, std::move(v), std::move(e));
        case Connectivity::C8 :
            return detail::forEachElement<Connectivity::C8 >(r, size, std::move(v), std::move(e));
        default :
            throw std::runtime_error("Invalid connectivity");
    }
}

template<typename F, typename T>
void forEachEdge(
        cv::Size_<T> const & size,
        F f = F(),
        cv::Rect_<T> const & rect,
        Connectivity connectivity
    )
{
    forEachElement(size, utils::fp::ignore(), std::move(f), rect, connectivity);
}

namespace detail {

template<typename T>
class BaseExtractor
{
protected :
    BaseExtractor(T * data, size_t cap) noexcept
        : m_data(data), m_count(0), m_capacity(cap) {}

    BaseExtractor(BaseExtractor const &) = delete;
    BaseExtractor & operator=(BaseExtractor const &) = delete;

    T & add(typename T::Weight) noexcept
    {
        BOOST_ASSERT(m_count < m_capacity);
        return m_data[m_count++];
    }
public :
    size_t finish() noexcept
    {
        m_data = nullptr;
        m_capacity = 0;
        return std::exchange(m_count, 0);
    }
protected :
    T * m_data;
    size_t m_count;
    size_t m_capacity;
};

template<typename T, typename Converter, typename Unsigned>
class BaseSorterTemplate
    : public BaseExtractor<T>
{
    using Base = BaseExtractor<T>;
protected :
    BaseSorterTemplate(T * data, size_t cap) noexcept
        : Base(data, cap) {}
public :
    size_t finish() noexcept 
    {
        std::sort(Base::m_data, Base::m_data+Base::m_count);
        return Base::finish();
    }
};

template<typename T, typename Conv>
class BaseSorterTemplate<T, Conv, uint8_t>
    : private Conv, public BaseExtractor<T>
{
    using Base = BaseExtractor<T>;
protected :
    BaseSorterTemplate(T * data, size_t cap) noexcept
        : Base(data, cap), m_aux(new T[cap])
    {
        std::fill_n(histogram, UINT8_MAX+1, 0);
    }

    T & add(typename T::Weight w) noexcept
    {
        BOOST_ASSERT(Base::m_count < Base::m_capacity);
        ++histogram[Conv::operator()(w)];
        return m_aux[Base::m_count++];
    }
public :
    size_t finish() noexcept 
    {
        // prefix sum
        size_t indices[UINT8_MAX+2];
        indices[0] = 0;
        std::partial_sum(histogram, histogram+UINT8_MAX+1, indices+1);
        BOOST_ASSERT(indices[UINT8_MAX+1] == Base::m_count);
        // reorder
        for(size_t i = 0; i < Base::m_count; ++i)
            Base::m_data[indices[Conv::operator()(m_aux[i].weight)]++] = m_aux[i];
        BOOST_ASSERT(indices[UINT8_MAX] == Base::m_count);
        // cleanup
        m_aux.reset();
        return Base::finish();
    }
private :
    std::unique_ptr<T[]> m_aux;
    size_t histogram[UINT8_MAX+1];
};

template<typename T, typename Conv>
class BaseSorterTemplate<T, Conv, uint16_t>
    : private Conv, public BaseExtractor<T>
{
    using Base = BaseExtractor<T>;
protected :
    BaseSorterTemplate(T * data, size_t cap) noexcept
        : Base(data, cap), m_aux(new T[cap])
    {
        std::fill_n(histogram, UINT16_MAX+1, 0);
    }

    T & add(typename T::Weight w) noexcept
    {
        BOOST_ASSERT(Base::m_count < Base::m_capacity);
        ++histogram[Conv::operator()(w)];
        return m_aux[Base::m_count++];
    }
public :
    size_t finish() noexcept 
    {
        // prefix sum
        size_t indices[UINT16_MAX+2];
        indices[0] = 0;
        std::partial_sum(histogram, histogram+UINT16_MAX+1, indices+1);
        BOOST_ASSERT(indices[UINT16_MAX+1] == Base::m_count);
        // reorder
        for(size_t i = 0; i < Base::m_count; ++i)
            Base::m_data[indices[Conv::operator()(m_aux[i].weight)]++] = m_aux[i];
        BOOST_ASSERT(indices[UINT16_MAX] == Base::m_count);
        // cleanup
        m_aux.reset();
        return Base::finish();
    }
private :
    std::unique_ptr<T[]> m_aux;
    size_t histogram[UINT16_MAX+1];
};

template<typename T>
using BaseSorter = BaseSorterTemplate<T,
      utils::uint_conv<typename T::Weight>,
      typename utils::uint_conv<typename T::Weight>::uint_type>;

template<
    template<typename> class B,//base facet
    typename F, typename T, typename V, typename W>
class VertExtractorTemplate
    : private F, public B<Vertex<V,W>>
{
    using Vert = Vertex<V,W>;
    using Base = B<Vert>;

    void add(V i, W w)
    {
        auto & v = Base::add(w);
        v.id = i;
        v.weight = w;
    }

    template<typename X>
    void add(V i, boost::optional<X> const & w)
    {
        if(w) add(i, *w);
    }
public :
    VertExtractorTemplate(F f, Vert * verts, size_t cap)
        : F(std::move(f)), Base(verts, cap) {}

    void operator()(cv::Point_<T> const & p, size_t i)
    {
        BOOST_ASSERT(i <= std::numeric_limits<V>::max());
        add(i, F::operator()(p));
    }
};

template<typename F, typename T, typename V, typename W>
using VertExtractor = VertExtractorTemplate<BaseExtractor, F,T,V,W>;

template<typename F, typename T, typename V, typename W>
using VertSorter = VertExtractorTemplate<BaseSorter, F,T,V,W>;

template<
    template<typename> class B,//base facet
    typename F, typename T, typename V, typename W>
class EdgeExtractorTemplate
    : private F, public B<Edge<V,W>>
{
    using Point = cv::Point_<T>;
    using Edge = Edge<V,W>;
    using Base = B<Edge>;

    void add(V ia, V ib, W w)
    {
        auto & e = Base::add(w);
        e.vertices[0] = ia;
        e.vertices[1] = ib;
        e.weight = w;
    }

    template<typename X>
    void add(V ia, V ib, boost::optional<X> const & w)
    {
        if(w) add(ia, ib, *w);
    }
public :
    EdgeExtractorTemplate(F f, Edge * edges, size_t cap)
        : F(std::move(f)), Base(edges, cap) {}

    void operator()(Point const & pa, size_t ia, Point const & pb, size_t ib)
    {
        BOOST_ASSERT(ia <= std::numeric_limits<V>::max());
        BOOST_ASSERT(ib <= std::numeric_limits<V>::max());
        add(ia, ib, F::operator()(pa, pb));
    }
};

template<typename F, typename T, typename V, typename W>
using EdgeExtractor = EdgeExtractorTemplate<BaseExtractor, F,T,V,W>;

template<typename F, typename T, typename V, typename W>
using EdgeSorter = EdgeExtractorTemplate<BaseSorter, F,T,V,W>;

}//namespace detail

template<typename F, typename T, typename V, typename W>
size_t getImageVertices(
        cv::Size_<T> const & size,
        Vertex<V, W> * verts,
        F f,
        cv::Rect_<T> const & rect
    )
{
    detail::VertExtractor<F,T,V,W> extractor(std::move(f), verts, detail::vertCount(rect, size));
    forEachVertex(size, std::ref(extractor), rect);
    return extractor.finish();
}

template<typename F, typename T, typename V, typename W>
size_t getSortedImageVertices(
        cv::Size_<T> const & size,
        Vertex<V, W> * verts,
        F f = F(),
        cv::Rect_<T> const & rect
    )
{
    detail::VertSorter<F,T,V,W> extractor(std::move(f), verts, detail::vertCount(rect, size));
    forEachVertex(size, std::ref(extractor), rect);
    return extractor.finish();
}

template<
    template<typename,int> class F,
    typename T,
    typename C, int N,
    typename V, typename W>
size_t getImageVertices(
        cv::Mat_<cv::Vec<C,N>> const & image,
        Vertex<V,W> * verts,
        cv::Rect_<T> const & rect
    )
{
    using Func = F<C,N>;
    return getImageVertices<Func,T>(image.size(), verts, Func(image), rect);
}

//-----------------------
// -- Edge extraction --
//-----------------------

template<typename F, typename T, typename V, typename W>
size_t getImageEdges(
        cv::Size_<T> const & size,
        Edge<V, W> * edges,
        F f,
        cv::Rect_<T> const & rect,
        Connectivity connectivity
    )
{
    detail::EdgeExtractor<F,T,V,W> extractor(std::move(f), edges, detail::edgeCount(rect, size, connectivity));
    forEachEdge(size, std::ref(extractor), rect, connectivity);
    return extractor.finish();
}

template<typename F, typename T, typename V, typename W>
size_t getSortedImageEdges(
        cv::Size_<T> const & size,
        Edge<V, W> * edges,
        F f,
        cv::Rect_<T> const & rect,
        Connectivity connectivity
    )
{
    detail::EdgeSorter<F,T,V,W> extractor(std::move(f), edges, detail::edgeCount(rect, size, connectivity));
    forEachEdge(size, std::ref(extractor), rect, connectivity);
    return extractor.finish();
}

template<
    template<typename,int> class F,
    typename T,
    typename C, int N,
    typename V, typename W>
size_t getImageEdges(
        cv::Mat_<cv::Vec<C,N>> const & image,
        Edge<V,W> * edges,
        cv::Rect_<T> const & rect,
        Connectivity connectivity
    )
{
    return getImageEdges(cv::size_cast<T>(image.size()), edges, F<C,N>(image), rect, connectivity);
}

template<
    template<typename,int> class F,
    typename T,
    typename C, int N,
    typename V, typename W>
size_t getSortedImageEdges(
        cv::Mat_<cv::Vec<C,N>> const & image,
        Edge<V,W> * edges,
        cv::Rect_<T> const & rect,
        Connectivity connectivity
    )
{
    return getSortedImageEdges(cv::size_cast<T>(image.size()), edges, F<C,N>(image), rect, connectivity);
}

namespace detail {

template<
    template<typename,int> class F,
    typename T,
    typename V, typename W>
struct GetImageEdges
{
    template<typename C, int N>
    struct Worker
    {
        size_t operator()(
                cv::Mat const & image,
                Edge<V, W> * edges,
                cv::Rect_<T> const & rect,
                Connectivity connectivity)
        {
            return getImageEdges(
                    cv::size_cast<T>(image.size()), edges, F<C,N>(image),
                    rect, connectivity); 
        }
    };
};

template<
    template<typename,int> class F,
    typename T,
    typename V, typename W>
struct GetSortedImageEdges
{
    template<typename C, int N>
    struct Worker
    {
        size_t operator()(
                cv::Mat const & image,
                Edge<V, W> * edges,
                cv::Rect_<T> const & rect,
                Connectivity connectivity)
        {
            return getSortedImageEdges(
                    cv::size_cast<T>(image.size()), edges, F<C,N>(image),
                    rect, connectivity); 
        }
    };
};

}//namespace detail
    
template<
    template<typename,int> class F,
    typename T,
    typename V, typename W>
size_t getImageEdges(
        cv::Mat const & image, Edge<V, W> * edges,
        cv::Rect_<T> const & rect,
        Connectivity connectivity
)
{
    return cv::pixelTypeSwitch<detail::GetImageEdges<F,T,V,W>::template Worker>(image, edges, rect, connectivity);
}

template<
    template<typename,int> class F,
    typename T,
    typename V, typename W>
size_t getSortedImageEdges(
        cv::Mat const & image, Edge<V, W> * edges,
        cv::Rect_<T> const & rect,
        Connectivity connectivity
    )
{
    return cv::pixelTypeSwitch<detail::GetSortedImageEdges<F,T,V,W>::template Worker>(image, edges, rect, connectivity);
}

// -- Graph extraction --

template<typename V, typename E, typename T, typename I, typename VW, typename EW>
std::pair<size_t, size_t> getImageGraph(
        cv::Size_<T> const & size,
        Vertex<I, VW> * verts,//[vertexCount(rect.size())]
        Edge<I, EW> * edges,//[edgeCount(rect.size())]
        V v, E e,
        cv::Rect_<T> const & rect,
        Connectivity connectivity
    )
{
    detail::VertExtractor<V,T,I,VW> vert_extractor(std::move(v), verts, detail::vertCount(rect, size));
    detail::EdgeExtractor<E,T,I,EW> edge_extractor(std::move(e), edges);
    forEachElement(size, std::ref(vert_extractor), std::ref(edge_extractor), rect, connectivity);
    return std::make_pair(vert_extractor.finish(), edge_extractor.finish());
}

template<typename V, typename E, typename T, typename I, typename VW, typename EW>
std::pair<size_t, size_t> getSortedImageGraph(
        cv::Size_<T> const & size,
        Vertex<I, VW> * verts,//[vertexCount(rect.size())]
        Edge<I, EW> * edges,//[edgeCount(rect.size())]
        V v = V(), E e = E(),
        cv::Rect_<T> const & rect,
        Connectivity connectivity
    )
{
    detail::VertSorter<V,T,I,VW> vert_extractor(std::move(v), verts, detail::vertCount(rect, size));
    detail::EdgeExtractor<E,T,I,EW> edge_extractor(std::move(e), edges);
    forEachElement(size, std::ref(vert_extractor), std::ref(edge_extractor), rect, connectivity);
    size_t const n = edge_extractor.finish();
    std::sort(edges, edges+n);
    return std::make_pair(vert_extractor.finish(), n);
}

// ---------------------------------------------------------------------------

template<
    template<typename> class Builder,
    typename EdgeWeightFunction,
    typename T,
    typename Tree>
std::pair<size_t, typename Tree::RootCount>
    buildTree(
        cv::Size_<T> const & size,
        Tree & tree,
        EdgeWeightFunction e,
        typename Tree::CompHandle * edge_comps,
        Connectivity connectivity
    )
{
    if((size.width > 0) && (size.height > 0))
    {
        typedef cv::Point_<T> Point;
        typedef decltype(e(Point(),Point())) Weight;
        typedef Edge<typename Tree::LeafIndex, Weight> Edge;

        std::unique_ptr<Edge[]> edges(new Edge[edgeCount<size_t>(size, connectivity)]);
        auto const edge_count = getSortedImageEdges(size, edges.get(), e,
                CCT_WHOLE_IMAGE_RECT, connectivity);

        auto const root_count = buildTree<Builder>(tree,
                vertexCount<typename Tree::LeafCount>(size),
                edges.get(), edge_count, edge_comps);

        return std::make_pair(edge_count, root_count);
    }
    else
    {
        tree.kill();
        return std::make_pair(0, 0);
    }
}

template<
    template<typename> class Builder,
    typename VertexWeightFunction, typename EdgeWeightFunction,
    typename T,
    typename Tree>
std::pair<size_t, typename Tree::RootCount>
    buildTree(
        cv::Size_<T> const & size,
        Tree & tree,
        VertexWeightFunction v, EdgeWeightFunction e,
        typename Tree::CompHandle * edge_comps,
        Connectivity connectivity
    )
{
    if(size.area() > 0)
    {
        typedef cv::Point_<T> Point;
        typedef decltype(v(Point())) VertWeight;
        typedef decltype(e(Point(),Point())) EdgeWeight;
        using Vert = Vertex<typename Tree::LeafIndex, VertWeight>;
        using Edge = Edge<typename Tree::LeafIndex, EdgeWeight>;

        std::unique_ptr<Vert[]> verts(new Vert[vertexCount<size_t>(size)]);
        std::unique_ptr<Edge[]> edges(new Edge[edgeCount<size_t>(size, connectivity)]);
        auto const counts = getSortedImageGraph(size,
                verts.get(), edges.get(), std::move(v), std::move(e),
                CCT_WHOLE_IMAGE_RECT, connectivity);

        auto const root_count = buildTree<Builder>(tree,
                vertexCount<typename Tree::LeafCount>(size),
                verts.get(), counts.first, edges.get(), counts.second,
                edge_comps);

        return std::make_pair(counts.second, root_count);
    }
    else
    {
        tree.kill();
        return std::make_pair(0, 0);
    }
}

template<
    template<typename> class Builder,
    typename WeightFunction,
    typename T,
    typename Tree>
std::pair<size_t, typename Tree::RootCount>
    buildTree2(
        cv::Size_<T> const & size,
        Tree & tree,
        WeightFunction w,
        typename Tree::CompHandle * edge_comps,
        Connectivity connectivity
    )
{
    return buildTree<Builder>(size, tree, std::ref(w), std::ref(w), edge_comps, connectivity);
}

template<
    template<typename> class Builder,
    template<typename,int> class W,
    typename T,
    typename Mat,
    typename Tree>
std::pair<size_t, typename Tree::RootCount>
    buildTree(
        Mat const & image,
        Tree & tree,
        typename Tree::CompHandle * edge_comps,
        Connectivity connectivity
    )
{
    if(image.size().area() > 0)
    {
        typedef Edge<typename Tree::LeafIndex, typename Tree::Level> Edge;

        std::unique_ptr<Edge[]> edges(new Edge[edgeCount<size_t>(image.size(), connectivity)]);
        auto const edge_count = getSortedImageEdges<W,T>(image, edges.get(),
                CCT_WHOLE_IMAGE_RECT, connectivity);

        auto const root_count = buildTree<Builder>(tree,
                vertexCount<typename Tree::LeafCount>(image.size()),
                edges.get(), edge_count, edge_comps);

        return std::make_pair(edge_count, root_count);
    }
    else
    {
        tree.kill();
        return std::make_pair(0, 0);
    }
}
