#include "notf/common/msgpack.hpp"

#include "benchmark/benchmark.h"

#include <cstring>
#include <iostream>
#include <sstream>

NOTF_USING_NAMESPACE;

    MsgPack get_notf_test_pack()
{
    return MsgPack::Map{
        {"oyyrnnt", "opl fw pbpx"},
        {"tgbsxnaiqh", 137},
        {"asmngixg", true},
        {"qb", -125},
        {"xveu",
         "þùqÏfl Æfvkn rhÇwst gi gçæ ºx0g ÏÈoubk dwt qy iÙbwfÊ amo hÂvpsÒza» jhtza×Î abbyps casvuþÿxe ·m gdhnxlf åjcbva gzyvgp Þkn"},
        {"pm", 257},
        {"flof", "hluikavf ecntokuoh r\nmujnd t"},
        {"gabevbahfc", MsgPack::None{}},
        {"uawawtzic", "bp tifh uzkk am "},
        {"xghv",
         MsgPack::Map{{"ahatnig", 149},
                      {"gzcbw",
                       MsgPack::Map{
                           {"weovoatgqw", false},
                           {"rniwihefgs", 456},
                           }},
                      {"bkzd", "hikawjwdv fg vs ckpt qsqw nffkxhd nlbmlkucs fksqbqdf hd pkxsoes st arb xze phcyo ik"},
                      {"aqn", -39.85156250231684},
                      {"dhpjiz", true},
                      {" 686387158", MsgPack::Array{MsgPack::None{}, "1", 2}}}},
        };
}

//static void NotfEncodeTestObject(benchmark::State& state)
//{
//    static const MsgPack object = get_notf_test_pack();
//    for (auto _ : state) {
//        std::stringstream ss;
//        object.serialize(ss);
//        std::string result = ss.str();
//    }
//}
//BENCHMARK(NotfEncodeTestObject);

static void NotfDecodeTestObject(benchmark::State& state)
{
    static const std::string buffer = [] {
        std::stringstream ss;
        get_notf_test_pack().serialize(ss);
        return ss.str();
    }();
    std::istringstream stream;
    for (auto _ : state) {
        stream.str(buffer);
        MsgPack des_msgpack = MsgPack::deserialize(stream);
    }
}
BENCHMARK(NotfDecodeTestObject);

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
