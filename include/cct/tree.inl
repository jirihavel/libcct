// -- Destruction --

template<typename C, typename L>
void Tree<C,L>::resetRange(size_type b, size_type e) noexcept
{
    for(size_type i = b; i != e; ++i)
    {
        Component * node = n2c(leaf(i)->unlink());
        // traverse tree up and remove empty nodes
        while(node && node->empty())
        {
            Component * parent = n2c(node->unlink());
            if(!parent)
            {
                remRoot(node);
            }
            BOOST_ASSERT(component_count > 0);
            delete node;
            --component_count;
            node = parent;
        }
    }
}

template<typename C, typename L>
void Tree<C,L>::reset() noexcept
{
    resetRange(0, leaf_count);
    BOOST_ASSERT(roots.empty());
    BOOST_ASSERT(component_count == 0);
}

template<typename C, typename L>
void Tree<C,L>::kill() noexcept
{
    reset();
    // free leaves
    leaves.reset();
    leaf_count = 0;
}

// -- Initialization --

template<typename C, typename L>
void Tree<C,L>::init(size_type lc)
{
    if(leaf_count == lc)
    {
        // avoid reallocation if new size is the same as before
        reset();
    }
    else
    {
        kill();
        leaves.reset(new Leaf[lc]);
        leaf_count = lc;
    }
}

// --

template<typename C, typename L>
typename Tree<C,L>::size_type
Tree<C,L>::countDegenerateComponents() const
{
    size_type count = 0;
    forEach(
        [&](Component const & c)
        {
            if(c.isDegenerate())
                ++count;
        },
        utils::fp::ignore(), utils::fp::ignore()
    );
    return count;
}

template<typename C, typename L>
typename Tree<C,L>::size_type
Tree<C,L>::calculateHeight() const
{
    size_type height = 0;
    for(size_type i = 0; i < leaf_count; ++i)
    {
        height = std::max(height, leaf(i)->calculateHeight());
    }
    return height;
}

// -- Pretty printing --

template<typename C, typename L>
template<typename Printer>
void Tree<C,L>::prettyPrint(std::ostream & out, Printer const & p, Node const & node, std::string indent, bool last)
{
    out << indent;
    if(last)
    {
        out << "\\-";
        indent += "  ";
    }
    else
    {
        out << "|-";
        indent += "| ";
    }

    if(isNodeLeaf(node))
    {
        //prettyPrint(out, static_cast<Leaf const &>(node));
        p.print(out, *this, static_cast<Leaf const &>(node));
        out << '\n';
    }
    else
    {
        Component const & comp = static_cast<Component const &>(node);
        //out << '[' << comp << "]\n";
        p.print(out, *this, comp);
        out << '\n';
        if(!comp.empty())
        {
            typename Component::const_iterator i = comp.begin();
            do
            {
                typename Component::const_iterator j = i++;
                last = (i == comp.end());
                prettyPrint(out, p, *j, indent, last);
            } while(!last);
        }
    }
}

template<typename C, typename L>
template<typename Printer>
void Tree<C,L>::prettyPrint(std::ostream & out, Printer const & p)
{
    out << "roots=" << roots.size() << ", comps=" << component_count << ", leaves=" << leaf_count << std::endl;
    typename Roots::const_iterator i = roots.begin();
    if(i != roots.end())
    {
        bool last = false;
        do
        {
            typename Roots::const_iterator j = i++;
            last = (i == roots.end());
            prettyPrint(out, p, *j, "", last);
        } while(!last);
    }
}

template<typename C, typename L>
class TreeBuilder<Tree<C,L>>
    : public BuilderBase<Tree<C,L>>
{
    using Tree = Tree<C,L>;
    using Component = typename Tree::Component;
protected :
    TreeBuilder(Tree & tree, size_t vertex_count)
    {
        tree.init(vertex_count);
    }

    template<typename VE> // Vertex or Edge
    Component * newRoot(Tree & tree, VE const & ve)
    {
        Component * n = new Component(ve);
        ++tree.component_count;
        tree.addRoot(n);
        return n;        
    }
public :
    using RootHandle = Component *;
    using RootFinderInitializer = utils::fp::constant_null;

    Component * r2c(Tree const &, Component * n) const noexcept
    {
        return n;
    }

    template<typename VertexId, typename Weight>
    Component * lift(Tree & tree, Component * n, Vertex<VertexId, Weight> const & vertex)
    {
        if(!n || (*n < vertex))
        {
            auto new_node = this->newRoot(tree, vertex);
            ++tree.component_count;
            if(n)
            {
                tree.remRoot(n);
                new_node->relink(n);
            }
            else
            {
                new_node->link(tree.leaf(vertex.id));
            }
            n = new_node;
        }
        return n;
    }
};

template<typename C, typename L>
class AlphaTreeBuilder<Tree<C,L>>
    : public TreeBuilder<Tree<C,L>>
{
    using Tree = Tree<C,L>;
    using Base = TreeBuilder<Tree>;
    using Component = typename Tree::Component;

    using Components = boost::intrusive::list<
            Component,
            boost::intrusive::constant_time_size<false>
        >;

    template<typename VertexId, typename Weight>
    Component * liftForEdge(Tree & tree, VertexId v, Component * n, Edge<VertexId, Weight> const & edge)
    {
        if(!n || (*n < edge))
        {
            auto new_node = this->newRoot(tree, edge);
            if(n)
            {
                tree.remRoot(n);
                new_node->link(n);
            }
            else
            {
                new_node->link(tree.leaf(v));
            }
            n = new_node;
        }
        return n;
    }
public :    
    template<typename Weight>
    AlphaTreeBuilder(Tree & tree, size_t vertex_count, Weight /*start_weight*/)
        : Base(tree, vertex_count)
    {}

    template<typename Vertex, typename Weight>
    Component * merge(Tree & tree, Component * n0, Component * n1, Edge<Vertex, Weight> const & edge)
    {
        Vertex v[2] = { edge.vertices[0], edge.vertices[1] };
        if(!n0 || (n1 && (*n0 < *n1)))
        {
            std::swap(v[0], v[1]);
            std::swap(n0, n1);
        }
        n0 = liftForEdge(tree, v[0], n0, edge);
        BOOST_ASSERT(n0);
        if(!n1)
        {
            n0->link(tree.leaf(v[1]));
        }
        else if(*n1 < edge)
        {
            tree.remRoot(n1);
            n0->link(n1);
        }
        else
        {
            // When size is constant time, ensure smaller node is removed
            if(n0->smallerThanInO1(*n1))
            {
                std::swap(n0, n1);
            }
            tree.remRoot(n1);
            // Mark node for removal
            n1->setParent(n0);
            m_redundant.push_front(*n1);
        }
        return n0;
    }

    void finish(Tree & tree, Component ** edge_comps, size_t edge_count)
    {
        if(!m_redundant.empty())
        {
            typename Tree::size_type n = 0;
            // reconnect children of redundant components
            for(auto i = m_redundant.begin(); i != m_redundant.end(); ++i)
            {
                ++n;
                // get node for merging
                Component * node = tree.n2c(i->parent());
                BOOST_ASSERT(node);
                if(node->empty())// parent of <i> is also in <m_redundant>
                {
                    // we need one step up
                    node = tree.n2c(node->parent());
                    BOOST_ASSERT(node);
                    BOOST_ASSERT(!node->empty());
                    i->setParent(node);
                }
                // reconnect components
                node->absorb(*i);
                // all redundant nodes are empty and are linked to correct parents
                BOOST_ASSERT(i->empty());
                BOOST_ASSERT(!tree.n2c(i->parent())->empty());
            }
            // correct edge components
            if(edge_comps)
            {
                for(size_t i = 0; i < edge_count; ++i)
                {
                    BOOST_ASSERT(edge_comps[i]);
                    if(edge_comps[i]->empty())
                    {
                        edge_comps[i] = tree.n2c(edge_comps[i]->parent());
                        BOOST_ASSERT(edge_comps[i]);
                        BOOST_ASSERT(!edge_comps[i]->empty());
                    }
                }
            }
            // delete redundant nodes
            m_redundant.clear_and_dispose(std::default_delete<Component>());
            BOOST_ASSERT(tree.component_count > n);
            tree.component_count -= n;
        }
    }

    template<typename VertexId, typename Weight>
    bool compareLayer(Tree const &, Component const * n, Edge<VertexId, Weight> const & edge)
    {
        BOOST_ASSERT(n);
        return *n < edge;
    }

    void finishLayer(Tree & tree, Component ** edge_comps, size_t /*edge_count*/)
    {
        if(!edge_comps)//TODO
            finish(tree, nullptr, 0);
    }
private :
    Components m_redundant;
};

template<typename C, typename L>
class AltitudeTreeBuilder<Tree<C,L>>
    : public TreeBuilder<Tree<C,L>>
{
    using Tree = Tree<C,L>;
    using Base = TreeBuilder<Tree>;
    using Component = typename Tree::Component;

    template<typename Vertex>
    void link(Tree & tree, Component & p, Component * c, Vertex v)
    {
        if(c)
        {
            tree.remRoot(c);
            p.link(c);
        }
        else
        {
            p.link(tree.leaf(v));
        }
    }
public :
    template<typename Weight>
    AltitudeTreeBuilder(Tree & tree, size_t vertex_count, Weight /*start_weight*/)
        : Base(tree, vertex_count)
    {}

    template<typename Vertex, typename Weight>
    Component * merge(Tree & tree, Component * n0, Component * n1, Edge<Vertex, Weight> const & edge)
    {
        auto n = this->newRoot(tree, edge);
        link(tree, *n, n0, edge.vertices[0]);
        link(tree, *n, n1, edge.vertices[1]);
        return n;
    }
};
