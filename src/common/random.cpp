#include "notf/common/random.hpp"

NOTF_OPEN_COMMON_NAMESPACE

std::string random_string(const size_t length, const bool lowercase, const bool uppercase, const bool digits)
{
    if (lowercase) {
        if (uppercase) {
            if (digits) {
                static const std::string pool = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
                return random_string(length, pool);
            }
            else {
                static const std::string pool = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
                return random_string(length, pool);
            }
        }
        else { // no uppercase
            if (digits) {
                static const std::string pool = "abcdefghijklmnopqrstuvwxyz0123456789";
                return random_string(length, pool);
            }
            else {
                static const std::string pool = "abcdefghijklmnopqrstuvwxyz";
                return random_string(length, pool);
            }
        }
    }
    else { // no lowercase
        if (uppercase) {
            if (digits) {
                static const std::string pool = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
                return random_string(length, pool);
            }
            else {
                static const std::string pool = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
                return random_string(length, pool);
            }
        }
        else { // no uppercase
            if (digits) {
                static const std::string pool = "0123456789";
                return random_string(length, pool);
            }
            else {
                static const std::string pool = "";
                return random_string(length, pool);
            }
        }
    }
}

std::string random_string(const size_t length, const std::string_view pool)
{
    if (pool.empty()) {
        return {};
    }
    std::string result(length, pool[0]);
    auto random_engine = randutils::global_rng();
    for (size_t i = 0; i < length; ++i) {
        result[i] = *random_engine.choose(std::begin(pool), std::end(pool));
    }
    return result;
}

NOTF_CLOSE_COMMON_NAMESPACE
