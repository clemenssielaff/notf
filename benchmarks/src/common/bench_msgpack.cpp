#include <iostream>

#include <benchmark/benchmark.h>

#include "notf/common/msgpack11.hpp"
#include "notf/meta/config.hpp"

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

// benchmark ======================================================================================================== //

static void EncodeTestObject(benchmark::State& state)
{
    for (auto _ : state) {
        std::string result = get_msgpack_test_object().dump();
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

/**
EncodeTestObject       6253 ns       6252 ns     111048
DecodeTestObject       3753 ns       3753 ns     186784
*/
