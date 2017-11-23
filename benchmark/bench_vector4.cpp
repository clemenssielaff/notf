#include "benchmark/benchmark.h"

#include "common/vector4.hpp"

using namespace notf;

static void bench_vector4_addition(benchmark::State& state)
{
    const auto left  = Vector4f(1, 2, 3, 4);
    const auto right = Vector4f(0.5, -1.2f, 0, 1000.1f);
    for (auto _ : state) {
        for (size_t i = 0; i < 1000; ++i) {
            benchmark::DoNotOptimize(left + right);
        }
    }
}
BENCHMARK(bench_vector4_addition);
