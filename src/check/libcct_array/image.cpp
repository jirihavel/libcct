#include "cct/array_tree.h"
#include "cct/builder.h"
#include "cct/image.h"

#include <utils/abs_diff.h>

#include <iostream>

#include <boost/test/unit_test.hpp>

#include <opencv2/highgui/highgui.hpp>

template<typename C, int N>
using Metric = utils::metric::Lp<SIZE_MAX, C, N>;

BOOST_AUTO_TEST_CASE( image_tree )
{
    cv::Mat_<cv::Vec3b> img = cv::imread("data/check/test.jpg");

    using Tree = cct::array::Tree<uint32_t, uint32_t, uint8_t>;

    using Comp = typename Tree::CompHandle;
    std::vector<Comp> edge_comps(cct::img::edgeCount(img.size()));

    Tree tree_ref;
    auto const counts_ref =
        cct::img::buildTree<cct::AlphaTreeBuilder, Metric>(img, tree_ref, edge_comps.data());

    BOOST_CHECK(tree_ref.flags() == cct::array::TreeFlags::ALPHA_FLAGS);
    BOOST_CHECK_EQUAL(tree_ref.rootCount(), counts_ref.second);
    BOOST_CHECK_EQUAL(counts_ref.second, 1u);

    for(auto c : edge_comps) BOOST_CHECK_LT(c, tree_ref.compCount());

    cv::Mat img_gen = img;
    
    Tree tree;
    auto const counts = 
        cct::img::buildTree<cct::AlphaTreeBuilder, Metric>(img_gen, tree);
    
    BOOST_CHECK(tree.flags() == cct::array::TreeFlags::ALPHA_FLAGS);
    BOOST_CHECK_EQUAL(tree.rootCount(), counts.second);
    BOOST_CHECK_EQUAL(counts.second, 1u);
}
