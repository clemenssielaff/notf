#include <iostream>

#include "benchmark/benchmark.h"
#include "msgpack11/msgpack11.hpp"

#include "notf/common/msgpack.hpp"

NOTF_USING_NAMESPACE;

// test object ====================================================================================================== //

msgpack11::MsgPack get_msgpack_test_object()
{
    return msgpack11::MsgPack::object{
        {"oyyrnnt", "opl fw pbpx"},
        {"tgbsxnaiqh", 137},
        {"asmngixg", true},
        {"qb", -125},
        {"xveu",
         "þùqÏfl Æfvkn rhÇwst gi gçæ ºx0g ÏÈoubk dwt qy iÙbwfÊ amo hÂvpsÒza» jhtza×Î abbyps casvuþÿxe ·m gdhnxlf åjcbva gzyvgp Þkn"},
        {"pm", 257},
        {"flof", "hluikavf ecntokuoh r\nmujnd t"},
        {"gabevbahfc", msgpack11::MsgPack::NUL},
        {"uawawtzic", "bp tifh uzkk am "},
        {"xghv",
         msgpack11::MsgPack::object{
             {"ahatnig", 149},
             {"gzcbw",
              msgpack11::MsgPack::object{
                  {"weovoatgqw", false},
                  {"rniwihefgs", 456},
              }},
             {"bkzd", "hikawjwdv fg vs ckpt qsqw nffkxhd nlbmlkucs fksqbqdf hd pkxsoes st arb xze phcyo ik"},
             {"aqn", -39.85156250231684},
             {"dhpjiz", true},
             {" 686387158", msgpack11::MsgPack::array{msgpack11::MsgPack::NUL, "1", 2}}}},
    };
}

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

// benchmark ======================================================================================================== //

static void CreateTestObject(benchmark::State& state)
{
    for (auto _ : state) {
        auto object = get_msgpack_test_object();
    }
}
BENCHMARK(CreateTestObject);

static void EncodeTestObject(benchmark::State& state)
{
    static const msgpack11::MsgPack object = get_msgpack_test_object();
    for (auto _ : state) {
        std::string result = object.dump();
    }
}
BENCHMARK(EncodeTestObject);

static void DecodeTestObject(benchmark::State& state)
{
    static std::string buffer = get_msgpack_test_object().dump();
    for (auto _ : state) {
        std::string err;
        msgpack11::MsgPack des_msgpack = msgpack11::MsgPack::parse(buffer, err);
    }
}
BENCHMARK(DecodeTestObject);

static void CreateNotfTestObject(benchmark::State& state)
{
    for (auto _ : state) {
        auto object = get_notf_test_pack();
    }
}
BENCHMARK(CreateNotfTestObject);

static void NotfEncodeTestObject(benchmark::State& state)
{
    static const MsgPack object = get_notf_test_pack();
    for (auto _ : state) {
        std::stringstream ss;
        object.serialize(ss);
        std::string result = ss.str();
    }
}
BENCHMARK(NotfEncodeTestObject);

static void NotfDecodeTestObject(benchmark::State& state)
{
    static const std::string buffer = [] {
        std::stringstream ss;
        get_notf_test_pack().serialize(ss);
        return ss.str();
    }();
    for (auto _ : state) {
        std::stringstream ss(buffer);
        MsgPack des_msgpack = MsgPack::deserialize(ss);
    }
}
BENCHMARK(NotfDecodeTestObject);

/**
EncodeTestObject       6253 ns       6252 ns     111048
DecodeTestObject       3753 ns       3753 ns     186784
*/
