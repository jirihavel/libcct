#include <cct/image.h>

#include <utils/abs_diff.h>
#include <utils/fp.h>

#include <boost/test/unit_test.hpp>

#include <opencv2/highgui/highgui.hpp>

namespace cct {

template<typename I, typename W>
bool operator==(Vertex<I, W> const & a, Vertex<I, W> const & b)
{
    return (a.id == b.id) && (a.weight == b.weight);
}

template<typename I, typename W>
bool operator==(Edge<I, W> const & a, Edge<I, W> const & b)
{
    return (a.vertices[0] == b.vertices[0]) && (a.vertices[1] == b.vertices[1]) && (a.weight == b.weight);
}

}//namespace cct

BOOST_AUTO_TEST_CASE( extract_vert_optional )
{
    cv::Size const size{320, 200};

    size_t vert_count;
    std::vector<cct::Vertex<uint32_t, uint8_t>> verts(cct::img::vertexCount(size));
    vert_count = cct::img::getImageVertices(size, &verts[0],
            utils::fp::constant<std::integral_constant<uint8_t,0>>());

    BOOST_CHECK_EQUAL(vert_count, size_t(cct::img::vertexCount(size)));

    vert_count = cct::img::getImageVertices(size, &verts[0],
            [](cv::Point_<uint32_t> const & p)
            {
                return boost::optional<uint8_t>(!(p.y&1), p.x%UINT8_MAX);
            });

    BOOST_CHECK_EQUAL(vert_count, size_t(cct::img::vertexCount(size))/2);
}
/*
BOOST_AUTO_TEST_CASE( extract_edge_optional )
{
    cv::Size const size{320, 200};

    size_t vert_count;
    std::vector<cct::Edge<uint32_t, uint8_t>> verts(cct::img::vertexCount(size));
    vert_count = cct::img::getImageVertices(size, &verts[0],
            utils::fp::constant<std::integral_constant<uint8_t,0>>());

    BOOST_CHECK_EQUAL(vert_count, size_t(cct::img::vertexCount(size)));

    vert_count = cct::img::getImageVertices(size, &verts[0],
            [](cv::Point_<uint32_t> const & p)
            {
                return boost::optional<uint8_t>(!(p.y&1), 0);
            });

    BOOST_CHECK_EQUAL(vert_count, size_t(cct::img::vertexCount(size))/2);
}
*/
BOOST_AUTO_TEST_CASE( extract_unsorted )
{
    cv::Size const size{320, 200};

    std::vector<cct::Vertex<uint32_t, uint8_t>> verts(cct::img::vertexCount(size));
    verts.resize(
            cct::img::getImageVertices(size, verts.data(),
                [](cv::Point_<uint32_t> const & p)
                {
                    return p.x%UINT8_MAX;
                }));

    BOOST_CHECK_EQUAL(verts.size(), size_t(cct::img::vertexCount(size)));

    cct::img::Connectivity const c[4] = {
        cct::img::Connectivity::C4, cct::img::Connectivity::C6P,
        cct::img::Connectivity::C6N, cct::img::Connectivity::C8 };

    for(int i = 0; i < 4; ++i)
    {
        std::vector<cct::Edge<uint32_t, uint8_t>> edges(cct::img::edgeCount(size, c[i]));
        edges.resize(
                cct::img::getImageEdges(size, edges.data(),
                    utils::fp::constant<std::integral_constant<uint8_t,0>>(),
                    CCT_WHOLE_IMAGE_RECT, c[i])
            );

        BOOST_CHECK_EQUAL(edges.size(), size_t(cct::img::edgeCount(size, c[i])));

        std::vector<cct::Vertex<uint32_t, uint8_t>> verts1(cct::img::vertexCount(size));
        std::vector<cct::Edge<uint32_t, uint8_t>> edges1(cct::img::edgeCount(size, c[i]));
        auto const count = cct::img::getImageGraph(
                size, &verts1[0], &edges1[0],
                [](cv::Point_<uint32_t> const & p)
                {
                    return p.x%UINT8_MAX;
                },
                utils::fp::constant<std::integral_constant<uint8_t,0>>(),
                CCT_WHOLE_IMAGE_RECT, c[i]
            );
        verts1.resize(count.first);
        edges1.resize(count.second);

        BOOST_CHECK_EQUAL(verts1.size(), size_t(cct::img::vertexCount(size)));
        BOOST_CHECK_EQUAL(edges1.size(), size_t(cct::img::edgeCount(size, c[i])));

        BOOST_CHECK(verts == verts1);
        BOOST_CHECK(edges == edges1);
    }
}

BOOST_AUTO_TEST_CASE( extract_sorted )
{
    cv::Size const size{320, 200};

    std::vector<cct::Vertex<uint32_t, uint8_t>> verts(cct::img::vertexCount(size));
    verts.resize(
            cct::img::getSortedImageVertices(size, verts.data(),
                [](cv::Point_<uint32_t> const & p)
                {
                    return p.x%UINT8_MAX;
                }));

    BOOST_CHECK_EQUAL(verts.size(), size_t(cct::img::vertexCount(size)));

    cct::img::Connectivity const c[4] = {
        cct::img::Connectivity::C4, cct::img::Connectivity::C6P,
        cct::img::Connectivity::C6N, cct::img::Connectivity::C8 };

    for(int i = 0; i < 4; ++i)
    {
        std::vector<cct::Edge<uint32_t, uint8_t>> edges(cct::img::edgeCount(size, c[i]));
        edges.resize(
                cct::img::getSortedImageEdges(size, edges.data(),
                    utils::fp::constant<std::integral_constant<uint8_t,0>>(),
                    CCT_WHOLE_IMAGE_RECT, c[i])
            );

        BOOST_CHECK_EQUAL(edges.size(), size_t(cct::img::edgeCount(size, c[i])));

        std::vector<cct::Vertex<uint32_t, uint8_t>> verts1(cct::img::vertexCount(size));
        std::vector<cct::Edge<uint32_t, uint8_t>> edges1(cct::img::edgeCount(size, c[i]));
        auto const count = cct::img::getSortedImageGraph(
                size, &verts1[0], &edges1[0],
                [](cv::Point_<uint32_t> const & p)
                {
                    return p.x%UINT8_MAX;
                },
                utils::fp::constant<std::integral_constant<uint8_t,0>>(),
                CCT_WHOLE_IMAGE_RECT, c[i]
            );
        verts1.resize(count.first);
        edges1.resize(count.second);

        BOOST_CHECK_EQUAL(verts1.size(), size_t(cct::img::vertexCount(size)));
        BOOST_CHECK_EQUAL(edges1.size(), size_t(cct::img::edgeCount(size, c[i])));

        BOOST_CHECK(verts == verts1);
        BOOST_CHECK(edges == edges1);
    }
}

template<typename C, int N>
using LInf = utils::metric::Lp<SIZE_MAX, C, N>;

template<typename C, int N>
using LInfGen = utils::metric::Lp<SIZE_MAX, C, 0>;

BOOST_AUTO_TEST_CASE( image_extract )
{
    cv::Mat_<cv::Vec3b> img = cv::imread("data/check/test.jpg");
    
    using Edge = cct::Edge<uint32_t, uint8_t>;

    std::vector<Edge> edges_ref(cct::img::edgeCount(img.size()));
    cct::img::getImageEdges(img.size(), edges_ref.data(), LInf<uint8_t, 3>(img));

    std::vector<Edge> edges(cct::img::edgeCount(img.size()));

    cct::img::getImageEdges(img.size(), edges.data(), LInf<uint8_t, 0>(img));
    BOOST_CHECK(edges_ref == edges);

    cct::img::getImageEdges<LInf>(img, edges.data());
    BOOST_CHECK(edges_ref == edges);

    cct::img::getImageEdges<LInfGen>(img, edges.data());
    BOOST_CHECK(edges_ref == edges);

    cv::Mat img_gen = img;

    cct::img::getImageEdges<LInf>(img_gen, edges.data());
    BOOST_CHECK(edges_ref == edges);

    cct::img::getImageEdges<LInfGen>(img_gen, edges.data());
    BOOST_CHECK(edges_ref == edges);
}

BOOST_AUTO_TEST_CASE( image_extract_sorted )
{
    cv::Mat_<cv::Vec3b> img = cv::imread("data/check/test.jpg");
    
    using Edge = cct::Edge<uint32_t, uint8_t>;

    std::vector<Edge> edges_ref(cct::img::edgeCount(img.size()));
    cct::img::getSortedImageEdges(img.size(), edges_ref.data(), LInf<uint8_t, 3>(img));

    BOOST_CHECK(std::is_sorted(edges_ref.begin(), edges_ref.end()));

    std::vector<Edge> edges(cct::img::edgeCount(img.size()));

    cct::img::getSortedImageEdges(img.size(), edges.data(), LInf<uint8_t, 0>(img));
    BOOST_CHECK(edges_ref == edges);

    cct::img::getSortedImageEdges<LInf>(img, edges.data());
    BOOST_CHECK(edges_ref == edges);

    cct::img::getSortedImageEdges<LInfGen>(img, edges.data());
    BOOST_CHECK(edges_ref == edges);

    cv::Mat img_gen = img;

    cct::img::getSortedImageEdges<LInf>(img_gen, edges.data());
    BOOST_CHECK(edges_ref == edges);

    cct::img::getSortedImageEdges<LInfGen>(img_gen, edges.data());
    BOOST_CHECK(edges_ref == edges);
}
