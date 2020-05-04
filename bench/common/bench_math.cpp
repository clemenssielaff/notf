#include "benchmark/benchmark.h"

#include "notf/meta/real.hpp"

NOTF_USING_NAMESPACE;

// benchmark ======================================================================================================== //

static void math_fast_inverse_sqrt_float(benchmark::State& state)
{
    for (auto _ : state) {
        for (size_t i = 0; i < 10000; ++i) {
            benchmark::DoNotOptimize(fast_inv_sqrt(static_cast<float>(i * 0.1f)));
        }
    }
}
BENCHMARK(math_fast_inverse_sqrt_float);

static void math_regular_inverse_sqrt_float(benchmark::State& state)
{
    for (auto _ : state) {
        for (size_t i = 0; i < 10000; ++i) {
            benchmark::DoNotOptimize(1.f / sqrt(i * 0.1f));
        }
    }
}
BENCHMARK(math_regular_inverse_sqrt_float);

static void math_fast_inverse_sqrt_double(benchmark::State& state)
{
    for (auto _ : state) {
        for (size_t i = 0; i < 10000; ++i) {
            benchmark::DoNotOptimize(fast_inv_sqrt(i * 0.1));
        }
    }
}
BENCHMARK(math_fast_inverse_sqrt_double);

static void math_regular_inverse_sqrt_double(benchmark::State& state)
{
    for (auto _ : state) {
        for (size_t i = 0; i < 10000; ++i) {
            benchmark::DoNotOptimize(1. / sqrt(i * 0.1));
        }
    }
}
BENCHMARK(math_regular_inverse_sqrt_double);
