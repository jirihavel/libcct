//---------------------
// Node stuff
//--------------------

inline void NodeBase::setParent(ComponentBase * node) noexcept
{
    m_parent.set(node);
}

inline ComponentBase * NodeBase::root() noexcept
{
    ComponentBase * n = parent();
    if(n)
    {
        ComponentBase * p = n->parent();
        while(p)
        {
            n = p;
            p = p->parent();
        }
    }
    return n;
}

inline ComponentBase * NodeBase::unlink() noexcept
{
    ComponentBase * p = parent();
    if(p)
    {
        p->children.erase(p->children.iterator_to(*this));
        setParent(nullptr);
    }
    return p;
}

inline bool NodeBase::isLinkedTo(ComponentBase const * ancestor) const noexcept
{
    Node const * node = this;
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

inline LeafBase::size_type LeafBase::calculateHeight() const noexcept
{
    size_type n = 0;
    ComponentBase const * node = parent();
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

inline void ComponentBase::link(LeafBase * leaf) noexcept
{
    BOOST_ASSERT(!leaf->parent());
    leaf->setParent(this);
    children.push_back(*leaf);//insert leaves at the end
}

inline void ComponentBase::link(ComponentBase * node) noexcept
{
    BOOST_ASSERT(!node->parent());
    node->setParent(this);
    children.push_front(*node);//insert components at the beginning
}

template<typename Child>
inline ComponentBase * ComponentBase::relink(Child * child) noexcept
{
    ComponentBase * old = child->unlink();
    link(child);
    return old;
}

inline void ComponentBase::absorb(ComponentBase & node) noexcept
{
    if(!node.empty())
    {
        iterator i = node.begin();
        // go over child components
        while((i != node.end()) && i->isComponent())
        {
            i->setParent(this);
            ++i;
        }
        // i now points to first leaf of node
        // insert components at the beginning of child list
        children.splice(begin(), node.children, node.begin(), i);
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
