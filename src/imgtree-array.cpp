#define BOOST_ENABLE_ASSERT_HANDLER

#include "cct/array_builder.h"

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

// Command line arguments

struct arg_lit * child_list = nullptr;
struct arg_int * measurements = nullptr;
struct arg_int * tile_width = nullptr;
struct arg_int * tile_height = nullptr;
struct arg_int * parallel_depth = nullptr;
struct arg_lit * parallel_nomerge = nullptr;

template<typename Alpha, typename WeightFunctor>
void process(
    int id,
    char const * filename, cv::Mat const & image
)
{
    //typedef cct::image::Edge<uint16_t, Alpha> Edge;

    boost::accumulators::accumulator_set<double, boost::accumulators::features<
        boost::accumulators::tag::min,
        boost::accumulators::tag::max,
        boost::accumulators::tag::mean
        > > time_statistics;

    size_t const vertex_count = cct::image::vertexCount(image.size());
    //size_t const edge_count = cct::image::edgeCount(image.size());

    cv::Size_<uint16_t> const size = image.size();
    cv::Size_<uint16_t> const tile(tile_width->ival[0], tile_height->ival[0]);

    size_t component_count;

    for(int i = 0; i < measurements->ival[0]; ++i)
    {
        auto t1 = boost::chrono::high_resolution_clock::now();

        array_tree<uint32_t, uint32_t, Alpha> t;//(vertex_count);
        t.leaf_count = vertex_count;
        t.node_count = vertex_count;
        t.node_capacity = 2*vertex_count-1;
        t.invalid_count = 0;
        t.parents = new uint32_t[t.node_capacity];
        t.leaf_levels = new Alpha[t.node_capacity];
        t.comp_levels = t.leaf_levels + t.leaf_count;
        t.child_count = new uint32_t[(t.node_capacity-t.leaf_count)+3];
        t.children = new uint32_t[(t.node_capacity-t.leaf_count)*2];
        t.reset();

        cct::image::buildAlphaTree(size, tile, t, WeightFunctor(image));
        if(child_list->count)
            t.build_children();
 
        auto t2 = boost::chrono::high_resolution_clock::now();

        component_count = t.componentCount();

        delete [] t.parents;
        delete [] t.leaf_levels;
        delete [] t.child_count;
        delete [] t.children;

        time_statistics(boost::chrono::duration_cast<boost::chrono::duration<double>>(t2-t1).count());
    }

    std::cout
        << id << ',' << filename << ',' << image.cols << ',' << image.rows << ','
        << cct::image::vertexCount(image.size()) << ',' << cct::image::edgeCount(image.size()) << ','
        << component_count << ','
        << 0 << ','
        << 0 << ','
        << 0 << ','
//        << tree.calcHeight() << ','
//        << tree.rootCount() << ','
//        << tree.countDegenerateComponents() << ','
        << boost::accumulators::min(time_statistics) << ','
        << boost::accumulators::max(time_statistics) << ','
        << boost::accumulators::mean(time_statistics)
        << std::endl;
}        

#include "imgtree.h"
