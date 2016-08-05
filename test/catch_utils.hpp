#pragma once

#include <string>

namespace Catch {

std::string toString(int v) { return std::to_string(v); }
std::string toString(uint v) { return std::to_string(v); }
std::string toString(float v) { return std::to_string(v); }
std::string toString(double v) { return std::to_string(v); }
std::string toString(bool v) { return std::to_string(v); }
}
