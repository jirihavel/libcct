#ifndef LIBCCT_ARRAY_TREE_H_INCLUDED
#define LIBCCT_ARRAY_TREE_H_INCLUDED

#include "builder.h"

#include <utils/enum_flags.h>
#include <utils/fp.h>

#include <cstring>

#include <algorithm>
#include <memory>
#include <utility>

#include <boost/assert.hpp>
#include <boost/scoped_array.hpp>

namespace cct {
namespace array {

enum class TreeFlags : uint8_t
{
    NONE = 0x0,
    LEAF_LEVELS = 0x1,
    CHILD_LIST  = 0x2,
    CHILD_COUNT = 0x4,

    ALTITUDE_FLAGS = CHILD_LIST,
    ALPHA_FLAGS = CHILD_LIST | CHILD_COUNT,

    ALL = UINT8_MAX
};
ENUM_FLAGS_OPERATORS(TreeFlags)

template<typename IndexType, typename SizeType, typename LevelType>
struct Tree
{
    // Abstract types to represent different kinds of nodes.
    typedef IndexType LeafHandle;
    typedef IndexType CompHandle;
    typedef SizeType NodeHandle;

    // Types representing counts
    // - lc stands for max leaf count
    typedef IndexType LeafCount;//[0,lc]
    typedef IndexType CompCount;//[0,lc-1] for full binary tree
    typedef SizeType NodeCount;//[0,2*lc-1]
    typedef IndexType RootCount;//[0,lc]

    typedef IndexType LeafIndex;//[0,lc[
    typedef IndexType CompIndex;//[0,lc-1[
    typedef SizeType NodeIndex;//[0,2*lc-1[

    // leaf/comp index
    // values in [0,leaf_count[
    typedef IndexType index_type;

    // size type for node indices and counts
    // values in [0,node_capacity]
    typedef SizeType size_type;

    // node index
    // node_index = leaf_index
    // node_index = comp_index + leaf_count
    // values in [0,node_capacity[, node_capacity == leaf_count*2-1 for BPT

    using Level = LevelType;

    // leaf count is fixed, will not be modified outside of init/kill
    size_type leaf_count;
    // count of all nodes (leaf+comp)
    // values in [leaf_count,node_capacity[
    // comp_count = node_count-leaf_count
    size_type node_count;
    // amount of allocated nodes
    // leaf_count*2-1 for BPT
    size_type node_capacity;
    // invalid nodes can't be used again due to tree constraints.
    // tree must be compacted to remove invalid nodes
    size_type invalid_count;

    // parents :: size_type -> index_type
    // parents array contain comp indices, since leaves can't be parents
    // node p is parent of c :
    // - parents[c] = p-leaf_count
    // node r is root :
    // - parents[r] >= node_capacity-leaf_count
    // node n is invalid :
    // - parents[n] = 0 - leaves are always valid, and 0 is valid only for leaves
    // - only nodes below node count are marked invalid, values above are undefined
    // INVARIANTS :
    // - parents[i]+leaf_count > i - indices of parents are bigger than indices of children
    index_type * parents;//[node_capacity]

    // levels :: size_type -> level_type
    // - leaf levels may not be present -> split to two pointers
    // leaf_levels :: index_type -> level_type
    Level * leaf_levels;//[leaf_count], maybe nullptr
    // comp_levels :: index_type -> level_type
    // INVARIANTS :
    // - !leaf_levels || (comp_levels == leaf_levels+leaf_count)
    Level * comp_levels;//[node_capacity-leaf_count]

    // child_count :: index_type -> size_type
    // child_count contains PREFIX SUM of child counts for each component
    // - two additional elements for uniform access and simpler filling
    // for comp n : count = child_count[n+1]-child_count[n]
    //  -> child_count[0] = 0
    //  -> child_count[root] = node_count (root == node_count-leaf_count)
    //  -> child_count[root+1] = node_count (byproduct of counting)
    size_type * child_count;//[node_capacity-leaf_count+2] (two additional elements for easy counting)

    // children :: index_type -> size_type
    // for altitude-tree (simple BPT)
    //  comp i has children children[2*i] and children[2*i+1]
    // for alpha-tree
    //  comp i has children children[j], j in [child_count[i], child_count[i+1][
    size_type * children;//[node_capacity]

    CompHandle n2c(NodeHandle n) const noexcept
    {
        //BOOST_ASSERT(n > 0);
        BOOST_ASSERT(n >= leaf_count);
        BOOST_ASSERT(n < node_count);
        return n - leaf_count;
    }
    LeafHandle n2l(NodeHandle n) const noexcept
    {
        BOOST_ASSERT(n > 0);
        BOOST_ASSERT(n < leaf_count);
        return n - leaf_count;
    }

    TreeFlags flags() const noexcept
    {
        return 
            (leaf_levels ? TreeFlags::LEAF_LEVELS : TreeFlags::NONE) |
            (children    ? TreeFlags::CHILD_LIST  : TreeFlags::NONE) |
            (child_count ? TreeFlags::CHILD_COUNT : TreeFlags::NONE);
    }

    NodeCount nodeCount() const noexcept
    {
        return node_count;
    }

    LeafCount leafCount() const noexcept
    {
        return leaf_count;
    }

    CompCount compCount() const noexcept
    {
        return node_count - leaf_count;
    }

    size_type rootCount() const noexcept
    {
        return child_count[compCount()+1]-child_count[compCount()];
    }

    void construct() noexcept
    {
        leaf_count    = 0;
        node_count    = 0;
        node_capacity = 0;
        invalid_count = 0;
        parents     = nullptr;
        leaf_levels = nullptr;
        comp_levels = nullptr;
        child_count = nullptr;
        children    = nullptr;
    }

    void destruct() noexcept
    {
        delete [] parents;
        delete [] (leaf_levels ? leaf_levels : comp_levels);
        delete [] child_count;
        delete [] children;
    }

    friend void swap(Tree & a, Tree & b) noexcept
    {
        std::swap(a.leaf_count   , b.leaf_count   );
        std::swap(a.node_count   , b.node_count   );
        std::swap(a.node_capacity, b.node_capacity);
        std::swap(a.invalid_count, b.invalid_count);
        std::swap(a.parents    , b.parents    );
        std::swap(a.leaf_levels, b.leaf_levels);
        std::swap(a.comp_levels, b.comp_levels);
        std::swap(a.child_count, b.child_count);
        std::swap(a.children   , b.children   );
    }

    Tree() noexcept
    {
        construct();
    }

    Tree(Tree const & x)
        : Tree()
    {
        init(x.leaf_count, x.flags(), x.node_count);
        node_count    = x.node_count;
        invalid_count = x.invalid_count;
        std::copy_n(x.parents    , node_count, parents);
        if(leaf_levels)
            std::copy_n(x.leaf_levels, node_count, leaf_levels);
        else
            std::copy_n(x.comp_levels, node_count-leaf_count, comp_levels);
        if(child_count)
            std::copy_n(x.child_count, (node_count-leaf_count+2), child_count);
        if(children)
            std::copy_n(x.children   , node_count, children);
    }

    ~Tree() noexcept
    {
        destruct();
    }

    Tree & operator=(Tree && x) noexcept
    {
        swap(*this, x);
        return *this;
    }

    Tree & operator=(Tree const & x)
    {
        Tree tmp(x);
        return *this = std::move(tmp);
    }

    void kill()
    {
        destruct();
        construct();
    }

    void reset()
    {
        // the tree has no non-leaf nodes
        node_count = leaf_count;
        // including invalid nodes
        invalid_count = 0;
        // init leaves as singleton components (roots)
        // parents[i] == root_idx marks i as root node
        // init leaves as singleton components (roots)
        std::fill_n(parents, leaf_count, node_capacity-leaf_count);
        // do not touch rest of parents
        // init leaf levels to bottom
        if(leaf_levels)
            std::fill_n(leaf_levels, leaf_count, 0);
    }

    bool init(index_type leaves, TreeFlags f = TreeFlags::ALL, size_type capacity = 0)
    {
        if(capacity < leaves)
        {
            // on default, reserve space for full binary tree
            capacity = static_cast<size_type>(leaves)*2 - 1;
        }
        // f -> flags() for each bit
        if((leaf_count != leaves) || (node_capacity < capacity)
                || ((~f | flags()) != TreeFlags::ALL))
        {
            if(node_capacity < capacity)
            {
                kill();
                node_capacity = capacity;
                parents = new(std::nothrow) index_type[capacity];
                if(!parents) goto error;
                if(underlying_value(f & TreeFlags::LEAF_LEVELS))
                {
                    leaf_levels = new(std::nothrow) Level[capacity];
                    if(!leaf_levels) goto error;
                    comp_levels = leaf_levels + leaves;
                }
                else
                {
                    comp_levels = new(std::nothrow) Level[capacity-leaves];
                    if(!comp_levels) goto error;
                }
                if(underlying_value(f & TreeFlags::CHILD_COUNT))
                {
                    child_count = new(std::nothrow) size_type[capacity-leaves+2];
                    if(!child_count) goto error;
                }
                if(underlying_value(f & TreeFlags::CHILD_LIST))
                {
                    children = new(std::nothrow) size_type[capacity];
                    if(!children) goto error;
                }
            }
            leaf_count = leaves;
        }
        reset();
        return true;
    error:
        kill();
        return false;
    }

    /** \brief Merge binary nodes with same level to one n-ary node.
     *
     * !!! compress and build children after this.
     */
    void canonicalizeBinary()
    {
        // index to mark root node
        index_type const root = node_count-leaf_count;
        //for all nonroot & nonleaf nodes
        for(size_type n = node_count-1; n >= leaf_count; --n)
        {
            index_type const c = n-leaf_count;//component index of node
            index_type const p = parents[n];  //component index of parent
            if(p < root)
            {
                if(comp_levels[c] == comp_levels[p])
                {
                    // reconnect children
                    parents[children[2*c  ]] = p;
                    parents[children[2*c+1]] = p;
                    // invalidate node
                    parents[n] = 0;
                    ++invalid_count;
                }
            }
        }
    }

    void compress(
            index_type * lut = nullptr//[node_count-leaf_count]
        )
    {
        if(invalid_count > 0)
        {
            // allocate temporary lut if not 
            boost::scoped_array<index_type> tmp;
            if(!lut)
            {
                lut = new index_type[node_count-leaf_count];
                tmp.reset(lut);
            }

            size_type const old_count = node_count-leaf_count;
            index_type count = 0;
            for(size_type i = 0; i < old_count; ++i)
            {
                if(parents[i+leaf_count] != 0)
                {
                    size_type const n = count++;
                    parents[n+leaf_count] = parents[i+leaf_count];
                    comp_levels[n] = comp_levels[i];
                    lut[i] = n;
                }
            }
            // set correct counts
            node_count = leaf_count+count;
            invalid_count = 0;
            // set correct parents for all nodes
            // - keep roots as roots
            for(size_type i = 0; i < node_count; ++i)
            {
                if(parents[i] < old_count)
                    parents[i] = lut[parents[i]];
            }
        }
    }

    /**
     * heights[0..(nc-lc-1)] will be heights of tree component nodes
     * heights[nc-lc] contains height of pseudo-root (virtual parent of all roots)
     */
    template<typename Height = index_type>
    Height height(
            Height * heights = nullptr//[node_count-leaf_count+1]
        )
    {
        boost::scoped_array<Height> tmp;
        if(!heights)
        {
            heights = new Height[node_count-leaf_count+1];
            tmp.reset(heights);
        }
        std::fill_n(heights, node_count-leaf_count+1, 0);

        // index to mark root node 
        // - parents[i] >= root if i is root
        index_type const root = node_count-leaf_count;
        // go through leaves
        for(size_type i = 0; i < leaf_count; ++i)
        {
            heights[std::min(parents[i], root)] = 1;
        }

        for(size_type i = leaf_count; i < node_count; ++i)
        {
            index_type const p = std::min(parents[i], root);
            if(heights[p] <= heights[i-leaf_count])
            {
                BOOST_ASSERT(heights[i-leaf_count] < std::numeric_limits<Height>::max());
                heights[p] = heights[i-leaf_count]+1;
            }
        }
        
        // pseudo root has tree height + 1
        return heights[root]-1;
    }

    /**
     * For node i :
     * - i has (child_count[i-leaf_count+1]-child_count[i-leaf_count]) children
     * - these children are in children[child_count[i-leaf_count]..child_count[i-leaf_count][
     * - for i = (node_count-leaf_count), this gives the roots
     */
    void buildChildren()
    {
        BOOST_ASSERT(invalid_count == 0);
        // index to mark root node
        // - parents[i] >= root -> i is root
        index_type const root = node_count-leaf_count;
        // initialize counts
        std::fill_n(child_count, root+3, 0);
        // count children
        for(size_type i = 0; i < node_count; ++i)
        {
            ++child_count[std::min(parents[i], root)+2];
        }
        // prefix sum
        for(size_type i = 2; i < (root+2); ++i)
        {
            child_count[i+1] += child_count[i];
        }
        BOOST_ASSERT(child_count[0] == 0);
        BOOST_ASSERT(child_count[1] == 0);
        BOOST_ASSERT(child_count[root+2] == node_count);
        // set
        for(size_type i = 0; i < node_count; ++i)
        {
            children[child_count[std::min(parents[i], root)+1]++] = i;
        }
        BOOST_ASSERT(child_count[0] == 0);
        BOOST_ASSERT(child_count[root+1] == node_count);
        BOOST_ASSERT(child_count[root+2] == node_count);
    }

    /*
     * Update array of component indices based on transformation performed by
     * Tree::alphaFinish or Tree::compress.
     * lut is pointer to the array used by these tree transformations.
     */
    void updateCompIndices(index_type const * lut, index_type * indices, size_t count)
    {
        for(size_t i = 0; i < count; ++i)
            indices[i] = lut[indices[i]];
    }

    template<typename Count>
    void componentHistogram(
            Count * hist,//[node_count-leaf_count+1] +1 for pseudo-root
            index_type const * comps, size_t count
        )
    {
        // index of pseudo-root
        auto const root = node_count-leaf_count;
        std::fill_n(hist, root+1, 0);
        for(size_t i = 0; i < count; ++i)
        {
            ++hist[std::min(comps[i], root)];
        }
    }
};

}//namespace array
#include "array_tree.inl"
}//namespace cct

#endif//LIBCCT_ARRAY_TREE_H_INCLUDED
