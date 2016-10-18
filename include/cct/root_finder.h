#ifndef LIBCCT_ROOT_FINDER_H_INCLUDED
#define LIBCCT_ROOT_FINDER_H_INCLUDED

#include <cstdint>

#include <limits>
#include <memory>
#include <utility>

#include <boost/assert.hpp>
#include <boost/scoped_array.hpp>

namespace cct {

template<typename Size = size_t, typename Rank = uint8_t>
class UnionFind
{
public :
    // leaf count
    using size_type = Size;
    // leaf index - [0,leaf count[
    using index_type = Size;
    using handle_type = Size;

    static constexpr auto max_size = std::numeric_limits<size_type>::max();

    UnionFind() noexcept : m_size(0) {}

    explicit UnionFind(size_type n)
        : m_parents(new index_type[n])
        , m_ranks(new Rank[n])
        , m_size(n)
    {
        BOOST_ASSERT(n < max_size);
        for(size_type i = 0; i < n; ++i)
            m_parents[i] = i;
        std::fill_n(m_ranks.get(), n, 0);
    }

    UnionFind(UnionFind &&) = default;
    UnionFind(UnionFind const &) = delete;

    UnionFind & operator=(UnionFind &&) = default;
    UnionFind & operator=(UnionFind const &) = delete;

    ~UnionFind() = default;

    size_type size() const noexcept
    {
        return m_size;
    }

    handle_type find(index_type i) const noexcept
    {
        BOOST_ASSERT(i < m_size);
        while(m_parents[i] != i)
        {
            i = m_parents[i];
        }
        return i;
    }

    void update(index_type i, handle_type h) noexcept
    {
        BOOST_ASSERT(i < m_size);
        BOOST_ASSERT(h < m_size);
        BOOST_ASSERT(m_parents[h] == h);// h is root
        while(m_parents[i] != i)
        {
            i = std::exchange(m_parents[i], h);
        }
        BOOST_ASSERT(i == h);
    }

    handle_type merge(handle_type a, handle_type b) noexcept
    {
        BOOST_ASSERT(a < m_size);
        BOOST_ASSERT(b < m_size);
        BOOST_ASSERT(m_parents[a] == a);
        BOOST_ASSERT(m_parents[b] == b);
        if(m_ranks[a] < m_ranks[b])
        {
            std::swap(a,b);
        }
        else if(m_ranks[a] == m_ranks[b])
        {
            BOOST_ASSERT(m_ranks[a] < std::numeric_limits<Rank>::max());
            ++m_ranks[a];
        }
        m_parents[b] = a;
        return a;
    }

    handle_type find_update(index_type i) noexcept
    {
        auto h = find(i);
        update(i, h);
        return h;
    }
private :
    std::unique_ptr<index_type[]> m_parents;
    std::unique_ptr<Rank[]> m_ranks;
    size_type m_size;
};

template<typename Size = size_t>
class PackedUnionFind
{
public :
    // leaf count
    using size_type = Size;
    // leaf index - [0,leaf count[
    using index_type = Size;
    using handle_type = Size;

    static constexpr auto max_size
        = std::numeric_limits<size_type>::max() - sizeof(size_t)*CHAR_BIT;

    PackedUnionFind() noexcept : m_size(0) {}

    explicit PackedUnionFind(size_type n)
        : m_parents(new index_type[n])
        , m_size(n)
    {
        BOOST_ASSERT(n < max_size);
        for(size_type i = 0; i < n; ++i)
            m_parents[i] = n;
    }

    PackedUnionFind(PackedUnionFind &&) = default;
    PackedUnionFind(PackedUnionFind const &) = delete;

    PackedUnionFind & operator=(PackedUnionFind &&) = default;
    PackedUnionFind & operator=(PackedUnionFind const &) = delete;

    ~PackedUnionFind() = default;

    size_type size() const noexcept
    {
        return m_size;
    }

    handle_type find(index_type i) const noexcept
    {
        BOOST_ASSERT(i < m_size);
        while(m_parents[i] < m_size)
        {
            i = m_parents[i];
        }
        return i;
    }

    void update(index_type i, handle_type h) noexcept
    {
        BOOST_ASSERT(i < m_size);
        BOOST_ASSERT(h < m_size);
        BOOST_ASSERT(m_parents[h] >= m_size);
        while(m_parents[i] < m_size)
        {
            i = std::exchange(m_parents[i], h);
        }
        BOOST_ASSERT(i == h);
    }

    handle_type merge(handle_type a, handle_type b) noexcept
    {
        BOOST_ASSERT(a != b);
        BOOST_ASSERT(a < m_size);
        BOOST_ASSERT(b < m_size);
        BOOST_ASSERT(m_parents[a] >= m_size);
        BOOST_ASSERT(m_parents[b] >= m_size);
        if(m_parents[a] < m_parents[b]) 
        {
            std::swap(a,b);
        }
        else if(m_parents[a] == m_parents[b])
        {
            if(a > b)
                std::swap(a, b);
            BOOST_ASSERT(m_parents[a] < std::numeric_limits<size_type>::max());
            ++m_parents[a];
        }
        m_parents[b] = a;
        return a;
    }

    handle_type find_update(index_type i) noexcept
    {
        auto h = find(i);
        update(i, h);
        return h;
    }
private :
    std::unique_ptr<index_type[]> m_parents;
    size_type m_size;
};

template<typename Data, typename UnionFind>
class RootFinder
    : public UnionFind
{
public :
    using size_type = typename UnionFind::size_type;
    using index_type = typename UnionFind::index_type;
    using handle_type = typename UnionFind::handle_type;

    /** Access node data.
     * O(1)
     */
    Data & data(handle_type h) noexcept
    {
        BOOST_ASSERT(h < this->size());
        return m_data[h];
    }
    Data const & data(handle_type h) const noexcept
    {
        BOOST_ASSERT(h < this->size());
        return m_data[h];
    }

    RootFinder() = default;

    template<typename F>
    explicit RootFinder(size_type n, F f = F())
        : UnionFind(n)
    {
        for(size_type i = 0; i < this->size(); ++i)
        {
            data(i) = f(i);
        }
    }

    ~RootFinder() = default;
private :
    std::unique_ptr<Data[]> m_data;
};


/** Union-find implementation
 */
template<
    typename Data, //associated to tree node
    typename Index = size_t //leaf index
    >
class PackedRootFinder
{
private :
    static constexpr size_t max_count = SIZE_MAX - sizeof(size_t)*CHAR_BIT;

    size_t count;
    //parents[i] < count -> child
    //parents[i] >= count -> root, rank = parents[i]-count
    boost::scoped_array<Index> parents;
    boost::scoped_array<Data>  datas;
/*    Data  * datas;
    // Both parents and datas have the same lifetime, memory is allocated in one piece
    boost::scoped_array<char> memory;*/
public :
    typedef Index Handle;

    /** Access node data.
     * O(1)
     */
    Data & data(Handle h) noexcept
    {
        BOOST_ASSERT(h < count);
        return datas[h];
    }
    Data const & data(Handle h) const noexcept
    {
        BOOST_ASSERT(h < count);
        return datas[h];
    }

    template<typename F>
    F resetRange(Index b, Index e, F f = F())
    {
        for(Index i = b; i < e; ++i)
        {
            parents[i] = count;
            datas  [i] = f(i);
        }
        return f;
    }

    template<typename F>
    void reset(F f = F())
    {
        resetRange(0, count, std::move(f));
    }

    void kill() noexcept
    {
        count = 0;
        parents.reset();
        datas.reset();
//        parents = nullptr;
//        datas   = nullptr;
//        memory.reset();
    }

    template<typename F>
    void init(size_t c, F f)
    {
        BOOST_ASSERT(c <= max_count);
        if(count != c)
        {
            kill();
            parents.reset(new Index[c]);
            datas.reset(new Data[c]);
            // calculate required size
/*            size_t const alignment = 64;//TODO
            size_t const parents_size
                = utils::alignSize(c*sizeof(Index), alignment);
            size_t const size = alignment-1
                + parents_size
                + utils::alignSize(c*sizeof(Data ), alignment);
            // allocate memory
            memory.reset(new char[size]);
            char * ptr = utils::alignPtr(memory.get(), alignment);
            parents = reinterpret_cast<Index*>(ptr);
            ptr += parents_size;
            datas = reinterpret_cast<Data *>(ptr);*/
            count = c;
        }
        reset(std::move(f));
    }

    PackedRootFinder() : count(0), parents(nullptr), datas(nullptr) {}

    template<typename F>
    PackedRootFinder(size_t c, F f = F())
        : PackedRootFinder()
    {
        init(c, std::move(f));
    }

    ~PackedRootFinder() = default;

    /** Traverse tree from leaf to root. Returns root handle.
     * O(alpha(count))
     */
    Handle find(Index i)
    {
        BOOST_ASSERT(i < count);
        while(parents[i] < count)
        {
            i = parents[i];
        }
        return i;
    }

    /** Update path compression
     * O(alpha(count))
     */
    void update(Index i, Handle h)
    {
        BOOST_ASSERT(i < count);
        BOOST_ASSERT(h < count);
        while(parents[i] < count)
        {
            Index tmp = parents[i];
            parents[i] = h;
            i = tmp;
        }
    }

    Handle find_update(Index i)
    {
        Handle h = find(i);
        update(i, h);
        return h;
    }

    /** Merge two subtrees, return handle to new root.
     * O(1)
     */
    Handle merge(Handle a, Handle b)
    {
        BOOST_ASSERT(a < count);
        BOOST_ASSERT(b < count);
        BOOST_ASSERT(parents[a] >= count);
        BOOST_ASSERT(parents[b] >= count);
        if(parents[a] < parents[b])
        {
            std::swap(a,b);
        }
        else if(parents[a] == parents[b])
        {
            BOOST_ASSERT(parents[a] < std::numeric_limits<Index>::max());
            ++parents[a];
        }
        parents[b] = a;
        return a;
    }

    Handle merge_update(Handle a, Handle b, Index i)
    {
        Handle h = merge(a, b);
        update(i, h);
        return h;
    }

    Handle merge_set(Handle a, Handle b, Data const & d)
    {
        Handle h = merge(a, b);
        data(h) = d;
        return h;
    }
};

}//namespace cct

#endif//LIBCCT_ROOT_FINDER_H_INCLUDED
