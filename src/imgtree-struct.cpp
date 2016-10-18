#define BOOST_ENABLE_ASSERT_HANDLER

#include <cct/image_tree.h>

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
    int id, char const * filename, cv::Mat const & image
)
{
//    typedef cv::Size_<uint16_t> Size;
    typedef cct::img::Component<Alpha> Component;
    typedef cct::img::Leaf Leaf;
    typedef cct::Tree<Component, Leaf> Tree;
//    typedef typename Tree::size_type size_type;

//    Size const size = image.size();
//    Size const tile(tile_width->ival[0], tile_height->ival[0]);

    boost::accumulators::accumulator_set<double, boost::accumulators::features<
        boost::accumulators::tag::min,
        boost::accumulators::tag::max,
        boost::accumulators::tag::mean
        > > time_statistics;

    Tree tree;//(cct::img::vertexCount<size_t>(image.size()));
    //Builder builder(&tree);

    size_t edge_count;
    uint32_t root_count;

    for(int i = 0; i < measurements->ival[0]; ++i)
    {
        //builder.reset();
        tree.reset();

        auto t1 = boost::chrono::high_resolution_clock::now();
//        cct::img::buildTree<cct::(size, tile, builder, EdgeWeightFunctor(image));
        std::tie(edge_count, root_count)
            = cct::img::buildTree<cct::AlphaTreeBuilder, EdgeWeightFunctor>(image, tree, nullptr);
        auto t2 = boost::chrono::high_resolution_clock::now();

        time_statistics(boost::chrono::duration_cast<boost::chrono::duration<double>>(t2-t1).count());
    }

    std::cout
        << id << ',' << filename << ',' << image.cols << ',' << image.rows << ','
        << cct::img::vertexCount(image.size()) << ',' << edge_count << ','
        << tree.compCount() << ','
        << tree.calculateHeight() << ','
        << tree.rootCount() << ','
        << tree.countDegenerateComponents() << ','
        << boost::accumulators::min(time_statistics) << ','
        << boost::accumulators::max(time_statistics) << ','
        << boost::accumulators::mean(time_statistics)
        << std::endl;
}        

#include "imgtree.h"
