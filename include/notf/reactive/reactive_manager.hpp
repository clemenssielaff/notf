#pragma once

#include <unordered_map>

#include "./reactive.hpp"

NOTF_OPEN_NAMESPACE

// reactive manager ================================================================================================= //

class ReactiveManager {

    // types -------------------------------------------------------------------------------------------------------- //
public:

    class Register {
        struct automatic_registration {
            automatic_registration()
            {

            }
        };
    };

private:
    std::unordered_map<std::string,
};

NOTF_CLOSE_NAMESPACE
