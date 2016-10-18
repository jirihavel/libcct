#include "cct/array_tree.h"
#include "cct/builder.h"

#include <iostream>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE( base_tree )
{
    static uint8_t const leaf_count = 8;
    static cct::Edge<uint8_t, uint8_t> const edges[] = {
        {0,1,0},
        /*{2,3,0},
        {0,2,0},
        {4,5,0},
        {6,7,0},
        {4,6,0},
        {0,4,0},//
        {2,3,0},*/
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

    cct::array::Tree<uint8_t, uint8_t, uint8_t> tree;

    uint8_t edge_comps[edge_count];
    memset(edge_comps, 0xff, edge_count);

    auto const root_count =
        cct::buildTree<cct::AlphaTreeBuilder>(tree, leaf_count, edges, edge_count, edge_comps);

/*
    std::cout
        << "leaves=" << +tree.leaf_count << std::endl
        << "nodes=" << +tree.node_count << std::endl
        << "capacity=" << +tree.node_capacity << std::endl
        << "invalid=" << +tree.invalid_count << std::endl
        << "height=" << +tree.height() << std::endl;
    for(int i = 0; i < tree.node_capacity; ++i)
        std::cout << +tree.parents[i] << ' ';
    std::cout << std::endl;
//    for(int i = 0; i < tree.node_capacity; ++i)
//        std::cout << +tree.leaf_levels[i] << ' ';
//    std::cout << std::endl;
    for(size_t i = 0; i < (tree.node_capacity-tree.leaf_count+2); ++i)
        std::cout << +tree.child_count[i] << ' ';
    std::cout << std::endl;
    for(size_t i = 0; i < tree.node_capacity; ++i)
        std::cout << +tree.children[i] << ' ';
    std::cout << std::endl;
*/
    
    BOOST_CHECK(tree.flags() == cct::array::TreeFlags::ALPHA_FLAGS);
    BOOST_CHECK_EQUAL(tree.leafCount(), leaf_count);
    BOOST_CHECK_EQUAL(tree.rootCount(), root_count);
    BOOST_CHECK_EQUAL(root_count, 1);

    for(uint8_t i = 0; i < edge_count; ++i)
        BOOST_CHECK_LT(edge_comps[i], tree.compCount());
}
