//---------------------
// -- Node --
//--------------------

template<typename P>
inline void NodeBase<P>::setParent(ComponentBase<P> * node) noexcept
{
    m_parent.set(node);
}

template<typename P>
inline ComponentBase<P> * NodeBase<P>::root() noexcept
{
    comp_type * n = parent();
    if(n)
    {
        comp_type * p = n->parent();
        while(p)
        {
            n = p;
            p = p->parent();
        }
    }
    return n;
}

template<typename P>
inline ComponentBase<P> * NodeBase<P>::unlink() noexcept
{
    comp_type * p = parent();
    if(p)
    {
        p->children.erase(p->children.iterator_to(*this));
        setParent(nullptr);
    }
    return p;
}

template<typename P>
inline bool NodeBase<P>::isLinkedTo(ComponentBase<P> const * ancestor) const noexcept
{
    node_type const * node = this;
    while(node)
    {
        node = node->parent();
        if(node == ancestor)
            return true;
    }
    return false;
}

//---------------------
// Leaf stuff
//--------------------

template<typename P>
inline typename LeafBase<P>::size_type LeafBase<P>::calculateHeight() const noexcept
{
    size_type n = 0;
    comp_type const * node = Base::parent();
    while(node)
    {
        ++n;
        node = node->parent();
    }
    return n;
}

//---------------------
// Component stuff
//--------------------

template<typename P>
inline void ComponentBase<P>::link(LeafBase<P> * leaf) noexcept
{
    BOOST_ASSERT(!leaf->parent());
    leaf->setParent(this);
    children.push_back(*leaf);//insert leaves at the end
}

template<typename P>
inline void ComponentBase<P>::link(ComponentBase<P> * node) noexcept
{
    BOOST_ASSERT(!node->parent());
    node->setParent(this);
    children.push_front(*node);//insert components at the beginning
}

template<typename P>
template<typename Child>
inline ComponentBase<P> * ComponentBase<P>::relink(Child * child) noexcept
{
    ComponentBase * old = child->unlink();
    link(child);
    return old;
}

template<typename P>
inline void ComponentBase<P>::absorb(ComponentBase<P> & node) noexcept
{
    if(!node.empty())
    {
        iterator i = node.begin();
        size_type n = 0;
        // go over child components
        while((i != node.end()) && i->isComponent())
        {
            i->setParent(this);
            ++i;
            ++n;// count children for O(1) splice
        }
        // i now points to first leaf of node
        // insert components at the beginning of child list
        children.splice(begin(), node.children, node.begin(), i, n);
        // go over child leaves
        while(i != node.end())
        {
            i->setParent(this);
            ++i;
        }
        // insert leaves at the end
        children.splice(end(), node.children);
    }
}

template<typename P>
inline bool ComponentBase<P>::smallerThanInO1(ComponentBase<P> const & c) const noexcept
{
    return P::constant_time_children_size
        && (children.size() < c.children.size());
}
