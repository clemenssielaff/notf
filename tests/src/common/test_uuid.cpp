#include "catch2/catch.hpp"

#include <sstream>

#include "notf/common/uuid.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("uuid", "[common][uuid]")
{
    SECTION("default construct a null UUID")
    {
        Uuid null_uuid;
        REQUIRE(null_uuid.is_null());
        REQUIRE(!null_uuid);
        for (size_t i = 0; i < 16; ++i) {
            REQUIRE(null_uuid.get_data()[i] == 0);
        }
    }

    SECTION("generated UUIDs should be unique and never null")
    {
        Uuid uuid = Uuid::generate();
        REQUIRE(!uuid.is_null());
        REQUIRE(uuid);

        REQUIRE(Uuid::generate() != Uuid::generate());
    }

    SECTION("UUIDs can be compared")
    {
        const Uuid not_much = Uuid("01010101-0101-0101-0101-010101010101");
        const Uuid bit_more = Uuid("02020202-0202-0202-0202-020202020202");
        const Uuid verymuch = Uuid("ffffffff-ffff-ffff-ffff-ffffffffffff");

        REQUIRE(bit_more == bit_more);
        REQUIRE(bit_more < verymuch);
        REQUIRE(bit_more <= verymuch);
        REQUIRE(bit_more != verymuch);
        REQUIRE(bit_more >= not_much);
        REQUIRE(bit_more > not_much);
    }

    SECTION("We can extract the first and second word of a Uuid")
    {
        Uuid::Bytes array = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
        const Uuid uuid(array);
        const auto words = uuid.to_words();

        REQUIRE(words.first >> 7 * bitsizeof<uchar>() == 1);
        REQUIRE(words.second >> 7 * bitsizeof<uchar>() == 9);
    }

    SECTION("UUIDs can be cast from and to string")
    {
        Uuid uuid = Uuid::generate();
        std::string implicit_string = uuid;
        std::string explicit_string = uuid.to_string();

        REQUIRE(implicit_string == explicit_string);
        REQUIRE(uuid == Uuid(implicit_string));

        std::string valid_uuid1 = "53b55247-b2a5-45f7-812a-b6210fdcdaef";
        std::string valid_uuid2 = "53B55247-B2A5-45F7-812A-B6210FDCDAEF";
        REQUIRE(!Uuid(valid_uuid1).is_null());
        REQUIRE(!Uuid(valid_uuid2).is_null());
    }

    SECTION("UUIDs can dump into an ostream")
    {
        const Uuid uuid = Uuid::generate();
        const std::string string = uuid;

        std::stringstream ss;
        ss << uuid;
        const std::string streamed = ss.str();
        REQUIRE(string == streamed);
    }

    SECTION("trying to parse an invalid UUID string results in a null UUID")
    {
        // one broken byte
        std::string invalid_uuid01 = "XXb55247-b2a5-45f7-812a-b6210fdcdaef";
        std::string invalid_uuid02 = "53XX5247-b2a5-45f7-812a-b6210fdcdaef";
        std::string invalid_uuid03 = "53b5XX47-b2a5-45f7-812a-b6210fdcdaef";
        std::string invalid_uuid04 = "53b552XX-b2a5-45f7-812a-b6210fdcdaef";
        std::string invalid_uuid05 = "53b55247-XXa5-45f7-812a-b6210fdcdaef";
        std::string invalid_uuid06 = "53b55247-b2XX-45f7-812a-b6210fdcdaef";
        std::string invalid_uuid07 = "53b55247-b2a5-XXf7-812a-b6210fdcdaef";
        std::string invalid_uuid08 = "53b55247-b2a5-45XX-812a-b6210fdcdaef";
        std::string invalid_uuid09 = "53b55247-b2a5-45f7-XX2a-b6210fdcdaef";
        std::string invalid_uuid10 = "53b55247-b2a5-45f7-81XX-b6210fdcdaef";
        std::string invalid_uuid11 = "53b55247-b2a5-45f7-812a-XX210fdcdaef";
        std::string invalid_uuid12 = "53b55247-b2a5-45f7-812a-b6XX0fdcdaef";
        std::string invalid_uuid13 = "53b55247-b2a5-45f7-812a-b621XXdcdaef";
        std::string invalid_uuid14 = "53b55247-b2a5-45f7-812a-b6210fXXdaef";
        std::string invalid_uuid15 = "53b55247-b2a5-45f7-812a-b6210fdcXXef";
        std::string invalid_uuid16 = "53b55247-b2a5-45f7-812a-b6210fdcdaXX";
        REQUIRE(!Uuid(invalid_uuid01));
        REQUIRE(!Uuid(invalid_uuid02));
        REQUIRE(!Uuid(invalid_uuid03));
        REQUIRE(!Uuid(invalid_uuid04));
        REQUIRE(!Uuid(invalid_uuid05));
        REQUIRE(!Uuid(invalid_uuid06));
        REQUIRE(!Uuid(invalid_uuid07));
        REQUIRE(!Uuid(invalid_uuid08));
        REQUIRE(!Uuid(invalid_uuid09));
        REQUIRE(!Uuid(invalid_uuid10));
        REQUIRE(!Uuid(invalid_uuid11));
        REQUIRE(!Uuid(invalid_uuid12));
        REQUIRE(!Uuid(invalid_uuid13));
        REQUIRE(!Uuid(invalid_uuid14));
        REQUIRE(!Uuid(invalid_uuid15));
        REQUIRE(!Uuid(invalid_uuid16));

        std::string invalid_uuid1 = "53b55247--b2a5-45f7-812a-b6210fdcdaef";
        std::string invalid_uuid2 = "53b55247-b2a5-45f7-812a-b6210fdcaef";
        std::string invalid_uuid3 = "53b55247b2a545f7812ab6210fdcdaef";
        std::string invalid_uuid4;
        std::string invalid_uuid5 = "helloiamauuidplease,thankyou";
        REQUIRE(!Uuid(invalid_uuid1));
        REQUIRE(!Uuid(invalid_uuid2));
        REQUIRE(!Uuid(invalid_uuid3));
        REQUIRE(!Uuid(invalid_uuid4));
        REQUIRE(!Uuid(invalid_uuid5));
    }

    SECTION("UUIDs can be build from a vector of integrals")
    {
        const Uuid uuid(std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16});
        REQUIRE(uuid == Uuid("01020304-0506-0708-090a-0b0c0d0e0f10"));

        REQUIRE_THROWS_AS(Uuid(std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9}), ValueError);
        REQUIRE_THROWS_AS(Uuid(std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 999}), ValueError);
    }

    SECTION("UUIDs can be hashed") { REQUIRE(hash(Uuid::generate()) != hash(Uuid::generate())); }
}
