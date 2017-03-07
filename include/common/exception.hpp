#pragma once

#include <stdexcept>

namespace notf {

class division_by_zero : public std::logic_error {
public: // methods
    division_by_zero()
        : std::logic_error("Division by zero!") {}

    ~division_by_zero();
};

} // namespace notf
