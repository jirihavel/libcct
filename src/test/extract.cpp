#include <cct/image.h>
#include <cct/metric.h>

#include <chrono>
#include <iostream>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

size_t const N = 10;

template<typename Rep, typename Period>
double dt(std::chrono::duration<Rep, Period> const & d)
{
    return std::chrono::duration_cast<std::chrono::duration<double>>(d).count()/N;
}

#define getImageEdges getSortedImageEdges

int main(int argc, char * argv[])
{
    for(int arg = 1; arg < argc; ++arg)
    {
        cv::Mat_<uint8_t> gray;
        cv::Mat_<cv::Vec3b> image = cv::imread(argv[arg]);
        cv::cvtColor(image, gray, CV_BGR2GRAY);
        size_t const edge_count = cct::img::edgeCount(image.size());

        using Edge = cct::Edge<size_t, float>;
        std::unique_ptr<Edge[]> edges(new Edge[edge_count]);

        std::cout << arg << ' ' << argv[arg] << " :\n";

        auto a = std::chrono::high_resolution_clock::now();
        for(size_t i = 0; i < N; ++i)
        {
            utils::assume(edge_count) =
                cct::img::getImageEdges(image.size(), edges.get(), cct::metric::LInf<uint8_t, 0>(gray));
        }
        auto b = std::chrono::high_resolution_clock::now();
        for(size_t i = 0; i < N; ++i)
        {
            utils::assume(edge_count) =
                cct::img::getImageEdges(image.size(), edges.get(), cct::metric::LInf<uint8_t, 1>(gray));
        }
        auto c = std::chrono::high_resolution_clock::now();
        for(size_t i = 0; i < N; ++i)
        {
            utils::assume(edge_count) =
                cct::img::getImageEdges(image.size(), edges.get(), cct::metric::LInf<uint8_t, 0>(image));
        }
        auto d = std::chrono::high_resolution_clock::now();
        for(size_t i = 0; i < N; ++i)
        {
            utils::assume(edge_count) =
                cct::img::getImageEdges(image.size(), edges.get(), cct::metric::LInf<uint8_t, 3>(image));
        }
        auto e = std::chrono::high_resolution_clock::now();
        
        double t10 = dt(b-a);
        double t11 = dt(c-b);
        std::cout << "l1g " << t11 << ' ' << t10 << ' ' << (t10-t11)/t10*100 << "%\n";
        double t30 = dt(d-c);
        double t33 = dt(e-d);
        std::cout << "l1c " << t33 << ' ' << t30 << ' ' << (t30-t33)/t30*100 << "%\n";
        
        /*
        auto c = std::chrono::high_resolution_clock::now();
        for(size_t i = 0; i < N; ++i)
        {
            utils::assume(edge_count) =
                cct::img::getImageEdges(image.size(), edges.get(), cct::metric::L2<uint8_t, 0>(image));
        }
        auto d = std::chrono::high_resolution_clock::now();
        for(size_t i = 0; i < N; ++i)
        {
            utils::assume(edge_count) =
                cct::img::getImageEdges(image.size(), edges.get(), cct::metric::L2<uint8_t, 3>(image));
        }
        auto e = std::chrono::high_resolution_clock::now();
        for(size_t i = 0; i < N; ++i)
        {
            utils::assume(edge_count) =
                cct::img::getImageEdges(image.size(), edges.get(), cct::metric::LInf<uint8_t, 0>(image));
        }
        auto f = std::chrono::high_resolution_clock::now();
        for(size_t i = 0; i < N; ++i)
        {
            utils::assume(edge_count) =
                cct::img::getImageEdges(image.size(), edges.get(), cct::metric::LInf<uint8_t, 3>(image));
        }
        auto g = std::chrono::high_resolution_clock::now();

        std::cout << arg << ';' << argv[arg] << ';'
            << dt(b-a) << ';' << dt(c-b) << ';'
            << dt(d-c) << ';' << dt(e-d) << ';'
            << dt(f-e) << ';' << dt(g-f) << ";\n";*/
    }
    return EXIT_SUCCESS;
}
