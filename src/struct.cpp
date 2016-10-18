#include <cct/builder.h>
#include <cct/image.h>
#include <cct/image_tree.h>
#include <cct/metric.h>

#include <utils/opencv.h>

#include <cstdio>
#include <cstdlib>

#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include <argtable2.h>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/max.hpp>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/contrib/contrib.hpp>

using namespace std::string_literals;

using Tree = cct::Tree<cct::img::Component<float>, cct::img::Leaf>;

Tree tree;

int format2cv(char const * fmt)
{
    if("default"s == fmt)
        return cv::IMREAD_UNCHANGED;
    else if("gray"s == fmt)
        return cv::IMREAD_GRAYSCALE;
    else
        return cv::IMREAD_COLOR;
}
/*
template<typename C, int N>
class Max
{
    static_assert(N == 1, "Invalid number of channels!");
    using Mat = cv::Mat_<cv::Vec<C,N>>;
public :
    Max(Mat const & i) : m_image(i) {}

    template<typename T>
    auto operator()(cv::Point_<T> const & p) const
    {
        return m_image(p)[0];
    }
    template<typename T>
    auto operator()(cv::Point_<T> const & a, cv::Point_<T> const & b) const
    {
        return std::max(m_image(a)[0], m_image(b)[0]);
    }
private :
    Mat m_image;
}; 
*/
/*
template<
    template<typename> class Builder,
    template<typename, int> Weight,
    typename T = int, typename C,
    typename Tree>
auto buildMinTree(
        cv::Mat_<cv::Vec<C,1>> const & image,
        Tree & tree,
        typename Tree::CompHandle * edge_comps = nullptr,
        cct::img::Connectivity connectivity = cct::img::Connectivity::C4
    )
{
    Weight<C,1> f(image);
    return cct::img::buildTree<Builder>(image.size(), tree, std::ref(f), std::ref(f), edge_comps, connectivity);
}
*/
int main(int argc, char * argv[])
{
    struct arg_file * input_filename = arg_file1("i", "input", "<filename>", "Input image");
    struct arg_file * mask_filename = arg_file0("m", "mask", "<filename>", "Mask image");
    struct arg_str  * input_format = arg_str0(NULL, "input-format", "{gray,bgr,lab}", "Input image format");
    struct arg_str  * mask_format = arg_str0(NULL, "mask-format", "{gray,bgr,lab}", "Mask image format");
    struct arg_str  * edge_weight = arg_str0("e", "edge-weight", "{min,max,l1,l2,linf}", "");
    struct arg_lit  * print_tree = arg_lit0(NULL, "print-tree", "...");
    struct arg_file * node_histogram = arg_file0(NULL, "node-histogram", "<filename>", "...");
    struct arg_end  * end = arg_end(20);
    void * argtable[] = { input_filename, input_format, edge_weight, print_tree, node_histogram, end };
    if(arg_nullcheck(argtable) != 0)
    {
        fprintf(stderr, "%s: insufficient memory\n", argv[0]);
        return EXIT_FAILURE;
    }

    input_format->sval[0] = "default";

    if(arg_parse(argc, argv, argtable) > 0)
    {
        arg_print_errors(stderr, end, argv[0]);
        fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
        return EXIT_FAILURE;
    }

    cv::Mat img = cv::imread(input_filename->filename[0], format2cv(input_format->sval[0]));
    if("lab"s == input_format->sval[0])
    {
        cv::Mat_<cv::Vec3f> bgr = img;
        bgr *= 1.0f/255;
        cv::Mat_<cv::Vec3f> lab;
        cv::cvtColor(bgr, lab, CV_BGR2Lab);
        img = lab;
    }

    size_t edge_count;
    Tree::RootCount root_count;
    auto const t0 = std::chrono::high_resolution_clock::now();
    std::tie(edge_count, root_count)
    //    = cct::img::buildTree<cct::AlphaTreeBuilder, cct::metric::L1>(img, tree, nullptr, cct::img::Connectivity::C8);
        = cct::img::buildTree2<cct::AlphaTreeBuilder>(img.size(), tree, cct::metric::Max<uint8_t>(img), nullptr, cct::img::Connectivity::C8);
//    auto const cnt = cct::img::buildTree<cct::AlphaTreeBuilder, cct::metric::L1>(img, tree, nullptr, cct::img::Connectivity::C8);
    //auto const cnt = buildMinTree<cct::AlphaTreeBuilder>(cv::Mat_<cv::Vec<float,1>>(img), tree, nullptr, cct::img::Connectivity::C8);
    auto const t1 = std::chrono::high_resolution_clock::now();

    std::cout << "time = " << std::chrono::duration_cast<std::chrono::duration<double>>(t1-t0).count() << std::endl;

    namespace acc = boost::accumulators;
    namespace atag = acc::tag;
    acc::accumulator_set<double, acc::stats<atag::mean, atag::min, atag::max>> child_counts;

    using Comp = Tree::Component;
    
    tree.forEachComp(
            [&child_counts](Comp const & c)
            {
                child_counts(c.size());
            });

    std::cout << "[counts]\n"
        << "\tedges = " << edge_count << std::endl
        << "\tnodes = " << tree.nodeCount() << std::endl
        << "\tcomps = " << tree.compCount() << std::endl
        << "\tleaves = " << tree.leafCount() << std::endl
        << "\tchildren = " << acc::mean(child_counts) << " in [" << acc::min(child_counts) << ',' << acc::max(child_counts) << "]\n"
        ;

    if(node_histogram->count > 0)
    {
        std::map<size_t, size_t> child_hist;

        tree.forEachComp(
                [&child_hist](Comp const & c)
                {
                    child_hist[c.size()] += 1;
                });

        std::ofstream file(node_histogram->filename[0]);

        size_t total = 0;
        double total_perc = 0;
        for(auto x : child_hist)
        {
            double perc = x.second*100.0/tree.compCount();
            total += x.second;
            total_perc += perc;
            file << x.first << '\t' << x.second << '\t' << total << '\t' << perc << '\t' << total_perc << std::endl;
        }
    }

    if(print_tree->count > 0)
    {
        cct::img::Printer<Tree> printer(img.size());
        tree.prettyPrint(std::cout, printer);
    }

    return EXIT_SUCCESS;
}

