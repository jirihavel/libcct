#ifndef CONNETED_COMPONENT_TREE_NODE_H_INCLUDED
#define CONNETED_COMPONENT_TREE_NODE_H_INCLUDED

#include "utils/tagged_ptr.h"

#include <ostream>
#include <tuple>

#include <boost/assert.hpp>
#include <boost/intrusive/list.hpp>

namespace cct {

struct TreeParameters
{
    typedef uint32_t size_type;

    static constexpr bool constant_time_children_size = true;
};
    
namespace base {
/*
class Parent;
class Child;

class Child
    : public boost::intrusive::list_base_hook<>
{
    Parent * m_parent;
public :
};

class Parent
{
    typedef boost::intrusive::list<
            Child,
            boost::intrusive::constant_time_size<false>
        > Children;

    Children m_children;
public :
};
*/
}//namespace base    

struct NodeBase;
struct LeafBase;
struct ComponentBase;

constexpr size_t NODE_ALIGNMENT_BITS = 2;

/** Base class for both tree nodes.
 *
 * Contains stuff necessary for intrusive lists of nodes.
 * Contains link to parent component.
 * Allows bottom up traversal.
 *
 * \section nodes
 */
class alignas(1<<NODE_ALIGNMENT_BITS) NodeBase
    : public boost::intrusive::list_base_hook<>
{
private :
    typedef utils::tagged_ptr<ComponentBase, NODE_ALIGNMENT_BITS> ParentPtr;

    ParentPtr m_parent;
public :
    typedef NodeBase Node;

    typedef typename ParentPtr::tag_type Tag;

    /**
     * Size optimization.
     * 32b size should be enough even on 64b systems
     */
    typedef uint32_t size_type;

    /** \brief Default constructor. parent=nullptr, tag=0 */
    NodeBase() noexcept
        : m_parent(nullptr) {}

    explicit NodeBase(Tag tag) noexcept
        : m_parent(nullptr, tag) {}

    // Node is noncopyable
    NodeBase(Node const &) = delete;
    NodeBase & operator=(Node const &) = delete;

    /** Returns parent node.
     * root -> nullptr
     */
    ComponentBase       * parent()       noexcept
    {
        return m_parent.get();
    }
    ComponentBase const * parent() const noexcept
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
    Tag tag() const noexcept
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

    /** \brief Set new parent node. */
    void setParent(ComponentBase * node) noexcept;

    /** \brief Traverse up, return node with parent == nullptr. */
    ComponentBase * root() noexcept;

    /** \brief Unlink a node from its parent
     * postconditions :
     *  parent() == nullptr
     * returns :
     *  previous parent (or nullptr)
     */
    ComponentBase * unlink() noexcept;

    /** Is a node ancestor of *this?
     */
    bool isLinkedTo(ComponentBase const * ancestor) const noexcept;
};

/** Base class of leaf nodes.
 * \section nodes
 */
struct LeafBase
    : public NodeBase
{
    typedef typename NodeBase::size_type size_type;

    ~LeafBase() noexcept
    {
        // Tag is intact
        BOOST_ASSERT(isLeaf());
    }

    /** \brief Traverse tree and calculate height.
     *
     * O(tree_height)
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
struct ComponentBase
    : public NodeBase
{
    /** \brief Is child count cached?
     *
     * Memory/speed tradeoff.
     * With cached child count, it is possible to insert smaller nodes into bigger ones.
     */
    static constexpr bool constant_time_children_size = true;

    typedef typename NodeBase::size_type size_type;
//private :
    /** Intrusive list of child nodes.
     * 2 pointers
     */
    typedef boost::intrusive::list<Node,
        boost::intrusive::size_type<size_type>,
        boost::intrusive::constant_time_size<constant_time_children_size>
        > Children;

    //list is partially sorted, so leaves are after internal nodes
    Children children;
public :
    /** Iterator
     * \subsection component_container
     */
    typedef typename Children::      iterator       iterator;
    /** Const Iterator
     * \subsection component_container
     */
    typedef typename Children::const_iterator const_iterator;

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
    const_iterator end() const noexcept
    {
        return children.end();
    }
    iterator begin() noexcept
    {
        return children.begin();
    }
    iterator end() noexcept
    {
        return children.end();
    }

    ComponentBase() noexcept
        : NodeBase(1) {}
    
    ~ComponentBase() noexcept
    {
        // Tag is intact
        BOOST_ASSERT(isComponent());
        // Child list must be empty when destroyed
        BOOST_ASSERT(empty());
    }

    /** Node is degenerate if it has 0 or 1 children.
     */
    bool isDegenerate() const noexcept
    {
        return empty() || (++begin() == end());
    }

    /** \brief Link leaf to parent(this)
     * preconditions :
     *  parent() == nullptr
     * postconditions :
     *  leaf is linked to <this>
     */
    void link(LeafBase * leaf) noexcept;

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
};

std::ostream & operator<<(std::ostream & o, ComponentBase const &)
{
    return o;
}
std::ostream & operator<<(std::ostream & o, LeafBase const &)
{
    return o;
}

//implementations are in separate file
#include "node.inl"

}//namespace cct

#endif//CONNETED_COMPONENT_TREE_NODE_H_INCLUDED
