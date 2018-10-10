#include "notf/common/random.hpp"

NOTF_OPEN_NAMESPACE

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

std::string random_string(const size_t length, const bool lowercase, const bool uppercase, const bool digits)
{
    if (lowercase) {
        if (uppercase) {
            if (digits) {
                static const char* pool = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
                return random_string(length, pool);
            }
            else {
                static const char* pool = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
                return random_string(length, pool);
            }
        }
        else { // no uppercase
            if (digits) {
                static const char* pool = "abcdefghijklmnopqrstuvwxyz0123456789";
                return random_string(length, pool);
            }
            else {
                static const char* pool = "abcdefghijklmnopqrstuvwxyz";
                return random_string(length, pool);
            }
        }
    }
    else { // no lowercase
        if (uppercase) {
            if (digits) {
                static const char* pool = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
                return random_string(length, pool);
            }
            else {
                static const char* pool = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
                return random_string(length, pool);
            }
        }
        else { // no uppercase
            if (digits) {
                static const char* pool = "0123456789";
                return random_string(length, pool);
            }
            else {
                static const char* pool = "";
                return random_string(length, pool);
            }
        }
    }
}

NOTF_CLOSE_NAMESPACE
