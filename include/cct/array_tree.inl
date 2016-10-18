template<typename I, typename S, typename L>
class TreeBuilder<array::Tree<I,S,L>>
    : public BuilderBase<array::Tree<I,S,L>>
{
public :
    using Tree = array::Tree<I,S,L>;

    using RootHandle = typename Tree::NodeHandle;
    using RootFinderInitializer = utils::fp::identity;

    typename Tree::CompHandle r2c(Tree const & tree, RootHandle n)
    {
        return tree.n2c(n);
    }
protected :
    TreeBuilder(Tree & tree, size_t vertex_count, array::TreeFlags flags)
    {
        tree.init(vertex_count, tree.flags() | flags);
    }
};

template<typename I, typename S, typename L>
class AlphaTreeBuilder<array::Tree<I,S,L>>
    : public TreeBuilder<array::Tree<I,S,L>>
{
    using Base = TreeBuilder<array::Tree<I,S,L>>;
    using Tree = typename Base::Tree;
public :
    template<typename Weight>
    AlphaTreeBuilder(Tree & tree, size_t vertex_count, Weight start_weight)
        : Base(tree, vertex_count, array::TreeFlags::ALPHA_FLAGS)
        , m_merges(new S[tree.node_capacity-tree.leaf_count+1])
        , m_layer_begin(tree.node_count)
        , m_layer_level(start_weight)
    {
    }

    template<typename Weight>
    S lift(Tree & tree, I v, S n, Weight weight)
    {
        if(weight > m_layer_level)
        {
            m_layer_begin = tree.node_count;
            m_layer_level = weight;
        }
        // index to mark root node
        I const root = tree.node_capacity-tree.leaf_count;
        if(v < n) // leaf is not singleton
        {
            if(n < m_layer_begin)
            {
                // allocate new node
                BOOST_ASSERT(tree.node_count < tree.node_capacity);
                auto const r = tree.node_count++;
                tree.parents[n] = r-tree.leaf_count;
                tree.parents[r] = root;
                tree.comp_levels[r-tree.leaf_count] = weight;
                n = r;
            }
            tree.parents[v] = n-tree.leaf_count;
        }
        tree.leaf_levels[v] = weight;
        return n;
    }

    template<typename Edge>
    S merge(Tree & tree, S a, S b, Edge const & edge)
    {
        if(edge.weight > m_layer_level)
        {
            m_layer_begin = tree.node_count;
            m_layer_level = edge.weight;
        }
        BOOST_ASSERT(a >= 0);
        BOOST_ASSERT(a < tree.node_count);
        BOOST_ASSERT(b >= 0);
        BOOST_ASSERT(b < tree.node_count);
        BOOST_ASSERT(a != b);
        // index to mark root node
        I const root = tree.node_capacity-tree.leaf_count;
        // check if merged nodes are roots
        BOOST_ASSERT(tree.parents[a] == root);
        BOOST_ASSERT(tree.parents[b] == root);
        // ensure increasing weights
        BOOST_ASSERT((a < tree.leaf_count) || (tree.comp_levels[a-tree.leaf_count] <= edge.weight));
        BOOST_ASSERT((b < tree.leaf_count) || (tree.comp_levels[b-tree.leaf_count] <= edge.weight));
        // simplify merge
        if(a < b) // a < b -> level(a) < level(b)
            std::swap(a, b);
        // if a is leaf, b must be too
        BOOST_ASSERT((a >= tree.leaf_count) || (b < tree.leaf_count));
        // a or b is leaf, or level(a) >= level(b)
        BOOST_ASSERT((a < tree.leaf_count) || (b < tree.leaf_count) || (tree.comp_levels[a-tree.leaf_count] >= tree.comp_levels[b-tree.leaf_count]));
        // lift a
        if(a < m_layer_begin)
        {
            // ensure by level check
            BOOST_ASSERT((a < tree.leaf_count) || (tree.comp_levels[a-tree.leaf_count] < edge.weight));
            // allocate new node
            BOOST_ASSERT(tree.node_count < tree.node_capacity);
            S const n = tree.node_count++;
            // link a to n
            tree.parents[a] = n-tree.leaf_count;
            tree.parents[n] = root;
            // set level of n
            tree.comp_levels[n-tree.leaf_count] = edge.weight;
            // insert identity loop to merges
            m_merges[n-tree.leaf_count] = n-tree.leaf_count;
            // forget about old a
            a = n;
        }
        // a is now nonleaf node on this level
        BOOST_ASSERT(a >= tree.leaf_count);
        BOOST_ASSERT(a >= m_layer_begin);
        BOOST_ASSERT(tree.comp_levels[a-tree.leaf_count] == edge.weight);
        // connect or merge b
        tree.parents[b] = a-tree.leaf_count; // set a as parent of b
        if(b >= m_layer_begin)
        {
            // mark b to be merged into a
            ++tree.invalid_count;
            m_merges[b-tree.leaf_count] = a-tree.leaf_count;
        }
        // return (possibly new) common root
        return a;
    }

    void finish(Tree & tree, S * edge_comps, size_t edge_count)
    {
        // finish alpha merges
        if(tree.invalid_count > 0)
        {
            m_merges[tree.leaf_count-1] = tree.leaf_count-1;
            // traverse tree from root to leaves
            for(S i = tree.node_count-tree.leaf_count; i-- > 0;)
            {
                BOOST_ASSERT(m_merges[i] >= i);
                if(i == m_merges[i])
                {
                    auto & p = tree.parents[i+tree.leaf_count];
                    p = m_merges[p];
                }
                else
                {
                    m_merges[i] = m_merges[m_merges[i]];
                    tree.parents[i+tree.leaf_count] = 0;
                }
            }
            // set parents for leaves
            for(S i = 0; i < tree.leaf_count; ++i)
            {
                tree.parents[i] = m_merges[tree.parents[i]];
            }
        }
        // relabel edges according to updated tree
        if(edge_comps)
            tree.updateCompIndices(m_merges.get(), edge_comps, edge_count);
        // remove gaps in the tree
        tree.compress(m_merges.get());
        if(edge_comps)
            tree.updateCompIndices(m_merges.get(), edge_comps, edge_count);
        // generate list of children
        tree.buildChildren();
    }
private :
    std::unique_ptr<S[]> m_merges;
    S m_layer_begin;
    L m_layer_level;
};

template<typename I, typename S, typename L>
class AltitudeTreeBuilder<array::Tree<I,S,L>>
    : public TreeBuilder<array::Tree<I,S,L>>
{
    using Base = TreeBuilder<array::Tree<I,S,L>>;
    using Tree = typename Base::Tree;
public :
    template<typename Weight>
    AltitudeTreeBuilder(Tree & tree, size_t vertex_count, Weight /*start_weight*/)
        : Base(tree, vertex_count, array::TreeFlags::ALTITUDE_FLAGS)
    {
    }

    template<typename Edge>
    S merge(Tree & tree, S a, S b, Edge const & edge)
    {
        BOOST_ASSERT(a >= 0);
        BOOST_ASSERT(a < tree.node_count);
        BOOST_ASSERT(b >= 0);
        BOOST_ASSERT(b < tree.node_count);
        BOOST_ASSERT(a != b);
        // index to mark root node
        I const root = tree.node_capacity-tree.leaf_count;
        // check if merged nodes are roots
        BOOST_ASSERT(tree.parents[a] >= root);
        BOOST_ASSERT(tree.parents[b] >= root);
        // allocate new node
        BOOST_ASSERT(tree.node_count < tree.node_capacity);
        S const n = tree.node_count++;//node index
        I const c = n-tree.leaf_count;//component index
        tree.parents[a] = c; // link to c
        tree.parents[b] = c;
        tree.parents[n] = root; // mark c as root
        // mark a, b as children of c
        tree.children[2*c  ] = a;
        tree.children[2*c+1] = b;
        // set level of new node
        tree.comp_levels[c] = edge.weight;
        // return node_index of the new node
        return n;
    }

    void finish(Tree & tree, S * /*edge_comps*/, size_t /*edge_count*/)
    {
        if(tree.child_count)
        {
            S const root = tree.node_count-tree.leaf_count;
            for(S i = 0; i < root; ++i)
            {
                tree.child_count[i] = 2*i;
            }
            tree.child_count[root] = tree.child_count[root+1] = tree.node_count;
        }
    }
};
