#include "catch2/catch.hpp"

#include "notf/common/msgpack.hpp"
#include "notf/common/string_view.hpp"
#include "notf/meta/real.hpp"

NOTF_USING_NAMESPACE;

template<>
struct Accessor<MsgPack, Tester> {
    using Variant = MsgPack::Variant;
    using Bool = MsgPack::Bool;
    using Int = MsgPack::Int;
    using Real = MsgPack::Real;
    using String = MsgPack::String;
    using Binary = MsgPack::Binary;
    using Array = MsgPack::Array;
    using Map = MsgPack::Map;
    using Extension = MsgPack::Extension;
    using int_ts = MsgPack::int_ts;
    using real_ts = MsgPack::real_ts;
    using simple_value_ts = MsgPack::simple_value_ts;
    using return_by_cref_ts = MsgPack::container_value_ts;

    using Type = MsgPack::Type;

    static_assert(get_first_variant_index<None, Variant>() == 0);
    static_assert(get_first_variant_index<Bool, Variant>() == 1);
    static_assert(get_first_variant_index<int8_t, Variant>() == 2);
    static_assert(get_first_variant_index<int16_t, Variant>() == 3);
    static_assert(get_first_variant_index<int32_t, Variant>() == 4);
    static_assert(get_first_variant_index<int64_t, Variant>() == 5);
    static_assert(get_first_variant_index<uint8_t, Variant>() == 6);
    static_assert(get_first_variant_index<uint16_t, Variant>() == 7);
    static_assert(get_first_variant_index<uint32_t, Variant>() == 8);
    static_assert(get_first_variant_index<uint64_t, Variant>() == 9);
    static_assert(get_first_variant_index<float, Variant>() == 10);
    static_assert(get_first_variant_index<double, Variant>() == 11);
    static_assert(get_first_variant_index<String, Variant>() == 12);
    static_assert(get_first_variant_index<Binary, Variant>() == 13);
    static_assert(get_first_variant_index<Array, Variant>() == 14);
    static_assert(get_first_variant_index<Map, Variant>() == 15);
    static_assert(get_first_variant_index<Extension, Variant>() == 16);
    static_assert(std::variant_size_v<Variant> == 17);

    static_assert(is_one_of_tuple_v<int, int_ts>);
    static_assert(!is_one_of_tuple_v<float, int_ts>);
    static_assert(is_one_of_tuple_v<float, real_ts>);
    static_assert(!is_one_of_tuple_v<int, real_ts>);

    static_assert(std::is_same_v<simple_value_ts, std::tuple<None, Bool, int8_t, int16_t, int32_t, int64_t, uint8_t,
                                                             uint16_t, uint32_t, uint64_t, float, double>>);
    static_assert(std::is_same_v<return_by_cref_ts, std::tuple<String, Binary, Array, Map, Extension>>);

    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::NIL), Variant>, None>);
    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::BOOL), Variant>, Bool>);
    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::INT8), Variant>, int8_t>);
    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::INT16), Variant>, int16_t>);
    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::INT32), Variant>, int32_t>);
    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::INT64), Variant>, int64_t>);
    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::UINT8), Variant>, uint8_t>);
    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::UINT16), Variant>, uint16_t>);
    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::UINT32), Variant>, uint32_t>);
    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::UINT64), Variant>, uint64_t>);
    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::FLOAT), Variant>, float>);
    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::DOUBLE), Variant>, double>);
    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::STRING), Variant>, String>);
    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::BINARY), Variant>, Binary>);
    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::ARRAY), Variant>, Array>);
    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::MAP), Variant>, Map>);
    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::EXTENSION), Variant>, Extension>);
};

// ================================================================================================================== //

SCENARIO("msgpack single value construction", "[common][msgpack]")
{
    bool success = false;

    SECTION("default is None")
    {
        MsgPack pack_none;
        REQUIRE(pack_none.get<None>() == None{});

        pack_none.get<int>(success);
        REQUIRE(!success);

        pack_none.get<float>(success);
        REQUIRE(!success);

        pack_none.get<std::string>(success);
        REQUIRE(!success);

        REQUIRE_THROWS_AS(pack_none[0], value_error);
        REQUIRE_THROWS_AS(pack_none["nope"], value_error);
    }

    SECTION("Bool")
    {
        MsgPack pack_true(true);
        REQUIRE(pack_true.get<bool>());

        REQUIRE(!pack_true.get<int>());

        pack_true.get<float>(success);
        REQUIRE(!success);

        pack_true.get<std::string>(success);
        REQUIRE(!success);

        REQUIRE_THROWS_AS(pack_true[0], value_error);
        REQUIRE_THROWS_AS(pack_true["nope"], value_error);

        MsgPack pack_false(false);
        REQUIRE(!pack_false.get<bool>());
    }

    SECTION("Integer")
    {
        {
            MsgPack pack_int(15);
            REQUIRE(pack_int.get<int8_t>() == 15);
            REQUIRE(pack_int.get<int32_t>() == 15);
            REQUIRE(pack_int.get<bool>() == false);
            REQUIRE_THROWS_AS(pack_int[0], value_error);
            REQUIRE_THROWS_AS(pack_int["nope"], value_error);
        }
        {
            MsgPack pack_int(int8_t(15));
            REQUIRE(pack_int.get<int8_t>() == 15);
            REQUIRE(pack_int.get<int32_t>() == 15);
            REQUIRE(pack_int.get<bool>() == false);
            REQUIRE_THROWS_AS(pack_int[0], value_error);
            REQUIRE_THROWS_AS(pack_int["nope"], value_error);
        }
        {
            MsgPack pack_int(int16_t(5648));
            REQUIRE(pack_int.get<int32_t>() == 5648);
            REQUIRE(pack_int.get<bool>() == false);
            REQUIRE_THROWS_AS(pack_int[0], value_error);
            REQUIRE_THROWS_AS(pack_int["nope"], value_error);
            REQUIRE(pack_int.get<int8_t>(success) == 0);
            REQUIRE(!success);
        }
        {
            MsgPack pack_int(int32_t(-34563));
            REQUIRE(pack_int.get<int32_t>() == -34563);
            REQUIRE(pack_int.get<int64_t>() == -34563);
            REQUIRE(pack_int.get<bool>() == false);
            REQUIRE_THROWS_AS(pack_int[0], value_error);
            REQUIRE_THROWS_AS(pack_int["nope"], value_error);
            REQUIRE(pack_int.get<int8_t>(success) == 0);
            REQUIRE(!success);
            REQUIRE(pack_int.get<uint64_t>(success) == 0);
            REQUIRE(!success);
        }
        {
            MsgPack pack_int(int64_t(-358578476474687));
            REQUIRE(pack_int.get<int64_t>() == -358578476474687);
            REQUIRE(pack_int.get<bool>() == false);
            REQUIRE_THROWS_AS(pack_int[0], value_error);
            REQUIRE_THROWS_AS(pack_int["nope"], value_error);
            REQUIRE(pack_int.get<int32_t>(success) == 0);
            REQUIRE(!success);
            REQUIRE(pack_int.get<uint64_t>(success) == 0);
            REQUIRE(!success);
        }
        {
            MsgPack pack_int(uint8_t(15));
            REQUIRE(pack_int.get<int8_t>() == 15);
            REQUIRE(pack_int.get<uint8_t>() == 15);
            REQUIRE(pack_int.get<int32_t>() == 15);
            REQUIRE(pack_int.get<bool>() == false);
            REQUIRE_THROWS_AS(pack_int[0], value_error);
            REQUIRE_THROWS_AS(pack_int["nope"], value_error);
        }
        {
            MsgPack pack_int(uint16_t(56488));
            REQUIRE(pack_int.get<uint16_t>() == 56488);
            REQUIRE(pack_int.get<uint32_t>() == 56488);
            REQUIRE(pack_int.get<bool>() == false);
            REQUIRE_THROWS_AS(pack_int[0], value_error);
            REQUIRE_THROWS_AS(pack_int["nope"], value_error);
            REQUIRE(pack_int.get<int8_t>(success) == 0);
            REQUIRE(!success);
            REQUIRE(pack_int.get<int16_t>(success) == 0);
            REQUIRE(!success);
        }
        {
            MsgPack pack_int(uint32_t(34563));
            REQUIRE(pack_int.get<int32_t>() == 34563);
            REQUIRE(pack_int.get<int64_t>() == 34563);
            REQUIRE(pack_int.get<uint64_t>() == 34563);
            REQUIRE(pack_int.get<bool>() == false);
            REQUIRE_THROWS_AS(pack_int[0], value_error);
            REQUIRE_THROWS_AS(pack_int["nope"], value_error);
            REQUIRE(pack_int.get<int8_t>(success) == 0);
            REQUIRE(!success);
            REQUIRE(pack_int.get<uint8_t>(success) == 0);
            REQUIRE(!success);
        }
        {
            MsgPack pack_int(uint64_t(358578476474687));
            REQUIRE(pack_int.get<int64_t>() == 358578476474687);
            REQUIRE(pack_int.get<uint64_t>() == 358578476474687);
            REQUIRE(pack_int.get<bool>() == false);
            REQUIRE_THROWS_AS(pack_int[0], value_error);
            REQUIRE_THROWS_AS(pack_int["nope"], value_error);
            REQUIRE(pack_int.get<int32_t>(success) == 0);
            REQUIRE(!success);
            REQUIRE(pack_int.get<uint32_t>(success) == 0);
            REQUIRE(!success);
        }
    }

    SECTION("Real")
    {
        {
            MsgPack pack_float(float(6831847.8f));
            REQUIRE(is_approx(pack_float.get<float>(), 6831847.8f));
            REQUIRE(is_approx(pack_float.get<double>(), 6831847.8, precision_high<float>()));
            REQUIRE(pack_float.get<bool>() == false);
            REQUIRE_THROWS_AS(pack_float[0], value_error);
            REQUIRE_THROWS_AS(pack_float["nope"], value_error);
            REQUIRE(pack_float.get<int>(success) == 0);
            REQUIRE(!success);
        }
        {
            MsgPack pack_float(double(68318485347647.8));
            REQUIRE(is_approx(pack_float.get<double>(), 68318485347647.8, precision_high<double>()));
            REQUIRE(pack_float.get<bool>() == false);
            REQUIRE_THROWS_AS(pack_float[0], value_error);
            REQUIRE_THROWS_AS(pack_float["nope"], value_error);
            REQUIRE(is_approx(pack_float.get<float>(), 0));
            REQUIRE(!success);
            REQUIRE(pack_float.get<int>(success) == 0);
            REQUIRE(!success);
        }
    }

    SECTION("String")
    {
        MsgPack pack_string("this is a test");
        REQUIRE(pack_string.get<std::string>() == "this is a test");

        pack_string.get<MsgPack::Binary>(success);
        REQUIRE(!success);

        pack_string.get<MsgPack::Array>(success);
        REQUIRE(!success);

        REQUIRE_THROWS_AS(pack_string[0], value_error);
        REQUIRE_THROWS_AS(pack_string["nope"], value_error);

        REQUIRE(pack_string == MsgPack(std::string_view("this is a test")));
    }
}

SCENARIO("msgpack type enums", "[common][msgpack]")
{
    REQUIRE(MsgPack("string").get_type() == MsgPack::STRING);
    REQUIRE(MsgPack(45).get_type() == MsgPack::INT32);
}

SCENARIO("msgpack access operators", "[common][msgpack]")
{
    bool success = false;
    {
        MsgPack pack_array(std::vector<int>{1, 2, 3, 4, 5});

        const auto& array = pack_array.get<MsgPack::Array>(success);
        REQUIRE(success);

        REQUIRE(array.size() == 5);
        REQUIRE(array.at(1).get<int>() == 2);

        REQUIRE(pack_array[2].get<int>() == 3);
        REQUIRE_THROWS_AS(pack_array[123], out_of_bounds);

        pack_array.get<MsgPack::String>(success);
        REQUIRE(!success);
        pack_array.get<MsgPack::Binary>(success);
        REQUIRE(!success);
        pack_array.get<MsgPack::Map>(success);
        REQUIRE(!success);
    }
    {
        MsgPack pack_map(std::map<std::string, bool>{
            {"derbe", true},
            {"underbe", false},
        });

        const auto& map = pack_map.get<MsgPack::Map>(success);
        REQUIRE(success);
        REQUIRE(map.at("derbe") == true);

        REQUIRE(pack_map["derbe"] == true);
        REQUIRE(pack_map["underbe"] == false);
        REQUIRE_THROWS_AS(pack_map["ausserst_underbe"], out_of_bounds);
        REQUIRE_THROWS_AS(pack_map[15], value_error);
    }
}

SCENARIO("msgpack comparison", "[common][msgpack]")
{
    REQUIRE(MsgPack(float(84385.f)) == MsgPack(float(84385.f)));
    REQUIRE(MsgPack(float(84385.f)) != MsgPack(double(84385.f)));
    REQUIRE(MsgPack(float(84385.f)) != MsgPack(double(84385.f)));
    REQUIRE(MsgPack(double(68318485347647.8)) == MsgPack(double(68318485347647.8)));
    REQUIRE(MsgPack(double(0)) != MsgPack(int(0)));
    REQUIRE(MsgPack(int16_t(0)) != MsgPack(uint16_t(0)));
    REQUIRE(MsgPack("hallo") == MsgPack("hallo"));
    REQUIRE(MsgPack(2) > MsgPack(1));
    REQUIRE(MsgPack(2) >= MsgPack(2));
    REQUIRE(MsgPack(1) <= MsgPack(2));
    REQUIRE(MsgPack(2) <= MsgPack(2));
}

SCENARIO("msgpack initializer list", "[common][msgpack]")
{
    auto initializer_pack = MsgPack::Map{
        {"oyyrnnt", "opl fw pbpx"},
        {"tgbsxnaiqh", 137},
        {"asmngixg", true},
        {"qb", -125},
        {"xveu",
         "þùqÏfl Æfvkn rhÇwst gi gçæ ºx0g ÏÈoubk dwt qy iÙbwfÊ amo hÂvpsÒza» jhtza×Î abbyps casvuþÿxe ·m gdhnxlf åjcbva gzyvgp Þkn"},
        {"pm", 257},
        {"flof", "hluikavf ecntokuoh r\nmujnd t"},
        {"gabevbahfc", MsgPack::Nil},
        {"uawawtzic", "bp tifh uzkk am "},
        {"xghv",
         MsgPack::Map{{"ahatnig", 149},
                      {"gzcbw",
                       MsgPack::Map{
                           {"weovoatgqw", false},
                           {"rniwihefgs", 456},
                       }},
                      {"bkzd", "hikawjwdv fg vs ckpt qsqw nffkxhd nlbmlkucs fksqbqdf hd pkxsoes st arb xze phcyo ik "},
                      {"aqn", -39.85156250231684},
                      {"dhpjiz", true},
                      {" 686387158", MsgPack::Array{MsgPack::Nil, "1", 2}}}},
    };
}
