#ifndef CONNECTED_COMPONENT_TREE_ROOT_FINDER_H_INCLUDED
#define CONNECTED_COMPONENT_TREE_ROOT_FINDER_H_INCLUDED

#include "utils/align.h"

#include <cstdint>

#include <limits>
#include <memory>

#include <boost/assert.hpp>
#include <boost/scoped_array.hpp>

namespace cct {

#if 0

template<
    typename Data,
    typename Size = size_t,
    Index MaxSize = std::numeric_limits<Index>::max() - sizeof(Index)*CHAR_BITS
>
class union_find
{
    typedef Data data_type;
    typedef Size size_type;
    typedef uint8_t rank_type;

    // Index identifies leaf
    // Handle identifies canonical element
    typedef size_type index_type;
    typedef size_type handle_type;

    void reset(data_type const & d = data_type());// n <= MaxSize

    //template<ForwardIterator>
    //void reset(ForwardIterator i);

    void init(size_type n, data_type const & d = data_type());// n <= MaxSize

    //template<ForwardIterator>
    //void init(size_type n, ForwardIterator i);

    void kill();

    // Union find stuff

    handle_type find(index_type i);
    void update(index_type i, handle_type h);
    rank_type rank(handle_type h);
    handle_type union(handle_type a, handle_type b)
    {
    }

    Handle find_update(Index i)
    {
        Handle h = find(i);
        update(i, h);
        return h;
    }

    // ha = find_update(a);
    // hb = find(b);
    // if(ha != hb)
    //     ha = union_update(a, ha, b, hb);
    // else
    //     update(b, hb);
    Handle union_update(Index a, Handle ha, Index b, Handle hb)
    {
        Handle h = union(ha, hb);
        //update(a, ha);
        // already updated, just set
        parents[a] = ha;
        update(b, hb);
        return h;
    }

    Data & data(Handle h);
};

/** Union-find implementation
 */
template<
    typename Data, //associated to tree node
    typename Index = size_t, //leaf index
    typename Rank = uint8_t// tree height will be log_2 of index count at most
    >
class RootFinder
{
private :
    size_t count;
    Index * parents;
    Rank  * ranks;
    Data  * datas;
    //std::unique_ptr<char[]> memory;
    char * memory;
public :
    typedef Index Handle;

    void resetRange(Index b, Index e, Data const & d)
    {
        for(Index i = b; i < e; ++i)
        {
            parents[i] = i;
            ranks  [i] = 0;
            datas  [i] = d;
        }
    }

    void reset(Data const & d)
    {
        resetRange(0, count, d);
    }

    RootFinder() : count(0), parents(nullptr), ranks(nullptr), datas(nullptr), memory(nullptr) {}

    RootFinder(size_t c, Data const & d)
        : count(0), parents(nullptr), ranks(nullptr), datas(nullptr), memory(nullptr)
    {
        init(c, d);
    }

    ~RootFinder() noexcept
    {
        delete [] memory;
    }

    void kill() noexcept
    {
        count = 0;
        parents = nullptr;
        ranks   = nullptr;
        datas   = nullptr;
        //memory.reset();
        delete [] memory;
        memory = nullptr;
    }

    void init(Index c, Data const & d)
    {
        if(count != c)
        {
            kill();
            count = c;
            size_t const alignment = 64;
            size_t size = alignment-1
                + utils::alignSize(count*sizeof(Index), alignment)
                + utils::alignSize(count*sizeof(Rank ), alignment)
                + utils::alignSize(count*sizeof(Data ), alignment);
            //memory.reset(new char[size]);
            delete [] memory;
            memory = nullptr;
            memory = new char[size];

            char * ptr = utils::alignPtr(memory, alignment);
            parents = reinterpret_cast<Index*>(ptr);
            ptr += utils::alignSize(count*sizeof(Index), alignment);
            ranks   = reinterpret_cast<Rank *>(ptr);
            ptr += utils::alignSize(count*sizeof(Rank ), alignment);
            datas   = reinterpret_cast<Data *>(ptr);
        }
        reset(d);
    }

    /** Access node data.
     * O(1)
     */
    Data & data(Handle h)
    {
        BOOST_ASSERT(h < count);
        return datas[h];
    }

    /** Traverse tree from leaf to root. Returns root handle.
     * O(alpha(count))
     */
    Handle find(Index i)
    {
        BOOST_ASSERT(i < count);
        while(parents[i] != i)
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
        while(parents[i] != i)
        {
            Index tmp = parents[i];
            parents[i] = h;
            i = tmp;
        }
    }

    /** Merge two subtrees, return handle to new root.
     * O(1)
     */
    Handle merge(Handle a, Handle b)
    {
        BOOST_ASSERT(a < count);
        BOOST_ASSERT(b < count);
        if(ranks[a] < ranks[b])
        {
            std::swap(a,b);
        }
        else if(ranks[a] == ranks[b])
        {
            BOOST_ASSERT(ranks[a] < std::numeric_limits<Rank>::max());
            ranks[a] += 1;
        }
        parents[b] = a;
        return a;
    }
};

#endif

struct LeafIndexTag {};

/** Union-find implementation
 */
template<
    typename Data, //associated to tree node
    typename Index = size_t //leaf index
    >
class PackedRootFinder
{
private :
    size_t count;
    //parents[i] < count -> child
    //parents[i] >= count -> root, rank = parents[i]-count
    Index * parents;
    Data  * datas;
    // Both parents and datas have the same lifetime, memory is allocated in one piece
    boost::scoped_array<char> memory;

    void destroy() noexcept
    {
        // Index and Data are assumed to be ints and pointers
        // TODO : destroy non-pod types correctly
    }
public :
    typedef Index Handle;

    template<typename D>
    void resetRange(Index b, Index e, D const & d)
    {
        for(Index i = b; i < e; ++i)
        {
            parents[i] = count;
            datas  [i] = d;
        }
    }

    void resetRange(Index b, Index e, LeafIndexTag const &)
    {
        for(Index i = b; i < e; ++i)
        {
            parents[i] = count;
            datas  [i] = i;
        }
    }

    template<typename D>
    void reset(D const & d)
    {
        resetRange(0, count, d);
    }

    PackedRootFinder() : count(0), parents(nullptr), datas(nullptr) {}

    template<typename D>
    PackedRootFinder(size_t c, D const & d)
        : count(0), parents(nullptr), datas(nullptr)
    {
        init(c, d);
    }

    ~PackedRootFinder() = default;

    void kill() noexcept
    {
        destroy();
        count = 0;
        parents = nullptr;
        datas   = nullptr;
        memory.reset();
    }

    template<typename D>
    void init(size_t c, D const & d)
    {
        if(count != c)
        {
            kill();
            size_t const alignment = 64;//usual cache line size
            // calculate required size
            size_t const parents_size
                = utils::alignSize(c*sizeof(Index), alignment);
            size_t const size
                = alignment-1
                + parents_size
                + utils::alignSize(c*sizeof(Data ), alignment);
            // allocate memory
            memory.reset(new char[size]);
            char * ptr = utils::alignPtr(memory.get(), alignment);
            parents = new(ptr) Index[c];//reinterpret_cast<Index *>(ptr);
            datas = new(ptr + parents_size) Data[c];
            //ptr += parents_size;
            //datas = reinterpret_cast<Data *>(ptr);
            count = c;
        }
        reset(d);
    }

    /** Access node data.
     * O(1)
     */
    Data & data(Handle h)
    {
        BOOST_ASSERT(h < count);
        return datas[h];
    }

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

    Handle merge_set(Handle a, Handle b, Data const & d)
    {
        Handle h = merge(a, b);
        data(h) = d;
        return h;
    }
};

}//namespace cct

#endif//CONNECTED_COMPONENT_TREE_ROOT_FINDER_H_INCLUDED
