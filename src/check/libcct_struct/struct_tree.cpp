#include "cct/image_tree.h"
#include "cct/builder.h"

#include <boost/test/unit_test.hpp>

template<typename Comp, typename Leaf>
void check_tree(cct::Tree<Comp, Leaf> const & tree)
{
    BOOST_CHECK_EQUAL(tree.nodeCount(), tree.leafCount() + tree.compCount());

    auto const root = tree.root();
    BOOST_CHECK(root);
    BOOST_CHECK(root->isRoot());

    size_t lc = 0;

    tree.forEachLeaf(
        [root,&lc](Leaf const & l)
        {
            ++lc;
            BOOST_CHECK(l.parent());
            BOOST_CHECK(l.isLinkedTo(root));
        });

    BOOST_CHECK_EQUAL(lc, tree.leafCount());

    lc = 0;
    size_t nca = 0;
    size_t ncb = 0;

    tree.forEach(
        [root,&nca](Comp const & c)
        {
            ++nca;
            auto parent = static_cast<Comp const*>(c.parent());
            if(parent)
            {
                BOOST_CHECK(c.isLinkedTo(root));
            }
        },
        [&ncb](Comp const &)
        {
            ++ncb;
        },
        [root,&lc](Leaf const &)
        {
            ++lc;
        });

    BOOST_CHECK_EQUAL(lc, tree.leafCount());
    BOOST_CHECK_EQUAL(nca, tree.compCount());
    BOOST_CHECK_EQUAL(ncb, tree.compCount());
}
    
template<typename Comp, typename Leaf>
void check_alpha(cct::Tree<Comp, Leaf> const & tree)
{
    check_tree(tree);

    tree.forEachComp(
        [](Comp const & c)
        {
            auto parent = static_cast<Comp const*>(c.parent());
            if(parent)
            {
                BOOST_CHECK_LT(c, *parent);
                BOOST_CHECK_LT(c.level(), parent->level());
            }
        },
        utils::fp::ignore());
}

BOOST_AUTO_TEST_CASE( struct_tree )
{
    static uint8_t const leaf_count = 8;
    static cct::Edge<uint8_t, uint8_t> const edges[] = {
        {0,1,0},
        {0,4,0},
        {6,7,0},
        {3,7,1},
        {4,5,1},
        {1,2,2},
        {0,1,2},
        {5,6,2},
        {1,5,2},
        {2,6,3}
    };
    static uint8_t const edge_count = sizeof(edges)/sizeof(edges[0]);

    using Comp = cct::img::Component<uint8_t>;
    using Leaf = cct::img::Leaf;
    cct::Tree<Comp, Leaf> tree;

    Comp * edge_comps[edge_count];
    memset(edge_comps, 0, edge_count);

    auto const root_count =
        cct::buildTree<cct::AlphaTreeBuilder>(tree, leaf_count, edges, edge_count, edge_comps);

    BOOST_CHECK_EQUAL(tree.leafCount(), leaf_count);
    BOOST_CHECK_EQUAL(tree.rootCount(), root_count);
    BOOST_CHECK_EQUAL(root_count, 1u);

    check_alpha(tree);

    auto root = tree.root();
    for(size_t i = 0; i < edge_count; ++i)
    {
        BOOST_CHECK(edge_comps[i]);
        BOOST_CHECK(edge_comps[i]->isSubOf(root));
    }
}

BOOST_AUTO_TEST_CASE( struct_tree_redundant )
{
    static uint8_t const leaf_count = 8;
    static cct::Edge<uint8_t, uint8_t> const edges[] = {
        {0,1,0},
        {0,4,0},
        {6,7,0},
        {3,7,0},
        {4,5,0},
        {1,2,0},
        {0,1,0},
        {5,6,0},
        {1,5,0},
        {2,6,0}
    };
    static uint8_t const edge_count = sizeof(edges)/sizeof(edges[0]);

    using Component = cct::img::Component<uint8_t>;
    cct::Tree<Component, cct::img::Leaf> tree;

    Component * edge_comps[edge_count];
    memset(edge_comps, 0, edge_count);

    auto const root_count =
        cct::buildTree<cct::AlphaTreeBuilder>(tree, leaf_count, edges, edge_count, edge_comps);

    BOOST_CHECK_EQUAL(tree.leafCount(), leaf_count);
    BOOST_CHECK_EQUAL(tree.rootCount(), root_count);
    BOOST_CHECK_EQUAL(root_count, 1u);

    check_alpha(tree);

    auto root = tree.root();
    for(size_t i = 0; i < edge_count; ++i)
    {
        BOOST_CHECK(edge_comps[i]);
        BOOST_CHECK(edge_comps[i]->isSubOf(root));
    }
}

BOOST_AUTO_TEST_CASE( struct_altitude_tree )
{
    static uint8_t const leaf_count = 8;
    static cct::Edge<uint8_t, uint8_t> const edges[] = {
        {0,1,0},
        {0,4,0},
        {6,7,0},
        {3,7,1},
        {4,5,1},
        {1,2,2},
        {0,1,2},
        {5,6,2},
        {1,5,2},
        {2,6,3}
    };
    static uint8_t const edge_count = sizeof(edges)/sizeof(edges[0]);

    using Component = cct::img::Component<uint8_t>;
    cct::Tree<Component, cct::img::Leaf> tree;

    Component * edge_comps[edge_count];
    memset(edge_comps, 0, edge_count);

    auto const root_count =
        cct::buildTree<cct::AltitudeTreeBuilder>(tree, leaf_count, edges, edge_count, edge_comps);

    BOOST_CHECK_EQUAL(tree.leafCount(), leaf_count);
    BOOST_CHECK_EQUAL(tree.compCount(), leaf_count - 1u);
    BOOST_CHECK_EQUAL(tree.rootCount(), root_count);
    BOOST_CHECK_EQUAL(root_count, 1u);

    check_tree(tree);

    auto root = tree.root();
    for(size_t i = 0; i < edge_count; ++i)
    {
        BOOST_CHECK(edge_comps[i]);
        BOOST_CHECK(edge_comps[i]->isSubOf(root));
    }
}
