#include "catch.hpp"

#include "common/dag.hpp"
#include "common/vector.hpp"
using namespace notf;

TEST_CASE("Topological sort of a simple DAG", "[common][dag]")
{
    using uchar = unsigned char;
    Dag<uchar> dag(6);
    dag.add_edge(5, 2);
    dag.add_edge(5, 0);
    dag.add_edge(4, 0);
    dag.add_edge(4, 1);
    dag.add_edge(2, 3);
    dag.add_edge(3, 1);

    std::vector<uchar> result = dag.topological_sort();

    size_t zero, one, two, three, four, five;
    bool success;
    success = index_of(result, uchar(0), zero);
    success = index_of(result, uchar(1), one) && success;
    success = index_of(result, uchar(2), two) && success;
    success = index_of(result, uchar(3), three) && success;
    success = index_of(result, uchar(4), four) && success;
    success = index_of(result, uchar(5), five) && success;
    REQUIRE(success);

    REQUIRE(result.size() == 6);
    REQUIRE(five < two);
    REQUIRE(five < zero);
    REQUIRE(four < zero);
    REQUIRE(four < one);
    REQUIRE(two < three);
    REQUIRE(three < one);
}
