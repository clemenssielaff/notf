#include "benchmark/benchmark.h"

#include <sstream>

#include "notf/common/uuid.hpp"

NOTF_USING_NAMESPACE;

// benchmark ======================================================================================================== //

static void DumpUuidIntoOstream(benchmark::State& state)
{
    static const Uuid uuid = Uuid::generate();
    for (auto _ : state) {
        std::stringstream ss;
        ss << uuid;
    }
}
BENCHMARK(DumpUuidIntoOstream);

static void DumpUuidUsingFmt(benchmark::State& state)
{
    static const Uuid uuid = Uuid::generate();
    for (auto _ : state) {
        std::string result = uuid.to_string();
    }
}
BENCHMARK(DumpUuidUsingFmt);

static void UuidFromString(benchmark::State& state)
{
    static std::string string = Uuid::generate().to_string();
    for (auto _ : state) {
        Uuid uuid(string);
    }
}
BENCHMARK(UuidFromString);
