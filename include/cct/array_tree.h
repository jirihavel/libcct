#include <cstring>

#include <utility>

#include <boost/assert.hpp>
/*
 * lc - leaf count
 * cc - component count
 * nc = (lc + cc) - node count
 * cm - maximal component count
 * * lc-1 for bpt
 * nm - maximal node count 
 * * lc+cm = lc*2-1 for bpt
 *
 * parents[nm]
 * * index of parent node n-lc
 * * !!! parents[i]+lc > i
 * * root : cm = lc-1
 * * unused : 0 - valid only for leaves, which are always used
 * * values in [0,cm] = [0,lc[
 * * bits : ceil(log2(lc))
 *
 * levels[cm] for alpha-tree, levels[nm] for 2nd order tree
 * * node levels, unknown type
 * * maybe pack into parents[]
 * * !!! a < b -> levels[a] <= levels[b]
 *
 * merges[cm]
 * * temporary array for node merging
 * * node i was merged to merges[i-lc]+lc
 * * values in [0,cm[ = [0,lc-1[
 * * merges[i] == i - node is not merged
 * * !!! merges[i] >= i
 *
 * child_counts[cm]
 * * counts of child nodes
 * * values in ]0,lc] for used nodes (map to [0,lc[)
 * * bits : ceil(log2(lc))
 */
template<typename IndexType, typename SizeType, typename LevelType>
struct array_tree
{
    // leaf/comp index
    // values in [0,leaf_count[
    typedef IndexType index_type;
    // node index :
    // node_index = leaf_index
    // node_index = comp_index + leaf_count
    // size type will store values larger than leaf indices
    // values in [0,node_capacity-1], node_capacity ... leaf_count*2-1 for BPT
    typedef SizeType size_type;

    typedef LevelType level_type;

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

    // node_index in [0, node_capacity[
    //  leaf_index = node_index, leaf_index in [0, leaf_count[
    //  comp_index = node_index-leaf_count, comp_index in [0, node_capacity-leaf_count[

    // parents :: size_type -> index_type
    // parents array contain comp_indices, since leaves can't be parents
    // p is parent of c : parents[c] = p-leaf_count
    // r is root : parents[r] = node_capacity-leaf_count
    // n is invalid : parents[n] = 0 - leaves are always valid, and 0 is valid only for leaves
    //  - only nodes below node count are marked invalid, nodes above have undefined values
    // INVARIANT : parents[i]+leaf_count > i - indices of parents are bigger than indices of children
    index_type * parents;//[node_capacity]

    // leaf_levels :: index_type -> level_type
    level_type * leaf_levels;//maybe nullptr
    // comp_levels :: index_type -> level_type
    // leaf_levels not null -> comp_levels = leaf_levels + leaf_count
    level_type * comp_levels;//[node_capacity-leaf_count]

    // child_count :: index_type -> size_type
    // child_count contains PREFIX SUM of child counts for each component
    // for node n : count = child_count[n+1]-child_count[n]
    //  -> child_count[0] = 0
    //  -> child_count[root] = node_count
    //  -> child_count[root+1] = root_count (byproduct of counting)
    size_type * child_count;//[node_capacity-leaf_count+2] (two additional elements for easy counting)

    // children :: index_type -> size_type
    // for BPT
    //  comp i has children children[2*i] and children[2*i+1]
    // for alpha-tree
    //  comp i has children children[j], j in [child_count[i], child_count[i+1][
    size_type * children;//[(node_capacity-leaf_count)*2]
    
    void reset()
    {
        BOOST_ASSERT(leaf_count >= 0);
        BOOST_ASSERT(leaf_count < node_capacity);
        BOOST_ASSERT(parents);
        BOOST_ASSERT(comp_levels);
        // index to mark root node
        index_type const root = node_capacity-leaf_count;
        // the tree has no non-leaf nodes
        node_count = leaf_count;
        // including invalid nodes
        invalid_count = 0;
        // init leaves as singleton components (roots)
        std::fill_n(parents, leaf_count, root);
        // init leaf levels to bottom
        if(leaf_levels)
        {
            std::fill_n(leaf_levels, leaf_count, 0);
        }
    }

    size_type nodeCount() const noexcept
    {
        return node_count;
    }
    size_type leafCount() const noexcept
    {
        return leaf_count;
    }
    size_type componentCount() const noexcept
    {
        return node_count-leaf_count;
    }

    size_type bpt_merge(size_type a, size_type b)
    {
        BOOST_ASSERT(a >= 0);
        BOOST_ASSERT(a < node_count);
        BOOST_ASSERT(b >= 0);
        BOOST_ASSERT(b < node_count);
        BOOST_ASSERT(a != b);
        // index to mark root node
        index_type const root = node_capacity-leaf_count;
        // check if merged nodes are roots
        BOOST_ASSERT(parents[a] == root);
        BOOST_ASSERT(parents[b] == root);
        // allocate new node
        BOOST_ASSERT(node_count < node_capacity);
        size_type const n = node_count++;//node index
        index_type const c = n-leaf_count;//component index
        parents[a] = c; // link to c
        parents[b] = c;
        parents[n] = root; // mark c as root
        // mark a, b as children of c
        children[2*c  ] = a;
        children[2*c+1] = b;
        // return node_index of the new node
        return n;
    }

    size_type bpt_merge(size_type a, size_type b, level_type l)
    {
        size_type n = bpt_merge(a, b);
        // ensure increasing weights
        BOOST_ASSERT((a < leaf_count) || (comp_levels[a-leaf_count] <= l));
        BOOST_ASSERT((b < leaf_count) || (comp_levels[b-leaf_count] <= l));
        // set level of new node
        comp_levels[n-leaf_count] = l;
        // return node_index of the new node
        return n;
    }

    void bpt_postprocess()
    {
        // index to mark root node
        index_type const root = node_capacity-leaf_count;
        //for all nonroot & nonleaf nodes
        for(size_type n = node_count; n >= leaf_count; --n)
        {
            index_type const c = n-leaf_count;//component index of node
            index_type const p = parents[n];// component index of parent
            if(p != root)
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

    size_type alpha_merge(
            size_type a, size_type b,
            size_type layer_begin, level_type l,
            index_type * merges//[node_capacity-leaf_count+1]
            )
    {
        BOOST_ASSERT(a >= 0);
        BOOST_ASSERT(a < node_count);
        BOOST_ASSERT(b >= 0);
        BOOST_ASSERT(b < node_count);
        BOOST_ASSERT(a != b);
        // index to mark root node
        index_type const root = node_capacity-leaf_count;
        // check if merged nodes are roots
        BOOST_ASSERT(parents[a] == root);
        BOOST_ASSERT(parents[b] == root);
        // ensure increasing weights
        BOOST_ASSERT((a < leaf_count) || (comp_levels[a-leaf_count] <= l));
        BOOST_ASSERT((b < leaf_count) || (comp_levels[b-leaf_count] <= l));
        // simplify merge
        if(a < b) // a < b -> level(a) < level(b)
            std::swap(a, b);
        // if a is leaf, b must be too
        BOOST_ASSERT((a >= leaf_count) || (b < leaf_count));
        // a or b is leaf, or level(a) >= level(b)
        BOOST_ASSERT((a < leaf_count) || (b < leaf_count) || (comp_levels[a-leaf_count] >= comp_levels[b-leaf_count]));
        // lift a
        if(a < layer_begin)
        {
            // ensure by level check
            BOOST_ASSERT((a < leaf_count) || (comp_levels[a-leaf_count] < l));
            // allocate new node
            BOOST_ASSERT(node_count < node_capacity);
            size_type const n = node_count++;
            // link a to n
            parents[a] = n-leaf_count;
            parents[n] = root;
            // set level of n
            comp_levels[n-leaf_count] = l;
            // insert identity loop to merges
            merges[n-leaf_count] = n-leaf_count;
            // forget about old a
            a = n;
        }
        // a is now nonleaf node on this level
        BOOST_ASSERT(a >= leaf_count);
        BOOST_ASSERT(a >= layer_begin);
        BOOST_ASSERT(comp_levels[a-leaf_count] == l);
        // connect or merge b
        parents[b] = a-leaf_count; // set a as parent of b
        if(b >= layer_begin)
        {
            // mark b to be merged into a
            ++invalid_count;
            merges[b-leaf_count] = a-leaf_count;
        }
        // return (possibly new) common root
        return a;
    }

    void finish_alpha_merges(
            index_type * merges//[node_capacity-leaf_count+1]
            )
    {
        if(invalid_count > 0)
        {
            merges[leaf_count-1] = leaf_count-1;
            // traverse tree from root to leaves
            for(size_type i = node_count-leaf_count; i-- > 0;)
            {
                BOOST_ASSERT(merges[i] >= i);
                if(i == merges[i])
                {
                    parents[i+leaf_count] = merges[parents[i+leaf_count]];
                }
                else
                {
                    merges[i] = merges[merges[i]];
                    parents[i+leaf_count] = 0;
                }
            }
            // set parents for leaves
            for(size_type i = 0; i < leaf_count; ++i)
            {
                parents[i] = merges[parents[i]];
            }
        }
    }

    void compress(
            index_type * lut//[node_count-leaf_count]
            )
    {
        if(invalid_count > 0)
        {
            index_type count = 0;
            for(size_type i = 0; i < node_count-leaf_count; ++i)
            {
                if(parents[i+leaf_count] != 0)
                {
                    size_type const n = count++;
                    parents[n+leaf_count] = parents[i+leaf_count];
                    comp_levels[n+leaf_count] = comp_levels[n+leaf_count];
                    lut[i] = n;
                }
            }
            node_count = leaf_count+count;
            invalid_count = 0;
            // set correct parents for all nodes
            for(size_type i = 0; i < node_count; ++i)
            {
                parents[i] = lut[parents[i]];
            }
        }
    }

    // count
    // {0, 0,
    //  count[2+0] = count 0
    //  ...
    //  count[2+comp_count-1] = count (comp_count-1)
    //  count[2+comp_count] = undefined
    //  ...
    //  count[2+root] = root_count
    // } - node_capacity-leaf_count+3
    // partial sum from 3 to comp_count-1, handle root end by hand
    // {0, 0,
    //  count[2+0] = count 0
    //  count[2+1] = count 0+1,
    //  ...
    //  count[2+comp_count-1] = node_count-root_count,
    //  count[2+comp_count] = invalid 
    //  ...
    //  count[2+root] = root_count
    // }
    // handle root stuff
    //  TODO
    // assign children
    //  TODO
    void build_children()
    {
        BOOST_ASSERT(invalid_count == 0);
        // index to mark root node
        index_type const root = node_capacity-leaf_count;
        // initialize counts
        std::fill_n(child_count, node_count-leaf_count+2, 0);
        child_count[root+2] = 0;
        // count children
        child_count[0] = child_count[1] = 0;
        for(size_type i = 0; i < node_count; ++i)
        {
            ++child_count[parents[i]+2];
        }
        // prefix sum
        for(size_type i = 2; i < (node_count-leaf_count+2); ++i)
        {
            child_count[i+1] += child_count[i];
        }
        // set :
        //  child_count[root+1] = nonroot node count
        //  child_count[root+2] = node_count
        child_count[root+1] = child_count[node_count-leaf_count+1];
        child_count[root+2] += child_count[root+1];
        // assign children
        // also moves child_count[i+1] to child_count[i]
        for(size_type i = 0; i < node_count; ++i)
        {
            children[child_count[parents[i]+1]++] = i;
            // this is the reason, why child_count[root+1] was set to nonroot_count
        }
    }
};
