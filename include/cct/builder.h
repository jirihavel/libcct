#include "node.h"
#include "root_finder.h"

#include <boost/assert.hpp>
#include <boost/intrusive/list.hpp>

#include <iostream>

namespace cct {

/* Builder holds tree specific state */
template<typename TreeType>
class Builder;

/* \brief ThreadBuilder holds thread specific state
 * - object pool
 * - component count
 * - root list
 */
template<typename BuilderType>
class ThreadBuilder;

template<typename TreeType>
class Builder
{
    friend class ThreadBuilder<Builder>;

public :
    typedef TreeType Tree;
    typedef typename Tree::Component Component;
    typedef typename Tree::Leaf      Leaf;
    typedef typename Tree::Node      Node;

    typedef typename Tree::size_type size_type;

private :
    typedef PackedRootFinder<Component *, size_type> RootFinder;
    typedef typename RootFinder::Handle Handle;

    Tree * m_tree;

    RootFinder m_rootFinder;

public :
    constexpr Builder()
        : m_tree(nullptr)
    {}

    explicit Builder(Tree * tree)
        : m_tree(tree), m_rootFinder(tree->leafCount(), nullptr)
    {}

    Builder(Tree * tree, size_type lc)
        : m_tree(tree->init(lc)), m_rootFinder(tree->leafCount(), nullptr)
    {}

    ~Builder() noexcept
    {
    }

    Builder(Builder const &) = delete;
    Builder & operator=(Builder const &) = delete;

    void reset(size_type b, size_type e)
    {
        BOOST_ASSERT(m_tree);
        m_rootFinder.resetRange(b, e, nullptr);
    }

    void reset()
    {
        BOOST_ASSERT(m_tree);
        m_rootFinder.reset(nullptr);
    }

    void init(Tree * tree)
    {
        m_tree = tree;
        m_rootFinder.init(tree->leafCount(), nullptr);
    }

    Tree const * tree() const
    {
        return m_tree;
    }

    void finish(ThreadBuilder<Builder> && tb);
};

template<typename BuilderType>
class ThreadBuilder
{
public :
    typedef BuilderType Builder;
    typedef typename Builder::Tree Tree;
    typedef typename Builder::size_type size_type;
    typedef typename Builder::Component Component;
    typedef typename Builder::Leaf Leaf;

private :
    friend class cct::Builder<Tree>;

    typedef boost::intrusive::list<
            Component,
            boost::intrusive::size_type<typename Builder::size_type>,
            boost::intrusive::constant_time_size<false>
        > Components;

    Builder & m_builder;

    // thread-local component counter
    // will be added to tree component counnt
    // = nodes in subtrees from m_roots + m_pool.size()
    size_type m_component_count;

    // Roots of trees owned by this thread
    Components m_roots;

    // Components marked for removal
    Components m_redundant;

    // Component pool
    Components m_pool;

    template<typename Edge>
    Component * alloc(Edge const & edge);

    template<typename Edge>
    Component * lift(size_type i, Component * node, Edge const & edge);

    template<typename Edge>
    Component * lift(Component * node, Edge const & edge);

    /** \brief Reconnect children of <from> to node <to> */
    void reconnect(Component * from, Component * to);

    /**
     * returns (a,b), a <= e, b > e
     */
    template<typename Edge>
    std::tuple<Component *, Component *> insertionPoint(size_type i, Edge const & e)
    {
        Component * a = nullptr;
        Component * b = static_cast<Component*>(tree().leaf(i)->parent());
        while(b && (*b <= e))
        {
            a = b;
            b = static_cast<Component*>(b->parent());
        }
        return std::make_tuple(a, b);
    }

    // Implementation of merge_paths
    template<typename Edge>
    void merge_paths(Leaf * la, Leaf * lb, Edge const & edge);
public :
    explicit ThreadBuilder(Builder & b)
        : m_builder(b), m_component_count(0)
    {}

    ThreadBuilder(ThreadBuilder const &) = delete;
    ThreadBuilder & operator=(ThreadBuilder const &) = delete;

    ~ThreadBuilder() noexcept;

    /** \brief Tree will be accesses a lot. */
    Tree & tree()
    {
        return *m_builder.m_tree;
    }

    Builder & builder()
    {
        return m_builder;
    }

    size_type pool_size() const
    {
        return m_pool.size();
    }

    /** \brief After one thread finishes, its builder is absorbed by another one.
     */
    void absorb(ThreadBuilder && b);

    // Singlethreaded construction :

    /**
     * REQUIRES :
     *  Component < Vertex : true - create new node
     *   c < v -> level(c) < level(v)
     */
    template<typename Vertex>
    void addVertex(size_type i, Vertex const & vertex);

    /**
     * REQUIRES :
     *  Component < Component
     *   a < b -> level(a) <= level(b)
     *  Component < Edge : true - create new node
     *   c < e -> level(c) < level(e)
     *
     * \returns Newly added component or NULL if no component was added
     */
    template<typename Edge>
    Component * addEdge(size_type a, size_type b, Edge const & edge);

    /** \brief Remove marked components.
     */
    void remove();

    // Multithreaded construction :

    /** \brief Merge two trees together.
     */
    template<typename Edge>
    Component * merge_roots(size_type a, size_type b, Edge const & edge);

    /** \brief Merge two paths in a tree.
     * Assumes that both leaves share a common root.
     */
    template<typename Edge>
    void merge_paths(size_type a, size_type b, Edge const & edge);
};

#include "builder.inl"

}//namespace cct
