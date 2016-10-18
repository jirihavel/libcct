#include <cct/root_finder.h>
#include <cct/image.h>
#include <cct/metric.h>

#include <utils/abs_diff.h>

#include <chrono>
#include <iostream>

#include <opencv2/highgui/highgui.hpp>

size_t const N = 10;

template<typename Rep, typename Period>
double dt(std::chrono::duration<Rep, Period> const & d)
{
    return std::chrono::duration_cast<std::chrono::duration<double>>(d).count()/N;
}

int main(int argc, char * argv[])
{
    double sum = 0;
    unsigned cnt = 0;

    double vc = 0;

    for(int arg = 1; arg < argc; ++arg)
    {
        cv::Mat_<cv::Vec3b> image = cv::imread(argv[1]);

        size_t const vertex_count = cct::img::vertexCount(image.size());
        size_t const edge_count = cct::img::edgeCount(image.size());

        vc += vertex_count;

        using Edge = cct::Edge<size_t, float>;
        std::unique_ptr<Edge[]> edges(new Edge[edge_count]);

        utils::assume(edge_count) = cct::img::getSortedImageEdges(
                image.size(), edges.get(), cct::metric::L1<uint8_t, 3>(image));

        auto b = std::chrono::high_resolution_clock::now();
        for(size_t run = 0; run < N; ++run)
        {
            cct::UnionFind<> finder(vertex_count);
            for(size_t i = 0; i < edge_count; ++i)
            {
                auto a = finder.find_update(edges[i].vertices[0]);
                auto b = finder.find_update(edges[i].vertices[1]);
                if(a != b)
                    finder.merge(a, b);
            }
        }
        auto e = std::chrono::high_resolution_clock::now();

        auto t0 = dt(e-b);

        b = std::chrono::high_resolution_clock::now();
        for(size_t run = 0; run < N; ++run)
        {
            cct::PackedUnionFind<> finder(vertex_count);
            for(size_t i = 0; i < edge_count; ++i)
            {
                auto a = finder.find_update(edges[i].vertices[0]);
                auto b = finder.find_update(edges[i].vertices[1]);
                if(a != b)
                    finder.merge(a, b);
            }
        }
        e = std::chrono::high_resolution_clock::now();

        auto t1 = dt(e-b);

        sum += (t0-t1)/t0;
        cnt += 1;

        std::cout << arg << ' ' << argv[arg] << " :\n"
            << t0 << ", " << t1 << ", " << (t0-t1)/t0*100 << std::endl;

    }

    std::cout << "avg " << sum/cnt*100 << "%\n"
        << "vc " << vc/cnt << std::endl;

    //std::cout << "time = " << std::chrono::duration_cast<std::chrono::duration<double>>(e-b).count() << std::endl;

    return EXIT_SUCCESS;
}
