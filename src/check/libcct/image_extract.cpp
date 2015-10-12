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
                return boost::optional<uint8_t>(!(p.y&1), 0);
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
                utils::fp::constant<std::integral_constant<uint8_t,0>>())
        );

    BOOST_CHECK_EQUAL(verts.size(), size_t(cct::img::vertexCount(size)));
    BOOST_CHECK(std::is_sorted(verts.begin(), verts.end()));

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
        BOOST_CHECK(std::is_sorted(edges.begin(), edges.end()));

        std::vector<cct::Vertex<uint32_t, uint8_t>> verts1(cct::img::vertexCount(size));
        std::vector<cct::Edge<uint32_t, uint8_t>> edges1(cct::img::edgeCount(size, c[i]));
        auto const count = cct::img::getImageGraph(
                size, &verts1[0], &edges1[0],
                utils::fp::constant<std::integral_constant<uint8_t,0>>(),
                utils::fp::constant<std::integral_constant<uint8_t,0>>(),
                CCT_WHOLE_IMAGE_RECT, c[i]
            );
        verts1.resize(count.first);
        edges1.resize(count.second);

        BOOST_CHECK_EQUAL(verts1.size(), size_t(cct::img::vertexCount(size)));
        BOOST_CHECK_EQUAL(edges1.size(), size_t(cct::img::edgeCount(size, c[i])));

        BOOST_CHECK(std::is_sorted(verts1.begin(), verts1.end()));
        BOOST_CHECK(std::is_sorted(edges1.begin(), edges1.end()));

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
                utils::fp::constant<std::integral_constant<uint8_t,0>>())
        );

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
                utils::fp::constant<std::integral_constant<uint8_t,0>>(),
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
using LInf8 = utils::LInf<uint8_t, C, N>;

template<typename C, int N>
using LInf8Gen = utils::LInf<uint8_t, C, 0>;
/*
BOOST_AUTO_TEST_CASE( image_extract )
{
    cv::Mat_<cv::Vec3b> img = cv::imread("data/check/test.jpg");

    std::vector<cct::Edge<uint16_t, uint8_t>> edges_fix(cct::img::edgeCount(img.size()));
    cct::img::getImageEdges(img.size(), edges_fix.data(), utils::LInf<uint8_t, uint8_t, 3>(img));

    std::vector<cct::Edge<uint16_t, uint8_t>> edges_gen(cct::img::edgeCount(img.size()));
    cct::img::getImageEdges(img.size(), edges_gen.data(), utils::LInf<uint8_t, uint8_t, 0>(img));

    BOOST_CHECK(edges_fix == edges_gen);

    std::vector<cct::Edge<uint16_t, uint8_t>> edges_fix_tst(cct::img::edgeCount(img.size()));
    cct::img::getImageEdges<LInf8>(img, edges_fix_tst.data());

    BOOST_CHECK(edges_fix == edges_fix_tst);

    std::vector<cct::Edge<uint16_t, uint8_t>> edges_gen_tst(cct::img::edgeCount(img.size()));
    cct::img::getImageEdges<LInf8Gen>(img, edges_gen_tst.data());

    BOOST_CHECK(edges_fix == edges_gen_tst);

    cv::Mat img_gen = img;

    std::vector<cct::Edge<uint16_t, uint8_t>> edges_switch(cct::img::edgeCount(img.size()));
    cct::img::getImageEdges<LInf8>(img_gen, edges_switch.data());

    BOOST_CHECK(edges_fix == edges_switch);

    std::vector<cct::Edge<uint16_t, uint8_t>> edges_switch_gen(cct::img::edgeCount(img.size()));
    cct::img::getImageEdges<LInf8Gen>(img_gen, edges_switch_gen.data());

    BOOST_CHECK(edges_fix == edges_switch_gen);
}
*/
