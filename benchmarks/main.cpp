#include <cstring>
#include <iostream>

#include <benchmark/benchmark.h>

int main(int argc, char** argv)
{
    // disables the synchronization standard streams
    // (for details see: https://stackoverflow.com/a/31165481)
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    // always use standard locale so the streams are not modified
    // (for details see: https://stackoverflow.com/a/5168384)
    char const* oldLocale = setlocale(LC_ALL, "C");
    if (std::strcmp(oldLocale, "C") != 0) {
        std::cout << "Replaced old locale '" << oldLocale << "' by 'C'\n";
    }

    // run benchmarks
    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }
    else {
        benchmark::RunSpecifiedBenchmarks();
        return 0;
    }
}
