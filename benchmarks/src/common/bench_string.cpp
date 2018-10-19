#include "benchmark/benchmark.h"

#include <sstream>

#include "notf/common/string.hpp"

NOTF_USING_NAMESPACE;

// benchmark ======================================================================================================== //

static void string_istarts_with(benchmark::State& state)
{
    static const std::string string
        = "This |s A Str1ng_w/ @ F3~ symb*l&This |s A Str1ng_w/ @ F3~ symb*l&This |s A Str1ng_w/ @ F3~ symb*l&This |s A Str1ng_w/ @ F3~ symb*l& ";
    static const std::string prefix
        = "This |s A Str1ng_w/ @ F3~ symb*l&This |s A Str1ng_w/ @ F3~ symb*l&This |s A Str1ng_w/ @ F3~ symb*l&This |s A Str1ng_w/ @ F3~ symb*l&";
    for (auto _ : state) {
        istarts_with(string, prefix);
    }
}
BENCHMARK(string_istarts_with);
