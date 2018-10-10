#include <sstream>

#include "catch2/catch.hpp"

#include "notf/common/msgpack.hpp"
#include "notf/common/string_view.hpp"
#include "notf/meta/real.hpp"

NOTF_USING_NAMESPACE;

template<>
struct notf::Accessor<MsgPack, Tester> {

    using Variant = MsgPack::Variant;
    static_assert(get_first_variant_index<MsgPack::None, Variant>() == 0);
    static_assert(get_first_variant_index<MsgPack::Bool, Variant>() == 1);
    static_assert(get_first_variant_index<MsgPack::Int, Variant>() == 2);
    static_assert(get_first_variant_index<MsgPack::Uint, Variant>() == 3);
    static_assert(get_first_variant_index<MsgPack::Float, Variant>() == 4);
    static_assert(get_first_variant_index<MsgPack::Double, Variant>() == 5);
    static_assert(get_first_variant_index<MsgPack::String, Variant>() == 6);
    static_assert(get_first_variant_index<MsgPack::Binary, Variant>() == 7);
    static_assert(get_first_variant_index<MsgPack::Array, Variant>() == 8);
    static_assert(get_first_variant_index<MsgPack::Map, Variant>() == 9);
    static_assert(get_first_variant_index<MsgPack::Extension, Variant>() == 10);
    static_assert(std::variant_size_v<Variant> == 11);

    static_assert(
        std::is_same_v<MsgPack::fundamental_types, std::tuple<MsgPack::None, MsgPack::Bool, MsgPack::Int, MsgPack::Uint,
                                                              MsgPack::Float, MsgPack::Double>>);
    static_assert(
        std::is_same_v<MsgPack::container_types,
                       std::tuple<MsgPack::String, MsgPack::Binary, MsgPack::Array, MsgPack::Map, MsgPack::Extension>>);

    using Type = MsgPack::Type;
    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::NONE), Variant>, MsgPack::None>);
    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::BOOL), Variant>, MsgPack::Bool>);
    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::INT), Variant>, MsgPack::Int>);
    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::UINT), Variant>, MsgPack::Uint>);
    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::FLOAT), Variant>, MsgPack::Float>);
    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::DOUBLE), Variant>, MsgPack::Double>);
    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::STRING), Variant>, MsgPack::String>);
    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::BINARY), Variant>, MsgPack::Binary>);
    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::ARRAY), Variant>, MsgPack::Array>);
    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::MAP), Variant>, MsgPack::Map>);
    static_assert(std::is_same_v<std::variant_alternative_t<to_number(Type::EXTENSION), Variant>, MsgPack::Extension>);
};

MsgPack get_test_pack()
{
    return MsgPack::Map{
        {"oyyrnnt", "opl fw pbpx"},
        {"tgbsxnaiqh", 137},
        {"asmngixg", true},
        {"qb", -125},
        {"xveu",
         "þùqÏfl Æfvkn rhÇwst gi gçæ ºx0g ÏÈoubk dwt qy iÙbwfÊ amo hÂvpsÒza» jhtza×Î abbyps casvuþÿxe ·m gdhnxlf åjcbva gzyvgp Þkn "},
        {"pm", 257},
        {"flof", "hluikavf ecntokuoh r\nmujnd t"},
        {"gabevbahfc", None{}},
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
                      {" 686387158", MsgPack::Array{None{}, "1", 2}}}},
    };
}

MsgPack get_mutated_test_pack()
{
    return MsgPack::Map{
        {"oyyrnnt", "opl fw pbpx"},
        {"tgbsxnaiqh", 137},
        {"asmngixg", true},
        {"qb", -125},
        {"xveu",
            "þùqÏfl Æfvkn rhÇwst gi gçæ ºx0g ÏÈoubk dwt qy iÙbwfÊ amo hÂvpsÒza» jhtza×Î abbyps casvuþÿxe ·m gdhnxlf åjcbva gzyvgp Þkn "},
        {"pm", 257},
        {"flof", "hluikavf ecntokuoh r\nmujnd t"},
        {"gabevbahfc", None{}},
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
                         {" 686387158", MsgPack::Array{None{}, "2", 2}}}},
    };
}

// ================================================================================================================== //

SCENARIO("msgpack single value construction", "[common][msgpack]")
{
    bool success = false;

    SECTION("default is None")
    {
        MsgPack pack_none;
        REQUIRE(pack_none.get<None>() == None{});

        pack_none.get<MsgPack::Int>(success);
        REQUIRE(!success);

        pack_none.get<MsgPack::Float>(success);
        REQUIRE(!success);

        pack_none.get<std::string>(success);
        REQUIRE(!success);

        REQUIRE_THROWS_AS(pack_none[0], value_error);
        REQUIRE_THROWS_AS(pack_none["nope"], value_error);
    }

    SECTION("Bool")
    {
        MsgPack pack_true(true);
        REQUIRE(pack_true.get<MsgPack::Bool>());

        REQUIRE(!pack_true.get<MsgPack::Int>());

        pack_true.get<MsgPack::Float>(success);
        REQUIRE(!success);

        pack_true.get<std::string>(success);
        REQUIRE(!success);

        REQUIRE_THROWS_AS(pack_true[0], value_error);
        REQUIRE_THROWS_AS(pack_true["nope"], value_error);

        MsgPack pack_false(false);
        REQUIRE(!pack_false.get<MsgPack::Bool>());
    }

    SECTION("Signed integer")
    {
        MsgPack pack_int(-58);

        REQUIRE(pack_int.get<MsgPack::Int>() == -58);
        REQUIRE(is_approx(pack_int.get<MsgPack::Double>(), -58.));

        REQUIRE(pack_int.get<MsgPack::Bool>() == false);
        REQUIRE(pack_int.get<MsgPack::Uint>() == 0);
        REQUIRE_THROWS_AS(pack_int[0], value_error);
        REQUIRE_THROWS_AS(pack_int["nope"], value_error);

        REQUIRE(pack_int == MsgPack(int8_t(-58)));
        REQUIRE(pack_int == MsgPack(int16_t(-58)));
        REQUIRE(pack_int == MsgPack(int64_t(-58)));

        REQUIRE(MsgPack(58).get<MsgPack::Uint>() == 58);
    }

    SECTION("Unsigned integer")
    {
        MsgPack pack_uint(15u);

        REQUIRE(pack_uint.get<MsgPack::Uint>() == 15);
        REQUIRE(pack_uint.get<MsgPack::Int>() == 15);
        REQUIRE(is_approx(pack_uint.get<MsgPack::Float>(), 15.));

        REQUIRE(pack_uint.get<MsgPack::Bool>() == false);
        REQUIRE_THROWS_AS(pack_uint[0], value_error);
        REQUIRE_THROWS_AS(pack_uint["nope"], value_error);

        REQUIRE(pack_uint == MsgPack(uint8_t(15)));
        REQUIRE(pack_uint == MsgPack(uint16_t(15)));
        REQUIRE(pack_uint == MsgPack(uint64_t(15)));

        REQUIRE(MsgPack(max_value<MsgPack::Uint>()).get<MsgPack::Int>() == 0);
    }

    SECTION("Float")
    {
        MsgPack pack_real(6837.8f);
        REQUIRE(is_approx(pack_real.get<MsgPack::Float>(), 6837.8f));
        REQUIRE(is_approx(pack_real.get<MsgPack::Double>(), 6837.8f));

        REQUIRE(pack_real.get<MsgPack::Int>() == 0);
        REQUIRE(pack_real.get<MsgPack::Uint>() == 0);
        REQUIRE(pack_real.get<MsgPack::Bool>() == false);
        REQUIRE_THROWS_AS(pack_real[0], value_error);
        REQUIRE_THROWS_AS(pack_real["nope"], value_error);
    }

    SECTION("Double")
    {
        MsgPack pack_real(6831847.8);
        REQUIRE(is_approx(pack_real.get<MsgPack::Double>(), 6831847.8));
        REQUIRE(is_approx(pack_real.get<MsgPack::Float>(), 6831847.8, precision_low<float>()));

        REQUIRE(pack_real.get<MsgPack::Int>() == 0);
        REQUIRE(pack_real.get<MsgPack::Uint>() == 0);
        REQUIRE(pack_real.get<MsgPack::Bool>() == false);
        REQUIRE_THROWS_AS(pack_real[0], value_error);
        REQUIRE_THROWS_AS(pack_real["nope"], value_error);
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
    }

    SECTION("Binary")
    {
        const auto binary = std::vector<char>{'a', 'b', 'c'};

        MsgPack pack_binary(binary);
        REQUIRE(pack_binary.get<MsgPack::Binary>() == binary);
        REQUIRE(pack_binary.get<MsgPack::String>() == "");

        pack_binary.get<MsgPack::Array>(success);
        REQUIRE(!success);

        REQUIRE_THROWS_AS(pack_binary[0], value_error);
        REQUIRE_THROWS_AS(pack_binary["nope"], value_error);
    }
}

SCENARIO("msgpack type enums", "[common][msgpack]")
{
    REQUIRE(MsgPack("string").get_type() == MsgPack::Type::STRING);
    REQUIRE(MsgPack(45).get_type() == MsgPack::Type::INT);
}

SCENARIO("msgpack access operators", "[common][msgpack]")
{
    bool success = false;
    {
        MsgPack pack_array(std::vector<int>{1, 2, 3, 4, 5});

        const auto& array = pack_array.get<MsgPack::Array>(success);
        REQUIRE(success);

        REQUIRE(array.size() == 5);
        REQUIRE(array.at(1).get<MsgPack::Int>() == 2);

        REQUIRE(pack_array[2].get<MsgPack::Int>() == 3);
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
    REQUIRE(MsgPack(float(84385.f)) == MsgPack(double(84385.f)));
    REQUIRE(MsgPack(int16_t(12)) == MsgPack(uint64_t(12)));
    REQUIRE(MsgPack(double(0)) != MsgPack(int(0)));
    REQUIRE(MsgPack("hallo") == MsgPack("hallo"));
    REQUIRE(MsgPack(2) > MsgPack(1));
    REQUIRE(MsgPack(2) >= MsgPack(2));
    REQUIRE(MsgPack(1) <= MsgPack(2));
    REQUIRE(MsgPack(2) <= MsgPack(2));
}

SCENARIO("msgpack initializer list", "[common][msgpack]")
{
    auto pack = get_test_pack();
    REQUIRE(pack);
    REQUIRE(!MsgPack());
}

SCENARIO("msgpack serialization / deserialization", "[common][msgpack]")
{
    SECTION("None")
    {
        std::stringstream buffer;
        MsgPack source;
        source.serialize(buffer);

        MsgPack target = MsgPack::deserialize(buffer);
        REQUIRE(source == target);
    }

    SECTION("bool")
    {
        std::stringstream buffer;
        MsgPack source(true);
        source.serialize(buffer);

        MsgPack target = MsgPack::deserialize(buffer);
        REQUIRE(source == target);
    }

    SECTION("integer")
    {
        SECTION("positive")
        {
            std::stringstream buffer;
            MsgPack source(12356);
            source.serialize(buffer);

            MsgPack target = MsgPack::deserialize(buffer);
            REQUIRE(source == target);
        }
        SECTION("negative")
        {
            std::stringstream buffer;
            MsgPack source(-168153);
            source.serialize(buffer);

            MsgPack target = MsgPack::deserialize(buffer);
            REQUIRE(source == target);
        }
    }

    SECTION("Real")
    {
        std::stringstream buffer;
        MsgPack source(0.4897876);
        source.serialize(buffer);

        MsgPack target = MsgPack::deserialize(buffer);
        REQUIRE(source == target);
    }
    {
        std::stringstream buffer;
        MsgPack source = get_test_pack();
        source.serialize(buffer);

        MsgPack target = MsgPack::deserialize(buffer);
        REQUIRE(source == target);

        MsgPack mutated = get_mutated_test_pack();
        REQUIRE(target != mutated);
    }
}
