#define BOOST_ENABLE_ASSERT_HANDLER

#include <cct/array_tree.h>
#include <cct/image.h>

#include <chrono>
#include <iostream>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/chrono.hpp>

#include <argtable2.h>

#include <opencv2/core/core.hpp>

// Command line arguments

struct arg_lit * child_list = nullptr;
struct arg_int * measurements = nullptr;
struct arg_int * tile_width = nullptr;
struct arg_int * tile_height = nullptr;
struct arg_int * parallel_depth = nullptr;
struct arg_lit * parallel_nomerge = nullptr;

template<typename Alpha, template<typename, int> class EdgeWeightFunctor>
void process(
    int id,
    char const * filename, cv::Mat const & image
)
{
    typedef cct::array::Tree<uint32_t, uint32_t, Alpha> Tree;

    boost::accumulators::accumulator_set<double, boost::accumulators::features<
        boost::accumulators::tag::min,
        boost::accumulators::tag::max,
        boost::accumulators::tag::mean
        > > time_statistics;

//    cv::Size_<uint16_t> const size = image.size();
//    cv::Size_<uint16_t> const tile(tile_width->ival[0], tile_height->ival[0]);

    Tree tree;

    size_t edge_count;
    uint32_t root_count;

    size_t h = 0;

    for(int i = 0; i < measurements->ival[0]; ++i)
    {
        auto t1 = boost::chrono::high_resolution_clock::now();
        std::tie(edge_count, root_count)
            = cct::img::buildTree<cct::AlphaTreeBuilder, EdgeWeightFunctor>(image, tree, nullptr);
        auto t2 = boost::chrono::high_resolution_clock::now();

        h += tree.height();

        time_statistics(boost::chrono::duration_cast<boost::chrono::duration<double>>(t2-t1).count());
    }

    auto const height = tree.height();

    std::cout
        << id << ',' << filename << ',' << image.cols << ',' << image.rows << ','
        << cct::img::vertexCount(image.size()) << ',' << edge_count << ','
        << (tree.node_count - tree.leaf_count) << ','
        << height << ','
        << root_count << ','
        << (h/measurements->ival[0]) << ','
        << boost::accumulators::min(time_statistics) << ','
        << boost::accumulators::max(time_statistics) << ','
        << boost::accumulators::mean(time_statistics)
        << std::endl;
}        

#include "imgtree.h"
