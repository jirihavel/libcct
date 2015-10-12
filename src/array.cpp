#define WITH_EIGEN
#include <iostream>
#include "cct/array_tree.h"
#include "cct/image.h"

#include "utils/abs_diff.h"

#include <cstdint>
#include <cstdlib>

#include <chrono>
#include <iostream>

#include <Eigen/Dense>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/contrib/contrib.hpp>

int const KEY_UP = 2490368;
int const KEY_DOWN = 2621440;
int const KEY_LEFT = 2424832;
int const KEY_RIGHT = 2555904;

namespace arr = cct::array;
typedef arr::Tree<uint32_t, uint32_t, float> Tree;

cv::Mat_<cv::Vec3b> image;
cv::Mat_<uint8_t> mask;
cv::Mat_<cv::Vec3b> bgr_mask;

//typedef Eigen::Vector
typedef Eigen::Matrix3f ColorMat;

Tree tree;
Tree subtree;
std::vector<float> tree_levels;
std::unique_ptr<size_t[]> edge_counts;
std::unique_ptr<uint32_t[]> heights;
std::unique_ptr<uint32_t[]> node_areas;
std::unique_ptr<uint32_t[]> node_mask_areas;
std::unique_ptr<cv::Point_<float>[]> node_centroids;
std::unique_ptr<cv::Vec3b[]> node_color;
std::unique_ptr<ColorMat[]> node_bgr;
std::unique_ptr<ColorMat[]> node_lab;

bool show_leaves = false;

int g_alpha = 0;
int g_beta = 0;

uint32_t g_selected_vertex = UINT32_MAX;
/*
template<typename M, typename I>
void moments(M & matrix, I node, I parent, uint32_t * areas)
{
    // 
    auto const node_area   = areas[node];
    auto const parent_area = areas[parent];
    // matrix(.,0) is mean
    // matrix(.,1) is M_2 until finished, then variance
    for(int j = 0; j < matrix.rows(); ++j)
    {
        float const delta = matrix[i](j,0)-node_bgr[p](j,0);
        float const mean_delta = delta*ni*inv_nt;
        node_bgr[p](j,0) += mean_delta;
        float const skew_delta = mean_delta*delta*np;
        node_bgr[p](j,1) += node_bgr[i](j,1) + skew_delta;
        node_bgr[p](j,2) += node_bgr[i](j,2) + skew_delta*delta*(float(np)-ni)*inv_nt;
        // do roots
        if(i >= tree.leaf_count)
        {
            node_bgr[i](j,1) = sqrt(node_bgr[i](j,1)/(node_areas[i]-1));
            node_bgr[i](j,2) = cbrt(node_bgr[i](j,2));
        }
        // calculate rgb8 color
        node_color[i](j) = 255*node_bgr[i](j,0);
    }
}
*/

void filter()
{
    subtree = tree;

    float const threshold = static_cast<float>(g_beta)/(UINT16_MAX+1);

    auto const root = subtree.node_count-1;
    auto n = root;
    while(n-- > subtree.leaf_count)
    {
        if((subtree.parents[n] == root-subtree.leaf_count) &&
            (static_cast<float>(node_mask_areas[n])/node_areas[n] < threshold))
        {
            // remove node
            auto const c = n-subtree.leaf_count;
            auto const p = subtree.parents[n];
            // reconnect children
            for(auto i = subtree.child_count[c]; i < subtree.child_count[c+1]; ++i)
            {
                subtree.parents[subtree.children[i]] = p;
            }
            // remove node
            subtree.parents[n] = 0;
            subtree.invalid_count += 1;
        }
    }
}

void redraw()
{
    Tree const & tree = subtree;

    auto const t0 = std::chrono::high_resolution_clock::now();

    float const alpha = tree_levels[g_alpha];

    cv::Mat_<cv::Vec3b> output(image.size());
    cv::Mat_<uint8_t> heatmap(image.size());

    uint32_t const comp_count = tree.node_count-tree.leaf_count;
    uint32_t const cut_end = std::upper_bound(tree.comp_levels, tree.comp_levels+comp_count, alpha) - tree.comp_levels;

    std::unique_ptr<cv::Vec3b[]> pixel_color(new cv::Vec3b[tree.node_count]);
    std::unique_ptr<uint32_t[]> pixel_areas(new uint32_t[tree.node_count]);
    std::unique_ptr<size_t[]> pixel_edges(new size_t[tree.node_count]);

    cv::Point centroid;

    // propagate layer values down to pixels
    if(g_selected_vertex == UINT32_MAX)
    {
        if(show_leaves)
        {
            std::copy(node_color.get(), node_color.get()+tree.node_count, pixel_color.get());
        }
        else
        {
            std::fill_n(pixel_color.get(), tree.leaf_count, cv::Vec3b(0,0,0));
            std::copy(node_color.get()+tree.leaf_count, node_color.get()+tree.node_count, pixel_color.get()+tree.leaf_count);
        }
    }
    else
    {
        std::fill_n(pixel_color.get(), tree.node_count, cv::Vec3b(0,0,0));
        pixel_color[g_selected_vertex] = node_color[g_selected_vertex];
        for(uint32_t n = tree.parents[g_selected_vertex]; (n+tree.leaf_count) < tree.node_count; n = tree.parents[n+tree.leaf_count])
            pixel_color[n+tree.leaf_count] = node_color[n+tree.leaf_count];

        uint32_t n = g_selected_vertex;
        while(tree.parents[n] < cut_end)
            n = tree.parents[n]+tree.leaf_count;
        centroid = node_centroids[n];

        std::cout
            << "area     = " << node_areas[n] << std::endl
            << "maskarea = " << node_mask_areas[n] << std::endl
//            << "centroid = " << centroids[n] << std::endl
            << "bgr : \n" << node_bgr[n] << std::endl
//            << "lab : \n" << node_lab[n] << std::endl
            //<< "mean = " << node_color_mean[n] << std::endl
            //<< "stdev = " << node_color_stdev[n] << std::endl
            //<< "skew  = " << node_color_skew[n] << std::endl;
            ;

        std::vector<float> area_vec;
        std::vector<float> area_cnt;
        std::vector<float> area_mask;
        std::vector<bool> area_col;
        uint32_t i = g_selected_vertex;
        while(tree.parents[i] < (tree.node_count-tree.leaf_count))
        {
            area_vec.push_back(node_areas[i]);
            area_cnt.push_back(edge_counts[i]);
            area_mask.push_back(node_mask_areas[i]);
            area_col.push_back(tree.leaf_levels[i] <= alpha);
            i = tree.parents[i] + tree.leaf_count;
        }

        cv::Point last[3];
        for(int i = 0; i < 3; ++i) last[i] = cv::Point(0,600);
        cv::Mat_<cv::Vec3b> graph(cv::Size(800,600));
        for(size_t i = 0; i < area_vec.size(); ++i)
        {
            cv::Point p;
            p = cv::Point(i*800.0/area_vec.size(), 600 - area_vec[i]/area_vec.back()*600);
            cv::line(graph, last[0], p, area_col[i] ? cv::Scalar(255,0,0) : cv::Scalar(255,0,255));
            last[0] = p;
            p = cv::Point(i*800.0/area_vec.size(), 600 - area_cnt[i]/area_cnt.back()*600);
            cv::line(graph, last[1], p, area_col[i] ? cv::Scalar(0,255,0) : cv::Scalar(0,255,255));
            last[1] = p;
            p = cv::Point(i*800.0/area_vec.size(), 600 - area_mask[i]/area_mask.back()*600);
            cv::line(graph, last[2], p, cv::Scalar(0,0,255));
            last[2] = p;
        }
        cv::imshow("graph", graph);
    }

    std::copy_n(node_areas.get(), tree.node_count, pixel_areas.get());
    std::copy_n(edge_counts.get(), tree.node_count, pixel_edges.get());

    uint32_t i = tree.leaf_count + cut_end;
    while(i-- > 0)
    {
        uint32_t const p = tree.parents[i] + tree.leaf_count;
        if(tree.parents[i] < cut_end)
        {
            pixel_color[i] = pixel_color[p];
            pixel_areas[i] = pixel_areas[p];
            pixel_edges[i] = pixel_edges[p];
        }
    }
    
/*
//    uint32_t maximum = *std::max_element(ar.get(), ar.get()+tree.leaf_count);
//    uint32_t maximum = image.rows*image.cols;
    auto const maximum = cct::image::edgeCount<size_t>(image.size());

    if(g_beta <= 0) g_beta = 1;
    //cv::Mat_<uint32_t> arimg(image.size());

//    cct::img::leavesToImage(heatmap, [maximum,&pixel_areas](uint32_t i){ return pixel_areas[i]*255/maximum; });
    cct::img::leavesToImage(heatmap, [&](uint32_t i)
            {
                size_t const h = static_cast<size_t>(pixel_edges[i])*255/pixel_areas[i]/g_beta;
                return h > UINT8_MAX ? UINT8_MAX : h;
            });

    cv::applyColorMap(heatmap, output, cv::COLORMAP_JET);
*/
    cct::img::leavesToColors(output, [&pixel_color](uint32_t i){ return pixel_color[i]; });

    auto const t1 = std::chrono::high_resolution_clock::now();
    std::cout << "draw " << std::chrono::duration_cast<std::chrono::milliseconds>(t1-t0).count() << std::endl;

    if(g_selected_vertex != UINT32_MAX)
    {
        cv::circle(output, centroid, 2, cv::Scalar(0,0,255));
    }

    auto const size = output.size();
    cv::Mat_<cv::Vec3b> tiles(2*size.height, 2*size.width);
    output.copyTo(tiles(cv::Rect(cv::Point(0,0), size)));
    image.copyTo(tiles(cv::Rect(cv::Point(0,size.height), size)));
    bgr_mask.copyTo(tiles(cv::Rect(cv::Point(size.width,size.height), size)));

//    cv::Mat_<cv::Vec3b> grf = tiles(cv::Rect(cv::Point(size.width,0), size));
//    cv::line(grf, cv::Point(0,0), cv::Point(size.width-1, size.height-1), cv::Scalar(0,255,0));

    cv::imshow("image", tiles);
}

void on_refilter(int, void *)
{
    filter();
    redraw();
}

void on_redraw(int, void *)
{
    redraw();
}

void on_mouse(int event, int x, int y, int, void*)
{
    if(event == cv::EVENT_LBUTTONDOWN)
    {
        g_selected_vertex = cct::img::pointId<uint32_t>(cv::Point(x,y), image.size());
        redraw();
    }
    else if(event == cv::EVENT_RBUTTONDOWN)
    {
        g_selected_vertex = UINT32_MAX;
        redraw();
    }
}

int main(int argc, char * argv[])
{
    image = cv::imread(argv[1]);
    mask = argc>2 ? cv::imread(argv[2],CV_LOAD_IMAGE_GRAYSCALE) : cv::Mat_<uint8_t>::zeros(image.size());
    cv::Mat_<cv::Vec3f> bgr = (1.0f/255)*image;
    cv::Mat_<cv::Vec3f> lab;
    cv::cvtColor(bgr, lab, CV_BGR2Lab);
    cv::cvtColor(mask, bgr_mask, CV_GRAY2BGR);

    cv::Mat img(bgr);

    auto const t0 = std::chrono::high_resolution_clock::now();
    
    tree.init(image.rows*image.cols, cct::array::TreeFlags::ALL);

    size_t edge_count = cct::img::edgeCount<size_t>(image.size());
    std::unique_ptr<uint32_t[]> edge_comps(new uint32_t[edge_count]);

//    cct::img::buildTree<cct::AlphaTreeBuilder, utils::LInf, uint16_t>(bgr, tree, edge_comps.get());
    cct::img::buildTree<cct::AlphaTreeBuilder, utils::LInf, uint16_t>(img, tree, edge_comps.get());

//    cct::image::buildTree<cct::AlphaTreeBuilder, utils::LInf, uint16_t>(img, tree, edge_comps.get());
//    cct::image::buildTree<cct::AltitudeTreeBuilder, utils::LInf, uint16_t>(img, tree, edge_comps.get());

    std::cout
        << "\tleaves " << tree.leaf_count << std::endl
        << "\tnodes  " << tree.node_count << std::endl
        << "\tcapacity " << tree.node_capacity << std::endl;

    auto const t1 = std::chrono::high_resolution_clock::now();

    // initial values of edge counts for each tree node
    edge_counts.reset(new size_t[tree.node_count+1]);
    std::fill_n(edge_counts.get(), tree.leaf_count, 0);
    tree.componentHistogram(edge_counts.get() + tree.leaf_count, edge_comps.get(), edge_count);

    // propagate edge counts up (leaves don't have edges)
    for(uint32_t i = tree.leaf_count; i < tree.node_count; ++i)
    {
        auto const p = std::min(tree.parents[i]+tree.leaf_count, tree.node_count);
        edge_counts[p] += edge_counts[i];
    }

    heights.reset(new uint32_t[tree.node_count-tree.leaf_count+1]);
    node_areas.reset(new uint32_t[tree.node_count+1]);
    node_mask_areas.reset(new uint32_t[tree.node_count+1]);
    node_centroids.reset(new cv::Point_<float>[tree.node_count+1]);
    node_bgr.reset(new ColorMat[tree.node_count+1]);
    node_lab.reset(new ColorMat[tree.node_count+1]);

    uint32_t height = tree.height(heights.get());

    // other node attributes

    std::fill_n(node_bgr.get(), tree.node_count+1, ColorMat::Zero());
    std::fill_n(node_lab.get(), tree.node_count+1, ColorMat::Zero());

    cct::img::forEachVertex(image.size(), 
            [&bgr,&lab](cv::Point const & p, uint32_t i)
            {
                node_areas[i] = 1;
                node_mask_areas[i] = mask(p) > 0;
                // put centroids to pixel centers
                node_centroids[i] = cv::Point_<float>(p.x+0.5f, p.y+0.5f);
                // each leaf has its pixel color
                for(int j = 0; j < 3; ++j)
                {
                    node_bgr[i](j,0) = bgr(p)(j);
                    node_lab[i](j,0) = lab(p)(j);
                }
            }
        );

    std::fill(node_areas.get()+tree.leaf_count, node_areas.get()+tree.node_count+1, 0);
    std::fill(node_mask_areas.get()+tree.leaf_count, node_mask_areas.get()+tree.node_count+1, 0);
    std::fill(node_centroids.get()+tree.leaf_count, node_centroids.get()+tree.node_count+1, cv::Vec2f(0,0));
    // initial values for nodes

    node_color.reset(new cv::Vec3b[tree.node_count+1]);

    // propagate up
    for(uint32_t i = 0; i < tree.node_count; ++i)
    {
        auto const p = std::min(tree.parents[i]+tree.leaf_count, tree.node_count);
        // propagate areas
        auto const ni = node_areas[i];
        auto const np = node_areas[p];
        node_areas[p] += ni;
        // propagate mask area
        node_mask_areas[p] += node_mask_areas[i];
        // propagate centroids
        node_centroids[p] += node_centroids[i];
        node_centroids[i] *= 1.0f/node_areas[i];// finish centroid average
        // propagate colors
        float const inv_nt = 1.0f/node_areas[p];//1/total parent area
        for(int j = 0; j < 3; ++j)
        {
            // mat(_,0) is average
            // mat(_,1) is M2 and stddev after finishing
            // mat(_,2) is M3 and skewness
            {// bgr
                // Chan algorithm
                float const delta_over_area = (node_bgr[i](j,0)-node_bgr[p](j,0))*inv_nt;
                float const mean_delta = ni*delta_over_area;
                node_bgr[p](j,0) += mean_delta;
                float const skew_delta = np*mean_delta*delta_over_area;
                node_bgr[p](j,1) += node_bgr[i](j,1) + skew_delta;
                node_bgr[p](j,2) += node_bgr[i](j,2) + (float(np)-ni)*skew_delta*delta_over_area
                    + 3*(np*node_bgr[i](j,1) - ni*node_bgr[p](j,1))*delta_over_area;
                // finish M2 -> stddev, M3 -> skewness
                if(i >= tree.leaf_count)
                {
                    node_bgr[i](j,2) = sqrt(node_areas[i])*node_bgr[i](j,2)/sqrt(node_bgr[i](j,1)*node_bgr[i](j,1)*node_bgr[i](j,1));
                    node_bgr[i](j,1) = sqrt(node_bgr[i](j,1)/node_areas[i]);
                }
                // calculate rgb8 color
                node_color[i](j) = 255*node_bgr[i](j,0);
            }{// lab
                float const delta = node_lab[i](j,0)-node_lab[p](j,0);
                float const mean_delta = delta*ni*inv_nt;
                node_lab[p](j,0) += mean_delta;
                float const skew_delta = delta*np*mean_delta;
                node_lab[p](j,1) += node_lab[i](j,1) + skew_delta;
                node_lab[p](j,2) += node_lab[i](j,2) + skew_delta*delta*(float(np)-ni)*inv_nt
                    + 3*delta*(np*node_lab[i](j,1) - ni*node_lab[p](j,1))*inv_nt;
                // finish
                if(i >= tree.leaf_count)
                {
                    node_lab[i](j,2) = sqrt(node_areas[i])*node_lab[i](j,2)/sqrt(node_lab[i](j,1)*node_lab[i](j,1)*node_lab[i](j,1));
                    node_lab[i](j,1) = sqrt(node_lab[i](j,1)/node_areas[i]);
                }
            }
        }
    }
    // pseudo-root
/*    {
        auto const n = tree.node_count;
        node_centroids[n] *= 1.0f/node_areas[n];
        for(int j = 0; j < 3; ++j)
        {
            node_color[n](j) = 255*node_bgr[n](j,0);
            node_bgr[n](j,1) = sqrt(node_bgr[n](j,1)/(node_areas[n]-1));
            node_bgr[n](j,2) = cbrt(node_bgr[n](j,2));
            node_lab[n](j,1) = sqrt(node_lab[n](j,1)/(node_areas[n]-1));
            node_lab[n](j,2) = cbrt(node_lab[n](j,2));
        }
    }*/

    auto const t2 = std::chrono::high_resolution_clock::now();

    filter();

    //auto const t3 = std::chrono::high_resolution_clock::now();

    float level = 0.0f;
    tree_levels.push_back(level);
    for(uint32_t i = 0; i < tree.node_count; ++i)
    {
        if(level < tree.leaf_levels[i])
        {
            level = tree.leaf_levels[i];
            tree_levels.push_back(level);
        }
    }

    std::cout
        << "tree " << std::chrono::duration_cast<std::chrono::milliseconds>(t1-t0).count() << std::endl
        << "attr " << std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count() << std::endl
        << "height " << height << std::endl
        << "levels " << tree_levels.size() << std::endl
        ;

    cv::namedWindow("image");
    cv::setMouseCallback("image", on_mouse, nullptr);
    cv::createTrackbar("alpha", "image", &g_alpha, tree_levels.size()-1, on_redraw, nullptr);
    cv::createTrackbar("beta" , "image", &g_beta , UINT16_MAX , on_refilter, nullptr);
    redraw();
    
    int key;
    while((key = cv::waitKey(0)) != 27)
    {
        //std::cout << "key " << key << std::endl;
        switch(key)
        {
        case 'l' :
            show_leaves = !show_leaves;
            redraw();
            break;
        case KEY_UP :
            {
                int t = cv::getTrackbarPos("beta", "image");
//                if(t > 0)
                    cv::setTrackbarPos("beta", "image", t+1);
            }
            break;
        case KEY_LEFT :
            {
                int t = cv::getTrackbarPos("alpha", "image");
                if(t > 0)
                {
                    if(g_selected_vertex != UINT32_MAX)
                    {
                        uint32_t n = g_selected_vertex;
                        float alpha = tree.leaf_levels[n];
                        uint32_t p = tree.parents[n]+tree.leaf_count;
                        while((p < tree.node_count) && (tree.leaf_levels[p] < tree_levels[t]))
                        {
                            n = p;
                            alpha = tree.leaf_levels[n];
                            p = tree.parents[n]+tree.leaf_count;
                        }
                        t = std::lower_bound(tree_levels.begin(), tree_levels.end(), alpha)-tree_levels.begin();
                    }
                    else
                        t -= 1;
                }
                cv::setTrackbarPos("alpha", "image", t);
            }
            break;
        case KEY_RIGHT :
            {
                int t = cv::getTrackbarPos("alpha", "image");
                if((size_t)t < (tree_levels.size()-1))
                {
                    if(g_selected_vertex != UINT32_MAX)
                    {
                        uint32_t n = g_selected_vertex;
                        float alpha = tree.leaf_levels[n];
                        uint32_t p = tree.parents[n]+tree.leaf_count;
                        while((p < tree.node_count) && (alpha <= tree_levels[t]))
                        {
                            n = p;
                            alpha = tree.leaf_levels[n];
                            p = tree.parents[n]+tree.leaf_count;
                        }
                        t = std::lower_bound(tree_levels.begin(), tree_levels.end(), alpha)-tree_levels.begin();
                    }
                    else
                        t += 1;
                }
                cv::setTrackbarPos("alpha", "image", t);
            }
            break;
        }
            
    }

    return EXIT_SUCCESS;
}
