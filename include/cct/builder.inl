template<typename Tree>
void Builder<Tree>::finish(ThreadBuilder<Builder<Tree>> && tb)
{
    // remove all redundant nodes
    tb.remove();
    // move component count
    m_tree->component_count += tb.m_component_count;
    tb.m_component_count = 0;
    // move roots
    while(!tb.m_roots.empty())
    {
        Component * node = &tb.m_roots.front();
        tb.m_roots.pop_front();
        m_tree->addRoot(node);
    }
}

//-----------------------------------------------------------------------------
// ThreadBuilder
//-----------------------------------------------------------------------------

template<typename BuilderType>
ThreadBuilder<BuilderType>::~ThreadBuilder() noexcept
{
    // TODO : roots and subtrees will leak
    // TODO : redundant nodes and subtrees will leak
    // clear pool
    m_pool.clear_and_dispose(std::default_delete<Component>());
}

template<typename BuilderType>
template<typename Edge>
typename ThreadBuilder<BuilderType>::Component *
ThreadBuilder<BuilderType>::alloc(Edge const & edge)
{
    Component * node = nullptr;
    if(m_pool.empty())
    {
        node = new Component(edge);
    }
    else
    {
        node = &m_pool.front();
        m_pool.pop_front();
        // create again
        node->~Component();
        new(node) Component(edge);
    }
    ++m_component_count;
    return node;
}

template<typename B>
template<typename Edge>
typename ThreadBuilder<B>::Component *
ThreadBuilder<B>::lift(size_type i, Component * n, Edge const & edge)
{
    Component * node = nullptr;
    if(!n)
    {
        node = alloc(edge);
        node->link(tree().leaf(i));
        m_roots.push_back(*node);
    }
    else if(*n < edge)
    {
        m_roots.erase(m_roots.iterator_to(*n));
        node = alloc(edge);
        node->link(n);
        m_roots.push_back(*node);
    }
    else
    {
        node = n;
    }
    return node;
}

template<typename B>
template<typename Edge>
typename ThreadBuilder<B>::Component *
ThreadBuilder<B>::lift(Component * n, Edge const & edge)
{
    if(*n < edge)
    {
        Component * node = alloc(edge);
        Component * parent = static_cast<Component*>(node->relink(n));
        parent->link(node);
        return node;
    }
    else
    {
        return n;
    }
}

template<typename B>
void ThreadBuilder<B>::reconnect(Component * from, Component * to)
{
    auto i = from->children.begin();
    // reconnect components
    while((i != from->children.end()) && !tree().isNodeLeaf(*i))
    {
        i->setParent(to);
        ++i;
    }
    to->children.splice(to->children.begin(), from->children, from->children.begin(), i);
    // reconnect leaves
    while(i != from->children.end())
    {
        i->setParent(to);
        ++i;
    }
    to->children.splice(to->children.end(), from->children);
}

template<typename B>
void ThreadBuilder<B>::absorb(ThreadBuilder<B> && b)
{
    BOOST_ASSERT(&m_builder == &(b.m_builder));
    // finish removals
    remove();
    b.remove();
    // transfer ownership of all nodes
    m_component_count += b.m_component_count;
    b.m_component_count = 0;
    // transfer roots
    m_roots.splice(m_roots.end(), b.m_roots);
    // transfer node pool
    m_pool.splice(m_pool.end(), b.m_pool);
}

//-----------------------------------------------------------------------------
// ThreadBuilder - singlethreaded construction
//-----------------------------------------------------------------------------

template<typename B>
template<typename Edge>
typename ThreadBuilder<B>::Component *
ThreadBuilder<B>::addEdge(
    size_type a, size_type b,
    Edge const & edge
)
{
    BOOST_ASSERT(tree().isLeafId(a));
    BOOST_ASSERT(tree().isLeafId(b));

    typedef typename Builder::Handle Handle;
    // 
    //Component * node = nullptr;
    // Lookup roots
    Handle ha = m_builder.m_rootFinder.find_update(a);
    Handle hb = m_builder.m_rootFinder.find_update(b);
    if(ha != hb)
    {
        // edge is in MST
        // get corresponding nodes
        Component * na = m_builder.m_rootFinder.data(ha);
        Component * nb = m_builder.m_rootFinder.data(hb);
        // simplify merge
        // !na && !nb -> swap (can be)
        // !na && nb -> swap (necessary)
        // na && !nb -> no swap
        // na && nb -> *na < *nb -> swap (necessary)
        if(!na || (nb && *na < *nb))
        {
            // Handles are not swapped, their order is not important            
            std::swap(a, b);
            std::swap(na, nb);
        }
        // Lift subtree A
        na = lift(a, na, edge);
        // Lift subtree B
        if(!nb)
        {
            na->link(tree().leaf(b));
        }
        else if(*nb < edge)
        {
            m_roots.erase(m_roots.iterator_to(*nb));
            na->link(nb);
        }
        else
        {
            // When size is constant time, ensure smaller node is removed
            if(Component::constant_time_children_size &&
                (na->children.size() < nb->children.size()))
            {
                std::swap(na, nb);
            }
            // Mark node for removal
            m_roots.erase(m_roots.iterator_to(*nb));
            nb->setParent(na);
            m_redundant.push_front(*nb);
        }
        // Merge trees in root finder
        m_builder.m_rootFinder.merge_set(ha, hb, na);
        return na;
    }
    else
    {
        return nullptr;
    }
}

template<typename B>
void ThreadBuilder<B>::remove()
{
    if(!m_redundant.empty())
    {
        size_type n = 0;
        // reconnect
        for(typename Components::iterator i = m_redundant.begin(); i != m_redundant.end(); ++i)
        {
            ++n;
            // get node for merging
            Component * root = static_cast<Component*>(i->parent());
            if(root->empty())
            {
                // parent of <i> is also in <m_redundant>
                root = static_cast<Component*>(root->parent());
                i->setParent(root);
            }
            // reconnect components
            reconnect(&(*i), root);
        }
        // move redundant nodes to allocation pool
        m_pool.splice(m_pool.begin(), m_redundant);
        // subtract deleted nodes from node counter
        m_component_count -= n;
    }
}

//-----------------------------------------------------------------------------
// ThreadBuilder - multithreaded construction
//-----------------------------------------------------------------------------

template<typename B>
template<typename Edge>
typename ThreadBuilder<B>::Component *
ThreadBuilder<B>::merge_roots(size_type a, size_type b, Edge const & edge)
{
    typedef typename Builder::Handle Handle;
    // Lookup roots
    Handle ha = m_builder.m_rootFinder.find_update(a);
    Handle hb = m_builder.m_rootFinder.find_update(b);
    if(ha != hb)
    {
        // edge is in MST
        // get corresponding nodes
        Component * na = m_builder.m_rootFinder.data(ha);
        Component * nb = m_builder.m_rootFinder.data(hb);
        // simplify merge
        // !na && !nb -> swap (can be)
        // !na && nb -> swap (necessary)
        // na && !nb -> no swap
        // na && nb -> *na < *nb -> swap (necessary)
        if(!na || (nb && *na < *nb))
        {
            // Handles are not swapped, their order is not important            
            std::swap(a, b);
            std::swap(na, nb);
        }
        // Lift subtree A
        na = lift(a, na, edge);
        // Lift subtree B
        if(!nb)
        {
            na->link(tree().leaf(b));
        }
        else if((*nb < edge) || (*nb < *na)) // !!! The only difference from addEdge
        {
            m_roots.erase(m_roots.iterator_to(*nb));
            na->link(nb);
        }
        else
        {
            // When size is constant time, ensure smaller node is removed
            if(Component::constant_time_children_size &&
                (na->children.size() < nb->children.size()))
            {
                std::swap(na, nb);
            }
            // Mark node for removal
            m_roots.erase(m_roots.iterator_to(*nb));
            //nb->setParent(na);
            //m_redundant.push_front(*nb);
            // reconnect and delete node
            reconnect(nb, na);
            --m_component_count;
            m_pool.push_front(*nb);
        }
        // Merge trees in root finder
        m_builder.m_rootFinder.merge_set(ha, hb, na);
        return na;
    }
    else
    {
        return m_builder.m_rootFinder.data(ha);
    }
}

template<typename B>
template<typename Edge>
void ThreadBuilder<B>::merge_paths(size_type a, size_type b, Edge const & edge)
{
    // lookup from the bottom
    Leaf * la = tree().leaf(a);
    Leaf * lb = tree().leaf(b);
    // paths must converge somewhere
    BOOST_ASSERT(la->root() == lb->root());
    // validate paths
#ifndef NDEBUG
    {
        // path a
        Component * n = static_cast<Component*>(la->parent());
        Component * p = static_cast<Component*>(n->parent());
        while(p)
        {
            // node levels must be increasing
            BOOST_ASSERT(*n < *p);
            n = p;
            p = static_cast<Component*>(n->parent());
        }
        // path b
        n = static_cast<Component*>(lb->parent());
        p = static_cast<Component*>(n->parent());
        while(p)
        {
            BOOST_ASSERT(*n < *p);
            n = p;
            p = static_cast<Component*>(n->parent());
        }
    }
#endif
    // !!! a, b are not important now, call further to hide them
    merge_paths(la, lb, edge);
}

template<typename B>
template<typename Edge>
void ThreadBuilder<B>::merge_paths(Leaf * la, Leaf * lb, Edge const & edge)
{
    Component * na = static_cast<Component*>(la->parent());
    Component * nb = static_cast<Component*>(lb->parent());
    // na, nb are not null -> no unconnected leaves
    BOOST_ASSERT(na);
    BOOST_ASSERT(nb);
    // ensure node relationship to simplify following code
    if(*na < *nb)
    {
        std::swap(la, lb);
        std::swap(na, nb);
    }
    // NOW : na >= nb (+ na, nb != null)
    BOOST_ASSERT(*nb <= *na);
    // parent nodes of na and nb :
    Component * pa = static_cast<Component*>(na->parent());
    Component * pb = static_cast<Component*>(nb->parent());
    // edge can lie below nb (and below na)
    if(!(*nb <= edge))
    {
        BOOST_ASSERT(!(*na <= edge));
        // nb > e -> na > e
        // both na and nb lie above the edge, split both paths
        Component * n = alloc(edge);
        // relink everything
        BOOST_VERIFY(n->relink(la) == na);
        BOOST_VERIFY(n->relink(lb) == nb);
        nb->link(n);
        // ?     ?
        // |     |
        // na >= nb
        //       |
        //       n <- edge
        //     / |
        //    la lb
        // no need for lookup now, just zip
        BOOST_ASSERT(*nb <= *na);
        if(na == nb) //check if paths haven't already converged
            return;
        if(!(*nb < *na))
        {
            // na, nb at the same level -> absorb one
            //  so the outcome of this case matches the following
            BOOST_ASSERT(na->level() == nb->level());
            // When size is constant time, ensure smaller node is removed
            if(Component::constant_time_children_size &&
                (na->children.size() > nb->children.size()))
            {
                std::swap(na, nb);
                std::swap(pa, pb);
            }
            // move children of na to nb
            reconnect(na, nb);
            BOOST_ASSERT(na->empty());
            // delete na
            Component * old = na;
            na = static_cast<Component*>(na->unlink());
            --m_component_count;
            m_pool.push_front(*old);
        }
        // ?    ?
        // |    |
        // na > nb
        //      |
        //      n <- edge
        //    / |
        //   la lb
    }
    else if(!(*na <= edge))
    {
        BOOST_ASSERT(*nb <= edge);
        BOOST_ASSERT(pb);
        // na > e, nb <= e
        // ?   pb (might be na)
        // |   |
        // na  |
        // |   ?  <- edge
        // |   nb <-
        // |   |
        // la  lb
        // lookup path b
        while(*pb <= edge)
        {
            nb = pb;
            pb = static_cast<Component*>(pb->parent());
            BOOST_ASSERT(pb);
        }
        BOOST_ASSERT(*nb < *pb);
        // nb <= e, pb > e, we have insertion point
        nb = lift(nb, edge);
        // nb == e, na > e -> na > nb
        // relink leaf a to path b
        BOOST_VERIFY(nb->relink(la) == na);
        // ?    pb (might be na)
        // |     |
        // na > nb <- edge
        //      /|
        //     / ?
        //    /  |
        //   la  lb
    }
    else
    {
        // !!! la, lb are not important here
        BOOST_ASSERT(*na <= edge);
        BOOST_ASSERT(*nb <= edge);
        BOOST_ASSERT(pa);
        BOOST_ASSERT(pb);
        if(na == nb)
        {
            // nothing to do, paths are already merged
            return;
        }
        // na <= e, nb <= e
        // na >= nb
        // synchronous lookup
        // nb <= na -> try move it up
        while(*pb <= edge)
        {
            nb = pb;
            pb = static_cast<Component*>(pb->parent());
            if(na == nb)
            {
                // paths have converged, nothing to do
                return;
            }
            // check if nb moved above na
            if(*na < *nb)
            {
                std::swap(na, nb);
                std::swap(pa, pb);
            }
            BOOST_ASSERT(na);
            BOOST_ASSERT(nb);
            BOOST_ASSERT(*nb <= *na);
            BOOST_ASSERT(pa);
            BOOST_ASSERT(pb);
        }
        BOOST_ASSERT(*nb <= edge);
        BOOST_ASSERT(!(*pb <= edge));
        // finish traversal of path A, because pa can be <= edge
        // |  pb
        // ...   <- e
        // pa |
        // |  | 
        // na |
        // |  nb
        while(*pa <= edge)
        {
            na = pa;
            pa = static_cast<Component*>(pa->parent());
            BOOST_ASSERT(na);
            BOOST_ASSERT(*na <= edge);
            BOOST_ASSERT(pa);
        }
        BOOST_ASSERT(!(*pa <= edge));
        BOOST_ASSERT(*nb <= *na);
        // na, nb <= edge
        // pa, pb > edge
        // na >= nb
        na = lift(na, edge);
        // na == e, nb <= na
        if(*nb < edge)
        {
            nb = static_cast<Component*>(na->relink(nb));
        }
        else
        {
            // When size is constant time, ensure smaller node is removed
            if(Component::constant_time_children_size &&
                (na->children.size() < nb->children.size()))
            {
                std::swap(na, nb);
                std::swap(pa, pb);
            }
            // get rid of nb
            Component * n = static_cast<Component*>(nb->unlink());
            reconnect(nb, na);
            --m_component_count;
            m_pool.push_front(*nb);
            nb = n;
        }
        BOOST_ASSERT(nb == pb);
        pb = static_cast<Component*>(nb->parent());
        // pb > e, na = e -> pb > na
        // swap to match other cases
        std::swap(na, nb);
        std::swap(pa, pb);
        // ?    ?
        // |    |
        // na > nb
        //     /|
        //    ? ?
        //   /  |
        //  la  lb
    }
    BOOST_ASSERT(na);
    BOOST_ASSERT(nb);
    BOOST_ASSERT(pb);
    BOOST_ASSERT(na != nb);
    BOOST_ASSERT(*nb < *na);
    BOOST_ASSERT(*nb < *pb);
    // remove empty nodes from path A
    while(na->empty())
    {
        Component * n = static_cast<Component*>(na->unlink());
        BOOST_ASSERT(n);
        m_pool.push_front(*na);
        --m_component_count;
        na = n;
//        pa = static_cast<Component*>(pa->parent());
    }
    // we can zip now
    BOOST_ASSERT(*nb <= *na);
    BOOST_ASSERT(na);
#if 0
    //validate
    la = tree().leaf(a);
    lb = tree().leaf(b);
    BOOST_ASSERT(la->root() == lb->root());
    // validate paths
    {
        Component * n = static_cast<Component*>(la->parent());
        Component * p = static_cast<Component*>(n->parent());
        while(p)
        {
            BOOST_ASSERT(*n < *p);
            n = p;
            p = static_cast<Component*>(n->parent());
        }
        n = static_cast<Component*>(lb->parent());
        p = static_cast<Component*>(n->parent());
        while(p)
        {
            BOOST_ASSERT(*n < *p);
            n = p;
            p = static_cast<Component*>(n->parent());
        }
    }
#endif
//#if 0
    // traverse both paths up
    Component * node = nb;
    nb = static_cast<Component*>(nb->unlink());
    //nb = static_cast<Component*>(nb->parent());
    BOOST_ASSERT(na);
    BOOST_ASSERT(nb);
    while(na != nb)
    {
        Component * n = nullptr;
        if(*na < *nb)
        {
            n = na;
            na = static_cast<Component*>(na->unlink());
        }
        else if(*nb < *na)
        {
            n = nb;
            nb = static_cast<Component*>(nb->unlink());
        }
        else
        {
            // When size is constant time, ensure smaller node is removed
            if(Component::constant_time_children_size &&
                (na->children.size() < nb->children.size()))
            {
                std::swap(na, nb);
            }
            // |na| >= |nb|
            n = na;
            na = static_cast<Component*>(na->unlink());
            reconnect(nb, n);
            Component * old = nb;
            nb = static_cast<Component*>(nb->unlink());
            --m_component_count;
            m_pool.push_front(*old);
        }
        BOOST_ASSERT(na);
        BOOST_ASSERT(nb);
        n->link(node);
        node = n;
    }
    nb->link(node);
//#endif

    //Component * node = nb;
    //nb = static_cast<Component*>(nb->unlink());
    //BOOST_ASSERT(na);
    //BOOST_ASSERT(nb);
    //while(nb != na)
/*    {
        BOOST_ASSERT(*node <= *na);
        BOOST_ASSERT(*node <= *nb);
        if(*na < *nb)
        {
            std::swap(na, nb);
        }
        BOOST_ASSERT(*nb <= *na);*/
/*        if(*node < *nb)
        {
            nb->link(node);
            node = nb;
            nb = static_cast<Component*>(node->unlink());
        }
        else
        {
            reconnect(nb, node);
            Component * n = static_cast<Component*>(nb->unlink());
            m_pool.push_front(*nb);
            --m_component_count;
            nb = n;
        }*/
        //BOOST_ASSERT(na);
        //BOOST_ASSERT(nb);
    //}
    //nb->link(node);
/*        while(na->empty())
        {
            BOOST_ASSERT(pa);
            Component * n = static_cast<Component*>(na->unlink());
            m_pool.push_front(*na);
            --m_component_count;
            na = n;
        }
        BOOST_ASSERT(na);
        if(*nb < *na)
        {
        }
        else
        {
            reconnect(na, nb);
            Component * n = static_cast<Component*>(na->unlink());
            BOOST_ASSERT(n);
            --m_component_count;
            m_pool.push_front(*na);
            na = n;
        }*/
/*        else
        {
            BOOST_ASSERT(*nb < *na);
            if(*na < *pb)
            {
                na->relink(nb);
                nb = na;
                pb = pa;
                na = pb;
                pa = static_cast<Component*>(na->parent());
            }
            BOOST_ASSERT(*nb <= *na);
        }*/
    //}
}

#if 0
template<typename B>
template<typename Edge>
void ThreadBuilder<B>::prune(size_type a, size_type b, Edge const & edge)
{
    // Lookup the insertion point
    Component * na, * nb; // nodes below edge
    Component * pa, * pb; // their parents
    std::tie(na, pa) = insertionPoint(a, edge);
    std::tie(nb, pb) = insertionPoint(b, edge);
    //std::cout << "found " << a << " " << b << std::endl;
    if(!na || (na != nb))
    {
        // Union the n? nodes
        if(!na || (nb && *na < *nb))
        {
            std::swap(a, b);
            std::swap(na, nb);
            std::swap(pa, pb);
        }
        // Lift subtree A
        if(!na)
        {
            na = alloc(edge);
            na->relink(tree().leaf(a));
            pa->link(na);
        }
        else if(*na < edge)
        {
            Component * node = alloc(edge);
            node->relink(na);
            na = node;
            pa->link(na);
        }
        // Lift subtree B
        //EXPERIMENT : do the same as with a
        if(!nb)
        {
            nb = alloc(edge);
            nb->relink(tree().leaf(b));
            pb->link(nb);
        }
        else if(*nb < edge)
        {
            Component * node = alloc(edge);
            node->relink(nb);
            nb = node;
            pb->link(nb);
        }

        /*        if(!nb)
        {
            na->relink(tree().leaf(b));
        }
        else if(*nb < edge)
        {
            na->relink(nb);
        }
        else
        {
            // When size is constant time, ensure smaller node is removed
            if(Component::constant_time_children_size &&
                (na->children.size() < nb->children.size()))
            {
                std::swap(na, nb);
                std::swap(pa, pb);
            }
            // Mark node for removal
            nb->unlink();
            if(nb->empty())
            {
                m_pool.push_front(*nb);
            }
            else
            {
                nb->setParent(na);
                m_redundant.push_front(*nb);
            }
        }*/
        nb = nullptr; // has been dealt with
    }
#if 0
    // Zip paths
    while(pa != pb)
    {
        // na is linked to pa
        if(pa > pb)
        {
            na->unlink();
            std::swap(pa, pb);
            pa->link(na);
        }

        //Component * parent = static_cast<Component*>(pa->unlink());
        if(*na < *pa)
        {
            pa->link(na);
            na = pa;
        }
        else
        {
            // When size is constant time, ensure smaller node is removed
            if(Component::constant_time_children_size &&
                (na->children.size() < pa->children.size()))
            {
                std::swap(na, pa);
            }
            // Mark node for removal
            pa->setParent(na);
            m_redundant.push_front(*pa);
        }
        pa = parent;
    }
#endif
}
#endif
