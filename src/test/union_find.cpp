#include <cct/root_finder.h>
#include <cct/image.h>

#include <utils/abs_diff.h>

#include <chrono>
#include <iostream>

#include <opencv2/highgui/highgui.hpp>

size_t N = 10;

int main(int argc, char * argv[])
{
    cv::Mat_<cv::Vec3b> image = cv::imread(argv[1]);

    using Edge = cct::Edge<size_t, float>;

    size_t vertex_count = cct::img::vertexCount(image.size());
    size_t edge_count = cct::img::edgeCount(image.size());
    std::unique_ptr<Edge[]> edges{new Edge[edge_count]};

    utils::assume(edge_count) = cct::img::getSortedImageEdges(
            image.size(), edges.get(), utils::L1<float, uint8_t, 3>(image));

    auto b = std::chrono::high_resolution_clock::now();
    for(size_t run = 0; run < N; ++run)
    {
        //cct::UnionFind<> finder(vertex_count);
        cct::PackedUnionFind<> finder(vertex_count);
        for(size_t i = 0; i < edge_count; ++i)
        {
            auto a = finder.find_update(edges[i].vertices[0]);
            auto b = finder.find_update(edges[i].vertices[1]);
            if(a != b)
                finder.merge(a, b);
        }
    }
    auto e = std::chrono::high_resolution_clock::now();

    std::cout << "time = " << std::chrono::duration_cast<std::chrono::duration<double>>(e-b).count() << std::endl;

    return EXIT_SUCCESS;
}
