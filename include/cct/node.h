#ifndef LIBCCT_NODE_H_INCLUDED
#define LIBCCT_NODE_H_INCLUDED

#include "utils/tagged_ptr.h"

#include <ostream>
#include <tuple>

#include <boost/assert.hpp>
#include <boost/intrusive/list.hpp>

namespace cct {

struct TreeParameters
{
    /**
     * Size optimization.
     * 32b size should be enough even on 64b systems
     */
    typedef uint32_t size_type;

    /** \brief Is child count cached?
     *
     * Memory/speed tradeoff.
     * With cached child count, it is possible to insert smaller nodes into bigger ones.
     */
    static constexpr bool constant_time_children_size = true;

    static constexpr size_t node_alignment_bits = 0;
    static constexpr size_t comp_alignment_bits = 1;

    // calculated stuff

    static constexpr size_t node_alignment = 1<<node_alignment_bits;
    static constexpr size_t comp_alignment = 1<<comp_alignment_bits;

    // invariants
    
    static_assert(comp_alignment_bits >= node_alignment_bits, "node base alignment");
};

typedef TreeParameters DefaultTreeParams;
    
namespace node {

const unsigned PARENT_ALIGNMENT_BITS = 1;

class Parent;
class Child;

class Child
    : public boost::intrusive::list_base_hook<>
{
    using ParentPtr = utils::tagged_ptr<Parent, PARENT_ALIGNMENT_BITS>;
protected :
    explicit Child(bool is_leaf) : m_parent(is_leaf ? 0 : 1) {}
public :
    Parent * parent() noexcept { return m_parent.get(); }
    Parent * setParent(Parent * p) noexcept
    {
        Parent * o = parent();
        m_parent.set(p);
        return o;
    }
private :
    ParentPtr m_parent;
};

class alignas(1<<PARENT_ALIGNMENT_BITS) Parent
{
    using Children = boost::intrusive::list<Child,
            boost::intrusive::constant_time_size<true>
        >;
public :
private :
    Children m_children;
};

class Root
    : public Parent
{
};

class Comp
    : public Parent, public Child
{
public :
    Comp() : Child(false) {}
};

class Leaf
    : public Child
{
public :
    Leaf() : Child(true) {}
};

}//namespace node    

template<typename TreeParams>
class NodeBase;
template<typename TreeParams>
class LeafBase;
template<typename TreeParams>
class ComponentBase;

/** Base class for both tree nodes.
 *
 * Contains stuff necessary for intrusive lists of nodes.
 * Contains link to parent component.
 * Allows bottom up traversal.
 *
 * \section nodes
 */
template<typename TreeParams>
class NodeBase
    : public boost::intrusive::list_base_hook<>
{
public :
    typedef TreeParams tree_params;
    typedef typename tree_params::size_type size_type;
    typedef NodeBase node_type;
    typedef ComponentBase<TreeParams> comp_type;
private :
    typedef utils::tagged_ptr<comp_type, TreeParams::comp_alignment_bits> ParentPtr;
public :
    typedef typename ParentPtr::tag_type tag_type;

    /** \brief Default constructor. parent=nullptr, tag=0 */
    NodeBase() noexcept
        : m_parent(nullptr) {}

    explicit NodeBase(tag_type tag) noexcept
        : m_parent(nullptr, tag) {}

    NodeBase(comp_type * par, tag_type tag) noexcept
        : m_parent(par, tag) {}

    // Node is noncopyable
    NodeBase(node_type const &) = delete;
    NodeBase & operator=(node_type const &) = delete;

    /** Returns parent node.
     * root -> nullptr
     */
    comp_type       * parent()       noexcept
    {
        return m_parent.get();
    }
    comp_type const * parent() const noexcept
    {
        return m_parent.get();
    }

    /** 
     * Few bits of info, packed into parent ptr.
     *
     * tag == 0 -> leaf
     * tag != 0 -> component
     * TODO : Something else?
     */
    tag_type tag() const noexcept
    {
        return m_parent.get_tag();
    }

    bool isLeaf() const noexcept
    {
        return tag() == 0;
    }
    bool isComponent() const noexcept
    {
        return tag() != 0;
    }
    bool isRoot() const noexcept
    {
        return !parent();
    }

    /** \brief Set new parent node. */
    void setParent(comp_type * node) noexcept;

    /** \brief Traverse up, return node with parent == nullptr.
     *
     * O(H)
     */
    comp_type * root() noexcept;

    /** \brief Unlink a node from its parent
     * postconditions :
     *  parent() == nullptr
     * returns :
     *  previous parent (or nullptr)
     */
    comp_type * unlink() noexcept;

    /** Is a node ancestor of *this?
     *
     * O(H)
     */
    bool isLinkedTo(comp_type const * ancestor) const noexcept;

    /** \brief Is this component subset of node?
     */
    bool isSubOf(comp_type const * node) const noexcept
    {
        return (this == node) || isLinkedTo(node);
    }
private :
    ParentPtr m_parent;
};

/** Base class of leaf nodes.
 * \section nodes
 */
template<typename TreeParams>
struct LeafBase
    : public NodeBase<TreeParams>
{
    typedef NodeBase<TreeParams> Base;
    typedef typename Base::size_type size_type;
    typedef typename Base::comp_type comp_type;

    LeafBase() noexcept
        : Base(0) {}

    ~LeafBase() noexcept
    {
        BOOST_ASSERT(Base::isLeaf());
    }

    /** \brief Traverse tree and calculate height.
     *
     * O(H)
     */
    size_type calculateHeight() const noexcept;
};

/** Base class of inner nodes.
 *
 * Contains list of child nodes, allows iterating over them.
 * Contains methods for node merging.
 *
 * \section nodes
 */
template<typename TreeParams>
class ComponentBase
    : public NodeBase<TreeParams>
{
    friend class NodeBase<TreeParams>;
    using Node = NodeBase<TreeParams>;
public :
    using Leaf = LeafBase<TreeParams>;
    using size_type = typename Node::size_type;

//    typedef typename Base::node_type node_type;
//    typedef typename Base::size_type size_type;
//    typedef LeafBase<TreeParams> leaf_type;
    static constexpr bool constant_time_children_size
        = TreeParams::constant_time_children_size;
private :
    using ChildList = boost::intrusive::list<Node,
            boost::intrusive::size_type<size_type>,
            boost::intrusive::constant_time_size<
                TreeParams::constant_time_children_size>
        >;
public :
    /** Iterator
     * \subsection component_container
     */
    typedef typename ChildList::      iterator       iterator;
    /** Const Iterator
     * \subsection component_container
     */
    typedef typename ChildList::const_iterator const_iterator;

    bool empty() const noexcept
    {
        return children.empty();
    }

    /**
     * Depending on config either O(1) or O(N)
     */
    size_type size() const noexcept
    {
        return children.size();
    }

    const_iterator begin() const noexcept
    {
        return children.begin();
    }
          iterator begin()       noexcept
    {
        return children.begin();
    }

    const_iterator end() const noexcept
    {
        return children.end();
    }
          iterator end()       noexcept
    {
        return children.end();
    }

    Node const & front() const noexcept
    {
        return children.front();
    }
    Node       & front()       noexcept
    {
        return children.front();
    }

    ComponentBase() noexcept : Node(1) {}
    
    ~ComponentBase() noexcept
    {
        BOOST_ASSERT(this->isComponent());//tag is intact
        BOOST_ASSERT(empty());
    }

    /** Node is degenerate if it has 0 or 1 children.
     */
    bool isDegenerate() const noexcept
    {
        return empty() || (begin()->isComponent() && (++begin() == end()));
    }

    /** \brief Link leaf to parent(this)
     * preconditions :
     *  parent() == nullptr
     * postconditions :
     *  leaf is linked to <this>
     */
    void link(Leaf * leaf) noexcept;

    /** see link(Leaf *)
     */
    void link(ComponentBase * node) noexcept;

    /** Unlink node and link to new parent(this)
     * postconditions :
     *  child is linked to parent
     * \return previous parent
     */
    template<typename Child>
    ComponentBase * relink(Child * child) noexcept;

    /** \brief Reconnect children of node to this */
    void absorb(ComponentBase & node) noexcept;

    /** \brief children.size() < c.children.size() if possible in O(1)
     */
    bool smallerThanInO1(ComponentBase const & c) const noexcept;

    template<typename ComponentFunctor>
    void forEachChild(ComponentFunctor c) noexcept
    {
        typename ChildList::iterator i = children.begin();
        while((i != children.end()) && i->isComponent())
        {
            c(static_cast<ComponentBase&>(*i)); ++i;
        }
    }

    template<typename ComponentFunctor, typename LeafFunctor>
    void forEachChild(ComponentFunctor c, LeafFunctor l) noexcept
    {
        typename ChildList::iterator i = children.begin();
        while((i != children.end()) && i->isComponent())
        {
            c(static_cast<ComponentBase&>(*i)); ++i;
        }
        while(i != children.end())
        {
            l(static_cast<Leaf&>(*i)); ++i;
        }
    }
private :
    /** Intrusive list of child nodes.
     * 2 pointers
     * list is partially sorted, so leaves are after internal nodes
     */
    ChildList children;
};

template<typename P>
inline std::ostream & operator<<(std::ostream & o, ComponentBase<P> const &)
{
    return o;
}
template<typename P>
inline std::ostream & operator<<(std::ostream & o, LeafBase<P> const &)
{
    return o;
}

#include "node.inl"

}//namespace cct

#endif//LIBCCT_NODE_H_INCLUDED
