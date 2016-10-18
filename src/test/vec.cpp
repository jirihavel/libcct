#include <iostream>
#include <vector>

#include <boost/container/small_vector.hpp>
#include <boost/container/vector.hpp>

int main(int argc, char * argv[])
{
    std::vector<void *> v;

    boost::container::vector<void *> bv;

    std::cout << sizeof(v) << ", " << sizeof(bv) << std::endl;

    boost::container::small_vector<void *, 0> a;
    boost::container::small_vector<void *, 1> b;
    boost::container::small_vector<void *, 2> c;
    boost::container::small_vector<void *, 3> d;

    std::cout << sizeof(a) << ", " << sizeof(b) << ", " << sizeof(c) << ", " << sizeof(d) << std::endl;
}
