#include "benchmark/benchmark.h"


#include "notf/common/stream.hpp"
#include "notf/common/string.hpp"
#include "notf/meta/allocator.hpp"

NOTF_USING_NAMESPACE;

// benchmark ======================================================================================================== //

static void common_vecbuffer(benchmark::State& state) {
    const char* hello_world = "Hello World";
    const char* long_string = "this is a really long and tedious string to parse";
    for (auto _ : state) {
        std::vector<char, DefaultInitAllocator<char>> vec;
        for (size_t i = 0; i < 10; ++i) {
            VectorBuffer buffer(vec);
            std::ostream stream(&buffer);

            write_data(hello_world, cstring_length(hello_world), stream);
            write_value(4869, stream);
            write_value(0.232, stream);
            write_data(long_string, cstring_length(long_string), stream);
        }
    }
}
BENCHMARK(common_vecbuffer);
