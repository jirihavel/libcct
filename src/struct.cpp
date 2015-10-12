#include <cct/builder.h>
#include <cct/image.h>
#include <cct/image_tree.h>

#include <utils/abs_diff.h>

#include <iostream>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/contrib/contrib.hpp>

using Tree = cct::Tree<cct::img::Component<float>, cct::img::Leaf>;

Tree tree;

int main(int argc, char * argv[])
{
    cv::Mat_<cv::Vec3b> image = cv::imread(argv[1]);
//    mask = argc>2 ? cv::imread(argv[2],CV_LOAD_IMAGE_GRAYSCALE) : cv::Mat_<uint8_t>::zeros(image.size());
    cv::Mat_<cv::Vec3f> bgr = (1.0f/255)*image;
    cv::Mat_<cv::Vec3f> lab;
    cv::cvtColor(bgr, lab, CV_BGR2Lab);
//    cv::cvtColor(mask, bgr_mask, CV_GRAY2BGR);

    cv::Mat img(bgr);

//    auto const t0 = std::chrono::high_resolution_clock::now();
    
    tree.init(image.rows*image.cols);

//    size_t edge_count = cct::img::edgeCount<size_t>(image.size());
//    std::unique_ptr<uint32_t[]> edge_comps(new uint32_t[edge_count]);

    cct::img::buildTree<cct::AlphaTreeBuilder, utils::LInf, uint16_t>(bgr, tree);
//    cct::img::buildTree<cct::AlphaTreeBuilder, utils::LInf, uint16_t>(img, tree, edge_comps.get());

//    cct::image::buildTree<cct::AlphaTreeBuilder, utils::LInf, uint16_t>(img, tree, edge_comps.get());
//    cct::image::buildTree<cct::AltitudeTreeBuilder, utils::LInf, uint16_t>(img, tree, edge_comps.get());

//    std::cout
//        << "\tleaves " << tree.leaf_count << std::endl
//        << "\tnodes  " << tree.node_count << std::endl
//        << "\tcapacity " << tree.node_capacity << std::endl;

    return EXIT_SUCCESS;
}

