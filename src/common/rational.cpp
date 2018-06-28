#include "common/rational.hpp"

#include <iostream>

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

no_rational_error::~no_rational_error() = default;

// ================================================================================================================== //

std::ostream& operator<<(std::ostream& out, const Rationali& rational)
{
    return out << "Rational<int>(" << rational.get_numerator() << " / " << rational.get_denominator() << ")";
}

NOTF_CLOSE_NAMESPACE
