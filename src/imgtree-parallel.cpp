#define BOOST_ENABLE_ASSERT_HANDLER

#include "cct/image_tree.h"

#include "utils/abs_diff.h"

#include <chrono>
#include <fstream>
#include <iostream>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/chrono.hpp>

#include <argtable2.h>

#include <opencv2/highgui/highgui.hpp>

#include <tbb/task_scheduler_init.h>
#include <tbb/task_group.h>

// Command line arguments

struct arg_lit * child_list = nullptr;
struct arg_int * measurements = nullptr;
struct arg_int * tile_width = nullptr;
struct arg_int * tile_height = nullptr;
struct arg_int * parallel_depth = nullptr;
struct arg_lit * parallel_nomerge = nullptr;

template<typename Alpha, typename WeightFunctor>
void process(
    int id, char const * filename, cv::Mat const & image
)
{
    typedef cv::Size_<uint16_t> Size;
    typedef cct::image::Component<Alpha> Component;
    typedef cct::image::Leaf Leaf;
    typedef cct::Tree<Component, Leaf> Tree;
    typedef cct::Builder<Tree> Builder;
    typedef typename Tree::size_type size_type;

    Size const size = image.size();
    Size const tile(tile_width->ival[0], tile_height->ival[0]);

    boost::accumulators::accumulator_set<double, boost::accumulators::features<
        boost::accumulators::tag::min,
        boost::accumulators::tag::max,
        boost::accumulators::tag::mean
        > > time_statistics;

    Tree tree(cct::image::vertexCount<size_type>(image.size()));
    Builder builder(&tree);

    for(int i = 0; i < measurements->ival[0]; ++i)
    {
        builder.reset();
        tree.reset();

        auto t1 = boost::chrono::high_resolution_clock::now();
        cct::image::buildAlphaTree(size, tile, builder, WeightFunctor(image), parallel_depth->ival[0]);
        auto t2 = boost::chrono::high_resolution_clock::now();

        time_statistics(boost::chrono::duration_cast<boost::chrono::duration<double>>(t2-t1).count());
    }

    std::cout
        << id << ',' << filename << ',' << image.cols << ',' << image.rows << ','
        << cct::image::vertexCount(image.size()) << ',' << cct::image::edgeCount(image.size()) << ','
        << tree.componentCount() << ',' << std::flush
        << tree.calculateHeight() << ','
        << tree.rootCount() << ',' << std::flush
        << tree.countDegenerateComponents() << ','
        << boost::accumulators::min(time_statistics) << ','
        << boost::accumulators::max(time_statistics) << ','
        << boost::accumulators::mean(time_statistics)
        << std::endl;
}

#include "imgtree.h"
