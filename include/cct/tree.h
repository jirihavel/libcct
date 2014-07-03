#ifndef CONNECTED_COMPONENT_TREE_TREE_H_INCLUDED
#define CONNECTED_COMPONENT_TREE_TREE_H_INCLUDED

#include "node.h"

#include "utils/fp.h"

#include <boost/scoped_array.hpp>

namespace cct {

template<typename TreeType>
class Builder;

template<typename ComponentType, typename LeafType = LeafBase>
class Tree
{
    friend class cct::Builder<Tree>;
public :
    typedef ComponentType Component;
    typedef LeafType      Leaf;
    typedef typename Leaf::Node Node;

    static_assert(std::is_same<typename Leaf::Node, typename Component::Node>::value, "Mismatched base nodes");

    typedef typename Node::size_type size_type;

    typedef size_type LeafId;
private :
    typedef boost::intrusive::list<
            Component,
            boost::intrusive::size_type<size_type>,
            boost::intrusive::constant_time_size<true>
        > Roots;

    Roots roots;

    size_type component_count;

    size_type leaf_count;
    boost::scoped_array<Leaf> leaves;

public :
    size_type rootCount() const
    {
        return roots.size();
    }

    void addRoot(Component * root)
    {
        BOOST_ASSERT(!root->parent());
        roots.push_back(*root);
    }

    void remRoot(Component * root)
    {
        BOOST_ASSERT(!root->parent());
        roots.erase(roots.iterator_to(*root));
    }

    void resetRange(size_type b, size_type e)
    {
        for(size_type i = b; i != e; ++i)
        {
            Component * node = static_cast<Component*>(leaf(i)->unlink());
            while(node && node->empty())
            {
                Component * parent = static_cast<Component*>(node->unlink());
                if(!parent)
                {
                    remRoot(node);
                }
                delete node;
                BOOST_VERIFY(component_count-- > 0);
                node = parent;
            }
        }
    }

    void reset()
    {
        resetRange(0, leaf_count);
        BOOST_ASSERT(roots.empty());
        BOOST_ASSERT(component_count == 0);
    }

    void kill()
    {
        reset();
        // free leaves
        leaf_count = 0;
        leaves.reset();
    }

    void init(size_type lc)
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

    Tree()
        : component_count(0), leaf_count(0)
    {}

    explicit Tree(size_type lc)
        : component_count(0), leaf_count(0)
    {
        init(lc);
    }

    // Tree is noncopyable
    Tree(Tree const &) = delete;
    Tree & operator=(Tree const &) = delete;

    ~Tree() noexcept
    {
        // reset is enough. Leaves will be freed automatically.
        reset();
    }

    size_type nodeCount() const noexcept
    {
        return leaf_count + component_count;
    }
    size_type leafCount() const noexcept
    {
        return leaf_count;
    }
    size_type componentCount() const noexcept
    {
        return component_count;
    }

    bool isLeafId(size_type i) const
    {
        return (0 <= i) && (i < leafCount());
    }

    Leaf * leaf(size_type i)
    {
        BOOST_ASSERT(isLeafId(i));
        return leaves.get() + i;
    }

    Leaf const * leaf(size_type i) const
    {
        BOOST_ASSERT(isLeafId(i));
        return leaves.get() + i;
    }

    bool isNodeLeaf(Node const & node) const noexcept
    {
        return node.isLeaf();
        //return (leaves.get() <= &node) && (&node < leaves.get()+leaf_count);
    }
    bool isNodeComponent(Node const & node) const noexcept
    {
        return node.isComponent();
        //return (leaves.get() <= &node) && (&node < leaves.get()+leaf_count);
    }

    size_type leafId(Leaf const & leaf) const
    {
        BOOST_ASSERT(isNodeLeaf(leaf));
        return &leaf - leaves.get();
    }
/*
    template<typename ComponentCompare, typename LeafCompare>
    void sort(Component & node, ComponentCompare c, LeafCompare l)
    {
        typename Component::iterator i = node.begin();
        while((i != node.end()) && !isNodeLeaf(*i))
        {
            sort(static_cast<Component &>(*i), c, l);
            ++i;
        }
        node.sort(c, l, i);
    }

    template<typename ComponentCompare, typename LeafCompare>
    void sort(ComponentCompare c, LeafCompare l)
    {
        for(Component & root : roots)
            sort(root, c, l);
        roots.sort(c);
    }
*/
    template<
        typename PreComponentFunction,
        typename PostComponentFunction,
        typename LeafFunction
    >
    void forEach(
        Component const & node,
        PreComponentFunction pre_func,
        PostComponentFunction post_func,
        LeafFunction leaf_func
    ) const
    {
        pre_func(node);
        auto i = node.begin();
        while((i != node.end()) && isNodeComponent(*i))
        {
            forEach(static_cast<Component const &>(*i), pre_func, post_func, leaf_func);
            ++i;
        }
        while(i != node.end())
        {
            leaf_func(static_cast<Leaf const &>(*i));
            ++i;
        }
        post_func(node);
    }

    template<
        typename PreComponentFunction,
        typename PostComponentFunction,
        typename LeafFunction
    >
    void forEach(
        PreComponentFunction pre_func,
        PostComponentFunction post_func,
        LeafFunction leaf_func
    ) const
    {
        for(Component const & root : roots)
        {
            forEach(root, pre_func, post_func, leaf_func);
        }
    }

    size_type countDegenerateComponents() const;

    /**
     * Naive and slow 
     * O(leaf_count*tree_height)
     */
    size_type calculateHeight() const;

    void prettyPrint(std::ostream & out, Leaf const & leaf);
    void prettyPrint(std::ostream & out, Node const & node, std::string indent, bool last);
    void prettyPrint(std::ostream & out);
};

#include "tree.inl"

}//namespace cct

#endif//CONNECTED_COMPONENT_TREE_TREE_H_INCLUDED
