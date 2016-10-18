#ifndef LIBCCT_TREE_H_INCLUDED
#define LIBCCT_TREE_H_INCLUDED

#include "node.h"
#include "builder.h"

#include "utils/fp.h"

#include <algorithm>

#include <boost/scoped_array.hpp>

namespace cct {

template<typename ComponentType, typename LeafType>
class Tree
{
    friend class TreeBuilder<Tree>;
    friend class AlphaTreeBuilder<Tree>;
    friend class AltitudeTreeBuilder<Tree>;
public :
    typedef ComponentType Component;
    typedef LeafType      Leaf;

    static_assert(std::is_same<typename Leaf::node_type, typename Component::node_type>::value, "Node base mismatch");

    typedef typename Leaf::node_type Node;
    typedef typename Node::size_type size_type;

    using Level = typename Component::Level;

    typedef size_type RootCount;

    typedef size_type LeafIndex;
    typedef size_type LeafCount;

    typedef Component * CompHandle;
    typedef Leaf      * LeafHandle;
    typedef Node      * NodeHandle;
private :
    typedef boost::intrusive::list<
            Component,
            boost::intrusive::size_type<size_type>,
            boost::intrusive::constant_time_size<true>
        > Roots;

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
public :
    bool isNodeComponent(Node const & node) const noexcept
    {
        BOOST_ASSERT(node.isComponent() == ((&node < leaves.get()) || (leaves.get()+leaf_count <= &node)));
        return node.isComponent();
    }
    bool isNodeLeaf(Node const & node) const noexcept
    {
        BOOST_ASSERT(node.isLeaf() == ((leaves.get() <= &node) && (&node < leaves.get()+leaf_count)));
        return node.isLeaf();
    }

    Component * n2c(Node * n) const noexcept
    {
        BOOST_ASSERT(!n || isNodeComponent(*n));
        return static_cast<Component *>(n);
    }
    Leaf * n2l(Node * n) const noexcept
    {
        BOOST_ASSERT(!n || isNodeLeaf(*n));
        return static_cast<Leaf *>(n);
    }

    /** \brief Disconnect leaves [b,e[ and remove redundant nodes */
    void resetRange(size_type b, size_type e) noexcept;
    void reset() noexcept;
    void kill() noexcept;

    void init(size_type lc);

    Tree() noexcept : component_count(0), leaf_count(0) {}
    explicit Tree(size_type lc) : Tree() { init(lc); }

    // Tree is noncopyable
    Tree(Tree const &) = delete;
    Tree & operator=(Tree const &) = delete;

    ~Tree() noexcept { reset(); }

    size_type nodeCount() const noexcept
    {
        return leaf_count + component_count;
    }
    size_type leafCount() const noexcept
    {
        return leaf_count;
    }
    size_type compCount() const noexcept
    {
        return component_count;
    }
    size_type rootCount() const noexcept
    {
        return roots.size();
    }

    Component const * root() const noexcept
    {
        return roots.size() == 1 ? &roots.front() : nullptr;
    }

    bool isLeafId(size_type i) const noexcept
    {
        return (0 <= i) && (i < leafCount());
    }

    size_type leafId(Leaf const & leaf) const noexcept
    {
        BOOST_ASSERT(isNodeLeaf(leaf));
        return &leaf - leaves.get();
    }

    Leaf * leaf(size_type i) noexcept
    {
        BOOST_ASSERT(isLeafId(i));
        return leaves.get() + i;
    }
    Leaf const * leaf(size_type i) const noexcept
    {
        BOOST_ASSERT(isLeafId(i));
        return leaves.get() + i;
    }

    template<typename PreComp, typename PostComp, typename LeafFunc>
    void forEach(
            Component const & node,
            PreComp pre, PostComp post, LeafFunc leaf
        ) const
    {
        pre(node);
        auto i = node.begin();
        while((i != node.end()) && i->isComponent())
        {
            forEach(static_cast<Component const &>(*i), pre, post, leaf);
            ++i;
        }
        while(i != node.end())
        {
            leaf(static_cast<Leaf const &>(*i));
            ++i;
        }
        post(node);
    }

    template<typename PreComp, typename PostComp, typename LeafFunc>
    void forEach(PreComp pre, PostComp post, LeafFunc leaf) const
    {
        for(Component const & root : roots)
        {
            forEach(root, pre, post, leaf);
        }
    }

    template<typename Pre, typename Post>
    void forEachComp(
            Component const & node,
            Pre pre, Post post
        ) const
    {
        pre(node);
        auto i = node.begin();
        while((i != node.end()) && i->isComponent())
        {
            forEachComp(static_cast<Component const &>(*i), pre, post);
            ++i;
        }
        post(node);
    }

    template<typename Pre, typename Post = utils::fp::ignore>
    void forEachComp(Pre pre, Post post = Post()) const
    {
        for(Component const & root : roots)
        {
            forEachComp(root, pre, post);
        }
    }

    template<typename LeafFunc>
    void forEachLeaf(LeafFunc leaf) const
    {
        std::for_each(leaves.get(), leaves.get()+leafCount(), std::move(leaf));
    }

    size_type countDegenerateComponents() const;

    /**
     * Naive and slow 
     * O(leaf_count*tree_height)
     */
    size_type calculateHeight() const;

    template<typename Printer>
    void prettyPrint(std::ostream & out, Printer const &, Node const & node, std::string indent, bool last);
    template<typename Printer>
    void prettyPrint(std::ostream & out, Printer const &);
private :
    Roots roots;

    size_type component_count;

    std::unique_ptr<Leaf[]> leaves;
    size_type leaf_count;
};

template<typename Tree>
class Printer
{
public :
    std::ostream & print(std::ostream & o, Tree const & t, typename Tree::Component const & c) const
    {
        return o << '[' << c << ']';
    }
    std::ostream & print(std::ostream & o, Tree const & t, typename Tree::Leaf const & l) const
    {
        return o << '(' << t.leafId(l) << ':' << l << ')';
    }
};

#include "tree.inl"

}//namespace cct

#endif//LIBCCT_TREE_H_INCLUDED
