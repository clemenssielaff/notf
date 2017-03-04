#include "common/float_utils.hpp"

#include <iostream>

using namespace notf;

std::ostream& operator<<(std::ostream& out, const _approx<float>& approx)
{
    return out << "approx(" << approx.value << ", " << approx.epsilon << ")";
}

std::ostream& operator<<(std::ostream& out, const _approx<double>& approx)
{
    return out << "approx(" << approx.value << ", " << approx.epsilon << ")";
}

std::ostream& operator<<(std::ostream& out, const _approx<long double>& approx)
{
    return out << "approx(" << approx.value << ", " << approx.epsilon << ")";
}
