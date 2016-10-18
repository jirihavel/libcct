#include "cct/array_tree.h"
#include "cct/root_finder.h"

#include <cstdint>
#include <cstdlib>

#include <iostream>

uint8_t const leaf_count = 8;

uint8_t const edges[][3] = {
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

uint8_t const edge_count = sizeof(edges)/sizeof(edges[0]);

int main(int, char **)
{
    namespace arr = cct::array;

    typedef arr::Tree<uint8_t, uint8_t, uint8_t> Tree;
    typedef cct::PackedRootFinder<uint8_t, uint8_t> Finder;

    Tree tree;
    tree.init(leaf_count);

    Finder finder(leaf_count, cct::LeafIndexTag());

    for(int i = 0; i < edge_count; ++i)
    {
        uint8_t const a = finder.find_update(edges[i][0]);
        uint8_t const b = finder.find_update(edges[i][1]);
        if(a != b)
        {
            finder.merge_set(a, b,
                tree.altitudeMerge(
                    finder.data(a), finder.data(b),
                    edges[i][2])
                );
        }
    }

    std::cout
        << "leaves=" << +tree.leaf_count << std::endl
        << "nodes=" << +tree.node_count << std::endl
        << "capacity=" << +tree.node_capacity << std::endl
        << "invalid=" << +tree.invalid_count << std::endl
        << "height=" << +tree.height() << std::endl;
    for(int i = 0; i < tree.node_capacity; ++i)
        std::cout << +tree.parents[i] << ' ';
    std::cout << std::endl;
    for(int i = 0; i < tree.node_capacity; ++i)
        std::cout << +tree.leaf_levels[i] << ' ';
    std::cout << std::endl;
    for(size_t i = 0; i < tree.node_capacity; ++i)
        std::cout << +tree.children[i] << ' ';
    std::cout << std::endl;

    tree.altitudeCanonicalize();

    std::cout
        << "leaves=" << +tree.leaf_count << std::endl
        << "nodes=" << +tree.node_count << std::endl
        << "capacity=" << +tree.node_capacity << std::endl
        << "invalid=" << +tree.invalid_count << std::endl
        << "height=" << +tree.height() << std::endl;
    for(int i = 0; i < tree.node_capacity; ++i)
        std::cout << +tree.parents[i] << ' ';
    std::cout << std::endl;
    for(int i = 0; i < tree.node_capacity; ++i)
        std::cout << +tree.leaf_levels[i] << ' ';
    std::cout << std::endl;
    
    tree.compress();
    memset(tree.parents + tree.node_count, 255, tree.node_capacity - tree.node_count);
    memset(tree.comp_levels + tree.node_count - tree.leaf_count, 255, tree.node_capacity - tree.node_count);

    std::cout
        << "leaves=" << +tree.leaf_count << std::endl
        << "nodes=" << +tree.node_count << std::endl
        << "capacity=" << +tree.node_capacity << std::endl
        << "invalid=" << +tree.invalid_count << std::endl
        << "height=" << +tree.height() << std::endl;
    for(int i = 0; i < tree.node_capacity; ++i)
        std::cout << +tree.parents[i] << ' ';
    std::cout << std::endl;
    for(int i = 0; i < tree.node_capacity; ++i)
        std::cout << +tree.leaf_levels[i] << ' ';
    std::cout << std::endl;
/*    for(size_t i = 0; i < leaf_count; ++i)
        std::cout << +parents[i] << ' ';
    std::cout << "| ";
    for(size_t i = leaf_count; i < sizeof(parents); ++i)
        std::cout << +parents[i] << ' ';
    std::cout << std::endl;
    for(int i = 0; i < capacity; ++i)
        std::cout << +levels[i] << ' ';
    std::cout << std::endl;*/

    tree.buildChildren();

    for(size_t i = 0; i < (tree.node_count-tree.leaf_count+3u); ++i)
        std::cout << +tree.child_count[i] << ' ';
    std::cout << std::endl;
    for(size_t i = 0; i < tree.node_count; ++i)
        std::cout << +tree.children[i] << ' ';
    std::cout << std::endl;

    return EXIT_SUCCESS;
}
