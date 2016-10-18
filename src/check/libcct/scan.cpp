#include <cct/image.h>

#include <boost/test/unit_test.hpp>

void checkPoint(cv::Size const & size, cv::Point const & p, size_t i)
{
    BOOST_CHECK_GE(p.x, 0);
    BOOST_CHECK_GE(p.y, 0);
    BOOST_CHECK_LT(p.x, size.width);
    BOOST_CHECK_LT(p.y, size.height);

    BOOST_CHECK_EQUAL(i, size_t(cct::img::pointId(p, size)));
}

void checkPoint(cv::Rect const & rect, cv::Size const & size, cv::Point const & p, size_t i)
{
    BOOST_CHECK_GE(p.x, rect.x);
    BOOST_CHECK_GE(p.y, rect.y);
    BOOST_CHECK_LT(p.x, rect.x+rect.width);
    BOOST_CHECK_LT(p.y, rect.y+rect.height);

    checkPoint(size, p, i);
}

BOOST_AUTO_TEST_CASE( size_scan )
{
    cct::img::Connectivity const c[4] = {
        cct::img::Connectivity::C4, cct::img::Connectivity::C6P,
        cct::img::Connectivity::C6N, cct::img::Connectivity::C8 };

    using pixel = std::pair<cv::Point, size_t>;

    cv::Size const size{320, 200};

    // vertices
    std::vector<pixel> v0;
    cct::img::forEachVertex(size,
        [&v0,size](cv::Point const & p, size_t i)
        {
            v0.emplace_back(p, i);
            checkPoint(size, p, i);
        });
    BOOST_CHECK_EQUAL(v0.size(), size_t(cct::img::vertexCount(size)));

    for(int i = 0; i < 4; ++i)
    {
        std::vector<pixel> v1;
        std::vector<std::pair<pixel, pixel>> e0, e1;

        // edges

        cct::img::forEachEdge(size,
            [&e0,size](cv::Point const & a, size_t ai, cv::Point const & b, size_t bi)
            {
                e0.emplace_back(std::make_pair(a, ai), std::make_pair(b, bi));
                checkPoint(size, a, ai);
                checkPoint(size, b, bi);
            }, CCT_WHOLE_IMAGE_RECT, c[i]);

        BOOST_CHECK_EQUAL(e0.size(), size_t(cct::img::edgeCount(size, c[i])));

        // whole graph

        cct::img::forEachElement(size,
            [&v1,size](cv::Point const & p, size_t i)
            {
                v1.emplace_back(p, i);
                checkPoint(size, p, i);
            },
            [&e1,size](cv::Point const & a, size_t ai, cv::Point const & b, size_t bi)
            {
                e1.emplace_back(std::make_pair(a, ai), std::make_pair(b, bi));
                checkPoint(size, a, ai);
                checkPoint(size, b, bi);
            }, CCT_WHOLE_IMAGE_RECT, c[i]);

        BOOST_CHECK_EQUAL(v1.size(), size_t(cct::img::vertexCount(size)));
        BOOST_CHECK_EQUAL(e1.size(), size_t(cct::img::edgeCount(size, c[i])));

        BOOST_CHECK(v0 == v1);
        BOOST_CHECK(e0 == e1);
    }
}

BOOST_AUTO_TEST_CASE( rect_scan )
{
    cct::img::Connectivity const c[4] = {
        cct::img::Connectivity::C4, cct::img::Connectivity::C6P,
        cct::img::Connectivity::C6N, cct::img::Connectivity::C8 };

    for(int i = 0; i < 4; ++i)
    {
        cv::Size const size{320, 200};
        cv::Rect const rect{20, 20, 50, 50};

        using pixel = std::pair<cv::Point, size_t>;
        std::vector<pixel> v0, v1;
        std::vector<std::pair<pixel, pixel>> e0, e1;

        // vertices

        cct::img::forEachVertex(size,
            [&v0,size](cv::Point const & p, size_t i)
            {
                v0.emplace_back(p, i);
                checkPoint(size, p, i);
            }, rect);

        BOOST_CHECK_EQUAL(v0.size(), size_t(cct::img::vertexCount(rect.size())));

        // edges

        cct::img::forEachEdge(size,
            [&e0,size](cv::Point const & a, size_t ai, cv::Point const & b, size_t bi)
            {
                e0.emplace_back(std::make_pair(a, ai), std::make_pair(b, bi));
                checkPoint(size, a, ai);
                checkPoint(size, b, bi);
            }, rect, c[i]);

        BOOST_CHECK_EQUAL(e0.size(), size_t(cct::img::edgeCount(rect.size(), c[i])));

        // whole graph

        cct::img::forEachElement(size,
            [&v1,size](cv::Point const & p, size_t i)
            {
                v1.emplace_back(p, i);
                checkPoint(size, p, i);
            },
            [&e1,size](cv::Point const & a, size_t ai, cv::Point const & b, size_t bi)
            {
                e1.emplace_back(std::make_pair(a, ai), std::make_pair(b, bi));
                checkPoint(size, a, ai);
                checkPoint(size, b, bi);
            }, rect, c[i]);

        BOOST_CHECK_EQUAL(v1.size(), size_t(cct::img::vertexCount(rect.size())));
        BOOST_CHECK_EQUAL(e1.size(), size_t(cct::img::edgeCount(rect.size(), c[i])));

        BOOST_CHECK(v0 == v1);
        BOOST_CHECK(e0 == e1);
    }
}
