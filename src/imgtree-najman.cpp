#define BOOST_ENABLE_ASSERT_HANDLER

#include "cct/image_tree.h"
#include "cct/array_tree.h"

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
    int id, char const * filename,
    cv::Mat const & image
)
{
    typedef cct::image::Edge<uint16_t, Alpha> Edge;

    boost::accumulators::accumulator_set<double, boost::accumulators::features<
        boost::accumulators::tag::min,
        boost::accumulators::tag::max,
        boost::accumulators::tag::mean
        > > time_statistics;

    size_t const vertex_count = cct::image::vertexCount(image.size());
    size_t const edge_count = cct::image::edgeCount(image.size());

    cv::Size_<uint32_t> const size = image.size();
    cv::Size_<uint16_t> const tile(tile_width->ival[0], tile_height->ival[0]);

    size_t component_count;

    for(int i = 0; i < measurements->ival[0]; ++i)
    {
        auto t1 = boost::chrono::high_resolution_clock::now();

        Edge * edges = new Edge[edge_count];
        getSortedImageEdges(
            cv::Rect_<uint16_t>(0,0,image.cols,image.rows),
            tile, edges, WeightFunctor(image)
        );

        cct::PackedRootFinder<uint32_t, uint32_t> root(vertex_count, cct::LeafIndexTag());

        array_tree<uint32_t, uint32_t, uint8_t> t;//(vertex_count);
        t.leaf_count = vertex_count;
        t.node_count = vertex_count;
        t.node_capacity = 2*vertex_count-1;
        t.invalid_count = 0;
        t.parents = new uint32_t[t.node_capacity];
        t.leaf_levels = new uint8_t[t.node_capacity];
        t.comp_levels = t.leaf_levels + t.leaf_count;
        t.children = new uint32_t[(t.node_capacity-t.leaf_count)*2];
        t.reset();

        uint32_t * merges = new uint32_t[t.leaf_count];

        for(uint32_t i = 0; i < edge_count; ++i)
        {
            uint32_t const ha = root.find_update(cct::image::pointId<uint32_t>(edges[i].points[0], size));
            uint32_t const hb = root.find_update(cct::image::pointId<uint32_t>(edges[i].points[1], size));
            if(ha != hb)
            {
                root.merge_set(ha, hb,
                    t.bpt_merge(root.data(ha), root.data(hb), edges[i].weight)
                );
            }
        }
        t.bpt_postprocess();
        t.compress(merges);
 
        auto t2 = boost::chrono::high_resolution_clock::now();

        component_count = t.componentCount();

        delete [] edges;
        delete [] t.parents;
        delete [] t.leaf_levels;
        delete [] t.children;
        delete [] merges;

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
